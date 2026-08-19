#include "watchdog.h"
#include "jack_manager.h"
#include "config.h"
#include "utils.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QThread>

Watchdog::Watchdog(JackManager& jackManager, Config& config, QObject* parent)
    : QObject(parent), m_jackManager(jackManager), m_config(config) {

    m_isRunning = false;
    m_isSelfRestart = false;
    m_restartCount = 0;
    m_failureCount = 0;
    m_backoffDelay = 5;

    m_pidFile = m_config.getStateFilePath();
    m_watchdogPidFile = m_config.getSettingsDir() + "/watchdog.pid";

    initialize();
}

Watchdog::~Watchdog() {
    stop();
}

bool Watchdog::isRunning() const {
    return m_isRunning;
}

bool Watchdog::isSelfRestart() const {
    return m_isSelfRestart;
}

void Watchdog::setSelfRestart(bool enabled) {
    m_isSelfRestart = enabled;
}

void Watchdog::start() {
    if (m_isRunning) {
        Utils::logWarn("Watchdog is already running");
        return;
    }

    // Check if another watchdog is already running
    if (QFile::exists(m_watchdogPidFile)) {
        QFile pidFile(m_watchdogPidFile);
        if (pidFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString pid = pidFile.readLine().trimmed();
            if (!pid.isEmpty()) {
                QProcess checkProcess;
                checkProcess.start("kill", QStringList() << "-0" << pid);
                checkProcess.waitForFinished(1000);
                if (checkProcess.exitCode() == 0) {
                    Utils::logWarn(QString("Watchdog already running (PID: %1)").arg(pid));
                    return;
                }
                // Stale pid file: process no longer exists, reclaim it.
                Utils::logWarn(QString("Removing stale watchdog pid file (PID %1 not running)").arg(pid));
                QFile::remove(m_watchdogPidFile);
            }
        }
    }

    Utils::logInfo("Starting watchdog...");

    // Save watchdog PID
    saveWatchdogPid();

    // Set up signal handling
    qApp->installEventFilter(this);

    // Start watch loop
    m_isRunning = true;
    startWatchLoop();

    emit started();
    Utils::logInfo("Watchdog started");
}

void Watchdog::stop() {
    if (!m_isRunning) {
        return;
    }

    Utils::logInfo("Stopping watchdog...");

    stopWatchLoop();
    removeWatchdogPid();

    m_isRunning = false;
    emit stopped();

    Utils::logInfo("Watchdog stopped");
}

void Watchdog::restart() {
    if (!m_isRunning) {
        return;
    }

    Utils::logInfo("Restarting watchdog...");
    stop();
    start();
}

void Watchdog::initialize() {
    // Initialize timers
    m_watchTimer = new QTimer(this);
    m_watchTimer->setInterval(5000); // Check every 5 seconds
    connect(m_watchTimer, &QTimer::timeout, this, &Watchdog::checkJackStatus);

    m_backoffTimer = new QTimer(this);
    m_backoffTimer->setSingleShot(true);
    connect(m_backoffTimer, &QTimer::timeout, this, [this]() {
        // Resume monitoring with the (increased) backoff interval so the
        // watchdog keeps working even after repeated failures.
        m_watchTimer->setInterval(m_backoffDelay * 1000);
        m_watchTimer->start();
        m_failureCount = 0;
        checkJackStatus();
    });
}

void Watchdog::startWatchLoop() {
    m_watchTimer->start();
}

void Watchdog::stopWatchLoop() {
    m_watchTimer->stop();
    m_backoffTimer->stop();
}

void Watchdog::checkJackStatus() {
    // Check if sysmode is active (watchdog should not restart JACK)
    if (Utils::sysmodeIsActive()) {
        Utils::logInfo("Sysmode active: watchdog not monitoring JACK");
        return;
    }

    // Check if JACK is running
    if (!m_jackManager.isRunning()) {
        m_failureCount++;
        m_restartCount++;

        Utils::logWarn(QString("JACK not running (restart #%1). Attempting to restart...")
                      .arg(m_restartCount));

        restartJack();
    } else {
        m_failureCount = 0;
        // Restore the fast 5s interval after a successful check that
        // followed a backoff period, and fully reset the backoff state.
        if (m_watchTimer->interval() != 5000) {
            resetBackoff();
        }
    }

    // Increase backoff after 3 failures
    if (m_failureCount >= 3) {
        increaseBackoff();
    }
}

void Watchdog::restartJack() {
    // Stop JACK if running
    if (m_jackManager.isRunning()) {
        m_jackManager.stop();
    }

    // Wait a bit before restarting
    QThread::msleep(1000);

    // Get current settings
    QString sr = m_config.getSampleRate();
    QString buf = m_config.getBufferSize();
    QString per = m_config.getPeriods();

    // Start JACK
    if (!m_jackManager.start(sr, buf, per)) {
        Utils::logError("Failed to restart JACK");
    } else {
        Utils::logInfo("JACK restarted successfully");
        m_restartCount = 0;
        emit jackRestarted();
    }

    // Check for sysmode auto-restore
    handleSysmodeAutoRestore();
}

void Watchdog::increaseBackoff() {
    if (m_backoffDelay < 30) {
        m_backoffDelay += 5;
    }

    Utils::logWarn(QString("3 failures detected. Backing off to %1 second intervals.")
                  .arg(m_backoffDelay));

    m_watchTimer->stop();
    m_backoffTimer->start(m_backoffDelay * 1000);
}

void Watchdog::resetBackoff() {
    m_backoffDelay = 5;
    m_failureCount = 0;
    m_watchTimer->start();
}

void Watchdog::saveWatchdogPid() {
    QFile pidFile(m_watchdogPidFile);
    if (pidFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        pidFile.write(QString::number(QCoreApplication::applicationPid()).toUtf8());
        pidFile.close();
    }
}

void Watchdog::removeWatchdogPid() {
    QFile::remove(m_watchdogPidFile);
}

void Watchdog::handleSysmodeAutoRestore() {
    // Check if sysmode should be auto-restored
    QString stateFile = m_config.getStateFilePath();
    QFile stateFileObj(stateFile);
    if (stateFileObj.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&stateFileObj);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("SYSMODE_AUTO_RESTORE=1")) {
                Utils::logInfo("Sysmode auto-restore detected. Restoring sysmode...");
                m_isSelfRestart = true;

                // Restore sysmode (use real binary path, not bare name)
                QString mgr = QCoreApplication::applicationFilePath();
                if (mgr.isEmpty() || !mgr.endsWith("tascam-us122l-manager")) {
                    mgr = QStandardPaths::findExecutable("tascam-us122l-manager");
                }
                if (!mgr.isEmpty()) {
                    QProcess sysmodeProcess;
                    sysmodeProcess.start(mgr, QStringList() << "--sysmode" << "on");
                    sysmodeProcess.waitForFinished(5000);
                }

                m_isSelfRestart = false;

                emit sysmodeAutoRestore();
                break;
            }
        }
        stateFileObj.close();
    }
}

bool Watchdog::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Quit) {
        // Handle application quit
        stop();
    }
    return QObject::eventFilter(watched, event);
}