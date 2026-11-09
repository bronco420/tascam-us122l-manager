#include "pipewire_bridge.h"
#include "config.h"
#include "utils.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QRegularExpression>
#include <QThread>
#include <csignal>
#include <unistd.h>

namespace {
const char* kModuleFileName = "libpipewire-module-jack-tunnel.so";
const char* kSinkNodeName = "tascam_bridge_sink";
const char* kSourceNodeName = "tascam_bridge_source";
const char* kBridgePidKey = "BRIDGE_PID";

bool processAlive(qint64 pid) {
    if (pid <= 0) {
        return false;
    }
    return ::kill(static_cast<pid_t>(pid), 0) == 0;
}

QString tailOfFile(const QString& path, int lines = 15) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "";
    }
    const QStringList all = QString::fromUtf8(file.readAll()).split('\n');
    file.close();
    return all.mid(qMax(0, all.size() - lines)).join('\n').trimmed();
}
}

PipeWireBridge::PipeWireBridge(Config& config, QObject* parent)
    : QObject(parent), m_config(config), m_previousSinkSaved(false), m_bridgePid(0) {

    // Load previous sink and bridge pid from state
    QString stateFile = m_config.getStateFilePath();
    QFile stateFileObj(stateFile);
    if (stateFileObj.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&stateFileObj);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("PREV_SINK=")) {
                m_previousSink = line.mid(10);
                m_previousSinkSaved = true;
            } else if (line.startsWith("BRIDGE_PID=")) {
                m_bridgePid = line.mid(11).trimmed().toLongLong();
            }
        }
        stateFileObj.close();
    }
}

PipeWireBridge::~PipeWireBridge() {
    // Keep bridge state alive: do not auto-disable on process exit.
}

QString PipeWireBridge::tunnelModuleName() {
    return QStringLiteral("libpipewire-module-jack-tunnel");
}

QString PipeWireBridge::tunnelSinkNodeName() {
    return QString::fromLatin1(kSinkNodeName);
}

QString PipeWireBridge::tunnelSourceNodeName() {
    return QString::fromLatin1(kSourceNodeName);
}

QString PipeWireBridge::bridgeLogPath() const {
    return m_config.getSettingsDir() + "/bridge.log";
}

QString PipeWireBridge::tunnelArgs() const {
    return QStringLiteral(
               "{ tunnel.mode = duplex "
               "audio.channels = 2 "
               "audio.position = [ FL FR ] "
               "jack.connect = true "
               "jack.client-name = tascam-bridge "
               "sink.props = { node.name = %1 node.description = \"Tascam US-122L Bridge Out\" } "
               "source.props = { node.name = %2 node.description = \"Tascam US-122L Bridge In\" } }")
        .arg(tunnelSinkNodeName(), tunnelSourceNodeName());
}

bool PipeWireBridge::isActive() const {
    return !getSinkName().isEmpty();
}

QString PipeWireBridge::findNodeByName(const QString& interface, const QString& name) const {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << interface);
    if (!pactl.waitForFinished(2000)) {
        pactl.kill();
        return "";
    }

    const QString output = pactl.readAllStandardOutput();
    for (const QString& line : output.split('\n')) {
        const QStringList parts = line.split('\t');
        if (parts.size() >= 2 && parts.at(1).trimmed() == name) {
            return parts.at(1).trimmed();
        }
    }
    return "";
}

QString PipeWireBridge::getSinkName() const {
    return findNodeByName("sinks", tunnelSinkNodeName());
}

QString PipeWireBridge::getSourceName() const {
    return findNodeByName("sources", tunnelSourceNodeName());
}

bool PipeWireBridge::tunnelModuleAvailable() const {
    const QStringList dirs = {
        QStringLiteral("/usr/lib/pipewire-0.3"),
        QStringLiteral("/usr/local/lib/pipewire-0.3"),
        QStringLiteral("/usr/lib64/pipewire-0.3"),
        QStringLiteral("/usr/lib/x86_64-linux-gnu/pipewire-0.3"),
    };
    for (const QString& dir : dirs) {
        if (QFileInfo::exists(dir + "/" + QString::fromLatin1(kModuleFileName))) {
            return true;
        }
    }
    return false;
}

