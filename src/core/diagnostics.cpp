#include "diagnostics.h"
#include "config.h"
#include "utils.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QSysInfo>

Diagnostics::Diagnostics(Config& config, QObject* parent)
    : QObject(parent), m_config(config) {

    // Initialize
}

Diagnostics::~Diagnostics() {
    // Cleanup
}

bool Diagnostics::isCardDetected() const {
    return Utils::checkDriverLoaded();
}

QString Diagnostics::getCardModel() const {
    return Utils::detectTascamModel();
}

QString Diagnostics::getCardNumber() const {
    return Utils::getCardNumber();
}

QString Diagnostics::getSampleWidth() const {
    return Utils::getSampleWidth();
}

QString Diagnostics::getUSBConnection() const {
    return Utils::getUSBConnection();
}

QString Diagnostics::getFirmwareVersion() const {
    return Utils::getFirmwareVersion();
}

QString Diagnostics::getDriverKernelVersion() const {
    return Utils::getDriverKernelVersion();
}

QString Diagnostics::getMidiClientName() const {
    return Utils::getMidiClientName();
}

bool Diagnostics::isJackRunning() const {
    return Utils::isJackRunning();
}

QString Diagnostics::getJackSamplerate() const {
    return Utils::getJackSamplerate();
}

QString Diagnostics::getJackBuffer() const {
    return Utils::getJackBuffer();
}

int Diagnostics::getXrunCount() const {
    QString logContent = getLogContent();
    QStringList xruns = parseXruns(logContent);
    return xruns.size();
}

bool Diagnostics::testMidiLoopback() const {
    return Utils::testMidiLoopback();
}

bool Diagnostics::isBridgeActive() const {
    return Utils::bridgeIsActive();
}

bool Diagnostics::isSysmodeActive() const {
    return Utils::sysmodeIsActive();
}

bool Diagnostics::isAutostartEnabled() const {
    return m_config.autostartEnabled();
}

QString Diagnostics::generateReport() const {
    QString report;
    report += "===== TASCAM DIAGNOSTICA =====\n\n";

    report += "--- Sistema ---\n";
    report += "Kernel:  " + QSysInfo::kernelVersion() + "\n";
    report += "Distro:  CachyOS (Arch Linux)\n";
    report += "GUI:     Qt6 (C++17)\n\n";

    report += "--- Dispositivo USB ---\n";
    report += "Modello: " + getCardModel() + "\n";
    report += "Scheda:  " + getCardNumber() + "\n";
    report += "USB:     " + getUSBConnection() + "\n";
    report += "Firmware: " + getFirmwareVersion() + "\n";
    report += "Campionamento: 24-bit\n\n";

    report += "--- ALSA ---\n";
    report += "Driver: " + QString(Utils::checkDriverLoaded() ? "caricato" : "NON caricato") + "\n";
    report += ".asoundrc: " + QString(Utils::checkAsoundrc() ? "configurato" : "NON configurato") + "\n\n";

    report += "--- JACK ---\n";
    report += "Stato: " + QString(isJackRunning() ? "ATTIVO" : "INATTIVO") + "\n";
    report += "PID: " + getJackPID() + "\n";
    report += "Versione: " + getJackVersion() + "\n";
    report += "Frequenza: " + getJackSamplerate() + " Hz\n";
    report += "Buffer: " + getJackBuffer() + " frame\n";
    report += "Xruns: " + QString::number(getXrunCount()) + "\n\n";

    report += "--- PipeWire ---\n";
    report += "Bridge: " + QString(isBridgeActive() ? "attivo" : "inattivo") + "\n";
    report += "Sysmode: " + QString(isSysmodeActive() ? "attivo" : "inattivo") + "\n\n";

    report += "--- MIDI ---\n";
    report += "Client: " + getMidiClientName() + "\n";
    report += "Loopback test: " + QString(testMidiLoopback() ? "OK" : "FAIL") + "\n\n";

    report += "--- Avvio automatico ---\n";
    report += "Attivo: " + QString(isAutostartEnabled() ? "si" : "no") + "\n\n";

    report += "--- Configurazioni ---\n";
    report += "Settings: " + m_config.getSettingsDir() + "/settings.conf\n";
    report += "State: " + m_config.getStateFilePath() + "\n";
    report += "Log: " + getLogFilePath() + "\n\n";

    report += "=== Log JACK ===\n";
    report += getLogContent();
    report += "\n";

    return report;
}

QString Diagnostics::getLogFilePath() const {
    return m_config.getLogFilePath();
}

QString Diagnostics::getLogContent() const {
    QString logPath = getLogFilePath();
    QFile logFile(logPath);

    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "File di log non trovato o non leggibile";
    }

    // Read last 1000 lines
    QTextStream in(&logFile);
    QString content;
    QStringList lines;
    while (!in.atEnd()) {
        lines.prepend(in.readLine());
        if (lines.size() > 1000) {
            lines.removeLast();
        }
    }

    return lines.join('\n');
}

QStringList Diagnostics::getLastXruns() const {
    QString logContent = getLogContent();
    return parseXruns(logContent);
}

QString Diagnostics::getCardInfo() const {
    QString info;
    info += "=== Tascam US-122L - Info Scheda ===\n\n";
    info += "Numero scheda:  " + getCardNumber() + "\n";
    info += "Modello:        " + getCardModel() + "\n";
    info += "Connessione:    " + getUSBConnection() + "\n";
    info += "Firmware:       " + getFirmwareVersion() + "\n";
    info += "Campionamento:  " + getSampleWidth() + "\n";
    info += "Driver ALSA:    " + QString(Utils::checkDriverLoaded() ? "caricato" : "NON caricato") + "\n";
    info += "Driver kernel:  " + getDriverKernelVersion() + "\n";
    info += "Device MIDI:    " + getMidiClientName() + "\n";
    info += "Server JACK:    " + QString(isJackRunning() ? "ATTIVO" : "INATTIVO") + "\n";

    if (isJackRunning()) {
        info += "Frequenza:      " + getJackSamplerate() + " Hz\n";
        info += "Buffer:         " + getJackBuffer() + " frame\n";
    }

    info += "\n=== Configurazione ===\n";
    info += ".asoundrc: " + QString(Utils::checkAsoundrc() ? "configurato" : "NON configurato") + "\n\n";

    return info;
}

QString Diagnostics::getAlsaConfigInfo() const {
    QString configPath = m_config.getAsoundrcPath();
    QFile configFile(configPath);

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "File di configurazione non trovato";
    }

    return configFile.readAll();
}

QString Diagnostics::getJackPID() const {
    if (!isJackRunning()) {
        return "N/A";
    }

    QStringList pids = Utils::getLivePids("jackd");
    if (!pids.isEmpty()) {
        return pids.first();
    }
    return "N/A";
}

QString Diagnostics::getJackVersion() const {
    QProcess jackd;
    jackd.start("jackd", QStringList() << "--version");
    jackd.waitForFinished(1000);

    QString output = jackd.readAllStandardOutput().trimmed();
    if (!output.isEmpty()) {
        return output;
    }

    return "unknown";
}

QStringList Diagnostics::parseXruns(const QString& logContent) const {
    QStringList xruns;
    QStringList lines = logContent.split('\n');

    for (const QString& line : lines) {
        if (line.contains("xrun", Qt::CaseInsensitive) ||
            line.contains("underrun", Qt::CaseInsensitive) ||
            line.contains("overrun", Qt::CaseInsensitive)) {
            xruns << line.trimmed();
        }
    }

    return xruns;
}