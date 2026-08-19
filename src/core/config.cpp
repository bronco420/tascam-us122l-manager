#include "config.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>
#include <QProcess>
#include <QTextStream>
#include <QCoreApplication>

static Config* s_instance = nullptr;

Config& Config::instance() {
    if (!s_instance) {
        s_instance = new Config();
    }
    return *s_instance;
}

Config::Config() {
    initPaths();
    initSettings();
}

void Config::initPaths() {
    // Application directory
    m_appDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
               "/tascam-us122l";

    // Settings directory (same as app dir)
    m_settingsDir = m_appDir;

    // Icons directory
    m_iconsDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
                 "/tascam-us122l/icons";

    // Product images directory
    m_productDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
                   "/tascam-us122l/product";

    // Launcher path (installed by CMake/build.sh, searched in common locations)
    m_launcherPath = findLauncherPath();

    // Ensure directories exist
    QDir().mkpath(m_settingsDir);
    QDir().mkpath(m_iconsDir);
    QDir().mkpath(m_productDir);
}

void Config::initSettings() {
    // Create settings file if it doesn't exist
    QString settingsFile = m_settingsDir + "/settings.conf";
    if (!QFile::exists(settingsFile)) {
        QFile file(settingsFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "SAMPLE_RATE=48000\n";
            out << "BUFFER_SIZE=128\n";
            out << "PERIODS=2\n";
            file.close();
        }
    }
}

QString Config::getAppDir() const {
    return m_appDir;
}

QString Config::getSettingsDir() const {
    return m_settingsDir;
}

QString Config::getAsoundrcPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
           "/.asoundrc";
}

QString Config::getLauncherPath() const {
    return m_launcherPath;
}

QString Config::getSampleRate() const {
    QSettings settings(m_settingsDir + "/settings.conf", QSettings::IniFormat);
    return settings.value("SAMPLE_RATE", "48000").toString();
}

void Config::setSampleRate(const QString& sr) {
    QSettings settings(m_settingsDir + "/settings.conf", QSettings::IniFormat);
    settings.setValue("SAMPLE_RATE", sr);
    settings.sync();
}

QString Config::getBufferSize() const {
    QSettings settings(m_settingsDir + "/settings.conf", QSettings::IniFormat);
    return settings.value("BUFFER_SIZE", "128").toString();
}

void Config::setBufferSize(const QString& buf) {
    QSettings settings(m_settingsDir + "/settings.conf", QSettings::IniFormat);
    settings.setValue("BUFFER_SIZE", buf);
    settings.sync();
}

QString Config::getPeriods() const {
    QSettings settings(m_settingsDir + "/settings.conf", QSettings::IniFormat);
    return settings.value("PERIODS", "2").toString();
}

void Config::setPeriods(const QString& per) {
    QSettings settings(m_settingsDir + "/settings.conf", QSettings::IniFormat);
    settings.setValue("PERIODS", per);
    settings.sync();
}

bool Config::asoundrcExists() const {
    return QFile::exists(getAsoundrcPath());
}

bool Config::autostartEnabled() const {
    QString autostartUnit = getAutostartUnit();
    return QFile::exists(autostartUnit);
}