bool PipeWireBridge::jackRunning() const {
    return Utils::isJackRunning();
}

bool PipeWireBridge::waitForSink(int timeoutMs) const {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (!getSinkName().isEmpty()) {
            return true;
        }
        QThread::msleep(100);
    }
    return false;
}

void PipeWireBridge::waitForSinkGone(int timeoutMs) const {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (getSinkName().isEmpty()) {
            return;
        }
        QThread::msleep(100);
    }
}

void PipeWireBridge::enable() {
    m_lastError.clear();

    if (isActive()) {
        Utils::logWarn("Bridge is already active");
        return;
    }

    if (!tunnelModuleAvailable()) {
        m_lastError = QString(
                          "Modulo PipeWire mancante: %1.\n"
                          "Installa il pacchetto 'pipewire-jack-client' "
                          "(sudo pacman -S pipewire-jack-client) e riprova.")
                          .arg(QString::fromLatin1(kModuleFileName));
        Utils::logError(m_lastError);
        emit bridgeError(m_lastError);
        return;
    }

    if (!jackRunning()) {
        m_lastError = QStringLiteral(
            "Il server JACK non è attivo.\n"
            "Avvia JACK (--start) prima di abilitare il bridge PipeWire.");
        Utils::logError(m_lastError);
        emit bridgeError(m_lastError);
        return;
    }

    // Rimuove eventuali processi pw-cli orfani di sessioni precedenti.
    unloadJackTunnel();

    Utils::logInfo("Enabling PipeWire-JACK bridge (libpipewire-module-jack-tunnel)...");

    // Save current default sink
    QString currentSink;
    QProcess pactl;
    pactl.start("pactl", QStringList() << "get-default-sink");
    pactl.waitForFinished(1000);

    QString output = pactl.readAllStandardOutput().trimmed();
    QStringList parts = output.split(' ');
    if (parts.size() >= 1 && !parts.at(0).isEmpty()) {
        currentSink = parts.at(0);
        savePreviousSink(currentSink);
        Utils::logInfo(QString("Saved previous sink: %1").arg(currentSink));
    }

    QString error;
    if (!loadJackTunnel(error)) {
        m_lastError = error;
        Utils::logError(error);
        clearPreviousSink();
        emit bridgeError(error);
        return;
    }

    const QString jackSink = getSinkName();
    if (!jackSink.isEmpty()) {
        QProcess setSink;
        setSink.start("pactl", QStringList() << "set-default-sink" << jackSink);
        setSink.waitForFinished(3000);
        Utils::logInfo(QString("Default sink impostato su %1").arg(jackSink));
    }

    Utils::logInfo(QString("PipeWire-JACK bridge enabled (pw-cli pid %1)").arg(m_bridgePid));
    emit bridgeEnabled();
}

void PipeWireBridge::disable() {
    m_lastError.clear();

    if (!isActive() && m_bridgePid <= 0 && findBridgeProcessPid() <= 0) {
        Utils::logWarn("Bridge is not active");
        clearPreviousSink();
        emit bridgeDisabled();
        return;
    }

    Utils::logInfo("Disabling PipeWire-JACK bridge...");
    unloadJackTunnel();
    waitForSinkGone(3000);

    // Restore previous sink
    QString prevSink = getPreviousSink();
    if (!prevSink.isEmpty()) {
        QProcess setSink;
        setSink.start("pactl", QStringList() << "set-default-sink" << prevSink);
        setSink.waitForFinished(3000);
        Utils::logInfo(QString("Restored previous sink: %1").arg(prevSink));
    }

    clearPreviousSink();

    Utils::logInfo("PipeWire-JACK bridge disabled");
    emit bridgeDisabled();
}

void PipeWireBridge::toggle() {
    if (isActive()) {
        disable();
    } else {
        enable();
    }
}

