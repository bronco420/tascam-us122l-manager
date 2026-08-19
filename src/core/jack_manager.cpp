#include "jack_manager.h"
#include "config.h"
#include "utils.h"
#include <QDebug>
#include <QThread>
#include <QFileInfo>
#include <QRegularExpression>

JackManager::JackManager(Config& config, QObject* parent)
    : QObject(parent), m_config(config), m_jackProcess(new QProcess(this)), m_isRunning(false) {

    m_pidFile = config.getSettingsDir() + "/jack.pid";
    m_logFile = config.getSettingsDir() + "/jack.log";

    // Connect process signals
    connect(m_jackProcess, &QProcess::started, this, &JackManager::started);
    connect(m_jackProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &JackManager::stop);

    // Start status timer
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(1000);
    connect(m_statusTimer, &QTimer::timeout, [this]() {
        if (m_isRunning) {
            // Check if jackd is still running
            QString pid = getPID();
            if (pid.isEmpty()) {
                m_isRunning = false;
                emit stopped();
            }
        }
    });
}

JackManager::~JackManager() {
    stop();
    m_statusTimer->stop();
}

bool JackManager::isRunning() const {
    if (m_isRunning) {
        return true;
    }
    // Also detect jackd started externally (previous session or manual start)
    return Utils::isJackRunning();
}

bool JackManager::isDriverLoaded() const {
    QFile modulesFile("/proc/modules");
    if (!modulesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString data = QString::fromUtf8(modulesFile.readAll());
    return data.contains("snd_usb_us122l");
}

QString JackManager::getSampleRate() const {
    return m_config.getSampleRate();
}

QString JackManager::getBufferSize() const {
    return m_config.getBufferSize();
}

QString JackManager::getPeriods() const {
    return m_config.getPeriods();
}

QString JackManager::getPID() const {
    QFile pidFile(m_pidFile);
    if (pidFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&pidFile);
        QString pid = in.readLine().trimmed();
        pidFile.close();

        // Verify PID is valid and alive
        if (!pid.isEmpty()) {
            if (Utils::getLivePids("jackd").contains(pid)) {
                return pid;
            }
        }
    }

    // Fallback: find jackd process
    QStringList pids = Utils::getLivePids("jackd");
    if (!pids.isEmpty()) {
        return pids.first();
    }

    return "";
}

bool JackManager::start(const QString& sampleRate, const QString& bufferSize, const QString& periods) {
    // Check if JACK is already running
    if (isRunning()) {
        qDebug() << "JACK is already running";
        return false;
    }

    QString sr = sampleRate;
    QString buf = bufferSize;
    QString per = periods;

    // Validate parameters
    if (!validateParams(sr, buf, per)) {
        qDebug() << "Invalid parameters, using defaults (48000/128/2)";
        // Apply real defaults instead of using the invalid values below
        sr = "48000";
        buf = "128";
        per = "2";
    }

    // Check if Tascam is detected
    if (!checkCardDetected()) {
        emit errorOccurred("Tascam US-122L not detected!");
        return false;
    }

    // Setup ALSA config
    setupAlsaConfig();

    // Cleanup old JACK instances
    cleanupOldJack();

    // Rotate log if too large
    rotateLog();

    // Create settings directory
    QDir().mkpath(m_config.getSettingsDir());

    qDebug() << "Starting JACK with sample rate:" << sr
             << "buffer size:" << buf
             << "periods:" << per;

    // Build jackd command
    QString cardNum = "0"; // Default card number
    QString launcher = m_config.getLauncherPath();

    QStringList jackdArgs;
    jackdArgs << "-vR" << "-P50" << "-t20000" << "-S";
    jackdArgs << "-dalsa";
    jackdArgs << "-dusb_stream:" + cardNum;
    jackdArgs << "-r" + sr;
    jackdArgs << "-p" + buf;
    jackdArgs << "-n" + per;

    // The launcher expects: --pidfile FILE -- jackd [args...]
    QStringList launcherArgs;
    launcherArgs << "--pidfile" << m_pidFile << "--" << "jackd" << jackdArgs;

    // Start JACK with launcher (subreaper), detached so jackd survives
    // after the CLI process exits. The launcher writes the real jackd PID
    // to the pidfile, which is used by isRunning()/getPID()/stop().
    qint64 launcherPid = 0;
    if (!QProcess::startDetached("python3", QStringList() << launcher << launcherArgs, QString(), &launcherPid)) {
        qDebug() << "Failed to start launcher";
        emit errorOccurred("Failed to start the JACK launcher.");
        return false;
    }

    // Wait for JACK to start
    if (!waitForJackStart(5)) {
        emit errorOccurred("Failed to start JACK. Check the log for details.");
        return false;
    }

    m_isRunning = true;
    emit started();

    qDebug() << "JACK started successfully with PID:" << getPID();

    return true;
}

void JackManager::stop() {
    // Stop jackd whether it was started by us or externally
    if (!isRunning()) {
        return;
    }

    QString pid = getPID();
    if (!pid.isEmpty()) {
        qDebug() << "Stopping JACK (PID:" << pid << ")";

        // Try graceful stop first
        QProcess stopProcess;
        stopProcess.start("kill", QStringList() << pid);
        stopProcess.waitForFinished(1000);

        // Force stop if needed
        if (stopProcess.exitCode() != 0) {
            qDebug() << "Graceful stop failed, forcing...";
            QProcess forceProcess;
            forceProcess.start("kill", QStringList() << "-9" << pid);
            forceProcess.waitForFinished(1000);
        }
    }

    // Clean up PID file
    QFile::remove(m_pidFile);

    m_isRunning = false;
    emit stopped();
}