void Config::setAutostart(bool enabled) {
    QString autostartUnit = getAutostartUnit();

    // Path to the real manager binary (works both when installed and from a build dir)
    QString managerBinary = QCoreApplication::applicationFilePath();
    if (managerBinary.isEmpty() || managerBinary.endsWith("tascam-us122l-manager") == false) {
        managerBinary = QStandardPaths::findExecutable("tascam-us122l-manager");
    }
    if (managerBinary.isEmpty()) {
        managerBinary = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                        + "/.local/bin/tascam-us122l-manager";
    }

    if (enabled) {
        // Create autostart script
        QString scriptPath = getAutostartScript();
        QFile script(scriptPath);
        if (script.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&script);
            out << "#!/bin/bash\n";
            out << "MGR=\"" << managerBinary << "\"\n";
            out << "if [ -f \"$HOME/.config/tascam-us122l/state.conf\" ]; then\n";
            out << "    \"$MGR\" --sysmode on --silent\n";
            out << "else\n";
            out << "    exec \"$MGR\" --watch --silent\n";
            out << "fi\n";
            script.close();

            // Make executable
            script.setPermissions(QFile::ExeOwner | QFile::ReadOwner |
                                 QFile::WriteOwner | QFile::ExeGroup |
                                 QFile::ReadGroup | QFile::WriteGroup);

            // Create systemd service
            QFile service(autostartUnit);
            if (service.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&service);
                out << "[Unit]\n";
                out << "Description=Tascam US-122L (system card or JACK watchdog)\n";
                out << "After=pipewire.service pipewire-pulse.service\n";
                out << "Wants=pipewire.service pipewire-pulse.service\n";
                out << "\n";
                out << "[Service]\n";
                out << "Type=oneshot\n";
                out << "RemainAfterExit=yes\n";
                out << "ExecStart=" << scriptPath << "\n";
                out << "ExecStop=-" << managerBinary << " --watch-stop --silent\n";
                out << "ExecStop=-" << managerBinary << " --stop --silent\n";
                out << "ExecStop=-" << managerBinary << " --sysmode off --silent\n";
                out << "\n";
                out << "[Install]\n";
                out << "WantedBy=default.target\n";
                service.close();

                // Enable service
                QProcess systemctl;
                systemctl.start("systemctl", QStringList() << "--user" << "daemon-reload");
                systemctl.waitForFinished(1000);

                systemctl.start("systemctl", QStringList() << "--user" << "enable" << "tascam-us122l.service");
                systemctl.waitForFinished(1000);

                qDebug() << "Auto-start enabled";
            }
        }
    } else {
        // Disable auto-start
        QProcess systemctl;
        systemctl.start("systemctl", QStringList() << "--user" << "disable" << "--now" << "tascam-us122l.service");
        systemctl.waitForFinished(1000);

        systemctl.start("systemctl", QStringList() << "--user" << "daemon-reload");
        systemctl.waitForFinished(1000);

        // Remove files
        QFile::remove(autostartUnit);
        QFile::remove(getAutostartScript());

        qDebug() << "Auto-start disabled";
    }
}

int Config::getSourceVolumeDefault() const {
    QSettings settings(m_settingsDir + "/settings.conf", QSettings::IniFormat);
    return qBound(0, settings.value("SOURCE_VOLUME_DEFAULT", 70).toInt(), 153);
}

void Config::setSourceVolumeDefault(int percent) {
    QSettings settings(m_settingsDir + "/settings.conf", QSettings::IniFormat);
    settings.setValue("SOURCE_VOLUME_DEFAULT", qBound(0, percent, 153));
    settings.sync();
}

QString Config::getAutostartUnit() const {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
           "/systemd/user/tascam-us122l.service";
}

QString Config::getAutostartScript() const {
    return m_settingsDir + "/autostart.sh";
}

QString Config::getIconsDir() const {
    return m_iconsDir;
}

QString Config::getLogoPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
           "/tascam-us122l/logo.jpg";
}

QString Config::getProductDir() const {
    return m_productDir;
}

QString Config::getTascamLogoPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
           "/tascam-us122l/tascam_logo.svg";
}

QString Config::getStateFilePath() const {
    return m_settingsDir + "/state.conf";
}

QString Config::getLogFilePath() const {
    return m_settingsDir + "/jack.log";
}

QString Config::getLockFilePath() const {
    return m_settingsDir + "/manager.lock";
}

QString Config::getActionLockFilePath() const {
    return m_settingsDir + "/action.lock";
}

QString Config::findLauncherPath() const {
    const QStringList candidates = {
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
            "/.local/bin/tascam-jackd-launcher.py",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
            "/.local/share/tascam-us122l/tascam-jackd-launcher.py",
        "/usr/share/tascam-us122l/tascam-jackd-launcher.py",
        "/usr/local/share/tascam-us122l/tascam-jackd-launcher.py",
    };

    for (const QString& path : candidates) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    return candidates.first();
}