bool PipeWireBridge::loadJackTunnel(QString& error) {
    // Il modulo va caricato da un processo `pw-cli -m` che resta vivo:
    // con `pw-cli load-module` semplice il modulo verrebbe distrutto
    // all'uscita del processo.
    const QString logPath = bridgeLogPath();
    const QString command = QString("exec pw-cli -m load-module %1 '%2' >'%3' 2>&1")
                                .arg(tunnelModuleName(), tunnelArgs(), logPath);

    qint64 pid = 0;
    if (!QProcess::startDetached("/bin/sh", QStringList() << "-c" << command,
                                 QString(), &pid) ||
        pid <= 0) {
        error = QStringLiteral("Impossibile avviare pw-cli per caricare il modulo jack-tunnel.");
        return false;
    }

    m_bridgePid = pid;

    if (!waitForSink(8000)) {
        const QString logTail = tailOfFile(logPath);
        error = QString("Il modulo jack-tunnel non ha creato il sink PipeWire '%1'.\n"
                        "Verifica che jackd sia attivo e che il modulo sia presente.\n%2")
                    .arg(tunnelSinkNodeName(),
                         logTail.isEmpty() ? QString() : "Log:\n" + logTail);

        ::kill(static_cast<pid_t>(pid), SIGTERM);
        QThread::msleep(300);
        if (processAlive(pid)) {
            ::kill(static_cast<pid_t>(pid), SIGKILL);
        }
        m_bridgePid = 0;
        clearBridgePid();
        return false;
    }

    saveBridgePid(pid);
    Utils::logInfo(QString("libpipewire-module-jack-tunnel caricato (pw-cli pid %1)").arg(pid));
    return true;
}

void PipeWireBridge::unloadJackTunnel() {
    qint64 pid = m_bridgePid;
    if (!processAlive(pid)) {
        pid = findBridgeProcessPid();
    }

    if (pid > 0) {
        Utils::logInfo(QString("Stopping pw-cli bridge process %1").arg(pid));
        ::kill(static_cast<pid_t>(pid), SIGTERM);

        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 3000 && processAlive(pid)) {
            QThread::msleep(100);
        }
        if (processAlive(pid)) {
            Utils::logWarn(QString("pw-cli %1 non termina, invio SIGKILL").arg(pid));
            ::kill(static_cast<pid_t>(pid), SIGKILL);
            QThread::msleep(200);
        }
    }

    m_bridgePid = 0;
    clearBridgePid();
}

qint64 PipeWireBridge::findBridgeProcessPid() const {
    QProcess pgrep;
    pgrep.start("pgrep", QStringList() << "-f" << "pw-cli -m load-module libpipewire-module-jack-tunnel");
    if (!pgrep.waitForFinished(1500)) {
        pgrep.kill();
        return 0;
    }

    const QString output = pgrep.readAllStandardOutput().trimmed();
    for (const QString& line : output.split('\n')) {
        bool ok = false;
        const qint64 pid = line.trimmed().toLongLong(&ok);
        if (ok && pid > 0) {
            return pid;
        }
    }
    return 0;
}

void PipeWireBridge::saveBridgePid(qint64 pid) {
    QMap<QString, QString> updates;
    updates[QString::fromLatin1(kBridgePidKey)] = QString::number(pid);
    Utils::updateStateFile(m_config.getStateFilePath(), updates);
}

void PipeWireBridge::clearBridgePid() {
    QMap<QString, QString> updates;
    updates[QString::fromLatin1(kBridgePidKey)] = QString();
    Utils::updateStateFile(m_config.getStateFilePath(), updates);
}

void PipeWireBridge::savePreviousSink(const QString& sink) {
    m_previousSink = sink;
    m_previousSinkSaved = true;

    QMap<QString, QString> updates;
    updates["PREV_SINK"] = sink;
    Utils::updateStateFile(m_config.getStateFilePath(), updates);
}

QString PipeWireBridge::getPreviousSink() const {
    return m_previousSink;
}

void PipeWireBridge::clearPreviousSink() {
    m_previousSinkSaved = false;
    m_previousSink.clear();

    QMap<QString, QString> updates;
    updates["PREV_SINK"] = QString();
    Utils::updateStateFile(m_config.getStateFilePath(), updates);
}