void JackManager::restart() {
    stop();
    QThread::sleep(1);

    QString sr = getSampleRate();
    QString buf = getBufferSize();
    QString per = getPeriods();

    if (!start(sr, buf, per)) {
        emit errorOccurred("Failed to restart JACK");
    }
}

void JackManager::saveSettings(const QString& sr, const QString& buf, const QString& per) {
    m_config.setSampleRate(sr);
    m_config.setBufferSize(buf);
    m_config.setPeriods(per);
}

void JackManager::loadSettings(QString& sr, QString& buf, QString& per) {
    sr = m_config.getSampleRate();
    buf = m_config.getBufferSize();
    per = m_config.getPeriods();
}

bool JackManager::validateParams(const QString& sr, const QString& buf, const QString& per) {
    // Sample rate validation
    static const QStringList validSR = {"44100", "48000", "88200", "96000"};
    if (!validSR.contains(sr)) {
        qDebug() << "Invalid sample rate:" << sr;
        return false;
    }

    // Buffer size validation
    static const QStringList validBuf = {"64", "128", "256", "512"};
    if (!validBuf.contains(buf)) {
        qDebug() << "Invalid buffer size:" << buf;
        return false;
    }

    // Periods validation
    static const QStringList validPer = {"2", "3", "4"};
    if (!validPer.contains(per)) {
        qDebug() << "Invalid periods:" << per;
        return false;
    }

    return true;
}

bool JackManager::checkCardDetected() {
    QFile cardsFile("/proc/asound/cards");
    if (!cardsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString data = QString::fromUtf8(cardsFile.readAll());
    cardsFile.close();
    return data.contains("US122L") || data.contains("US-144");
}

void JackManager::setupAlsaConfig() {
    QString asoundrcPath = m_config.getAsoundrcPath();

    // Backup if doesn't exist
    if (!QFile::exists(asoundrcPath + ".bak")) {
        QFile::copy(asoundrcPath, asoundrcPath + ".bak");
    }

    // If the config already exists and declares rate/period_size explicitly,
    // keep it: the usb_stream plugin goes into timeout on long streams
    // without an explicit period_size (no sound + missing microphone source).
    QFile existing(asoundrcPath);
    if (existing.exists()) {
        if (existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString data = QString::fromUtf8(existing.readAll());
            existing.close();
            if (data.contains("period_size") && data.contains("rate 48000")) {
                qDebug() << "ALSA config is already complete (rate + period_size)";
                return;
            }
        }
    }

    // Create usb_stream configuration with explicit rate and period_size.
    // Both JACK (-dusb_stream) and PipeWire sysmode (module-alsa-sink/source
    // on the "usb_stream" PCM) depend on these values.
    QFile file(asoundrcPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "# Tascam US-122L - Plugin usb_stream for JACK Audio Connection Kit\n";
        out << "# This plugin is REQUIRED for the US-122L on Linux.\n";
        out << "# The card does NOT work with standard PCM devices (hw:, default:)\n";
        out << "# rate and period_size MUST be explicit: without period_size the\n";
        out << "# usb_stream plugin times out on long streams (no sound, no mic).\n";
        out << "pcm.!usb_stream {\n";
        out << "    @args [ CARD ]\n";
        out << "    @args.CARD {\n";
        out << "        type string\n";
        out << "        default \"US122L\"\n";
        out << "    }\n";
        out << "    type usb_stream\n";
        out << "    card $CARD\n";
        out << "    rate 48000\n";
        out << "    period_size 128\n";
        out << "}\n";
        out << "\n";
        out << "ctl.!usb_stream {\n";
        out << "    @args [ CARD ]\n";
        out << "    @args.CARD {\n";
        out << "        type string\n";
        out << "        default \"US122L\"\n";
        out << "    }\n";
        out << "    type hw\n";
        out << "    card $CARD\n";
        out << "}\n";
        file.close();

        qDebug() << "ALSA config created at:" << asoundrcPath;
    }
}

void JackManager::cleanupOldJack() {
    // Kill any existing jackd processes
    QProcess killProcess;
    killProcess.start("pkill", QStringList() << "-x" << "jackd");
    killProcess.waitForFinished(1000);

    // Remove shared memory files (expand globs explicitly: `rm` without a
    // shell does not expand `*`, so the pattern was being passed literally).
    const QStringList shmPatterns = {
        "/dev/shm/jack-1000-*",
        "/dev/shm/jack_sem.1000*"
    };
    for (const QString& pattern : shmPatterns) {
        QDir dir("/dev/shm");
        for (const QString& entry : dir.entryList({ QFileInfo(pattern).fileName() }, QDir::Files)) {
            QFile::remove("/dev/shm/" + entry);
        }
    }
    QFile::remove("/dev/shm/jack_db-1000");
    QFile::remove("/dev/shm/jack-shm-registry");

    QThread::sleep(1);
}

void JackManager::rotateLog() {
    QFile logFile(m_logFile);
    if (logFile.exists()) {
        qint64 size = logFile.size();
        if (size > 1024 * 1024) { // 1MB
            QFile::rename(m_logFile, m_logFile + ".old");
            qDebug() << "Log rotated (over 1MB)";
        }
    }
}

bool JackManager::waitForJackStart(int timeoutSeconds) {
    for (int i = 0; i < timeoutSeconds; i++) {
        if (isRunning()) {
            return true;
        }
        QThread::sleep(1);
    }
    return false;
}