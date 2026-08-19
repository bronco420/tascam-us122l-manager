#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QFile>
#include <QDir>

class Config;

class Diagnostics : public QObject {
    Q_OBJECT

public:
    explicit Diagnostics(Config& config, QObject* parent = nullptr);
    ~Diagnostics();

    // Device detection
    bool isCardDetected() const;
    QString getCardModel() const;
    QString getCardNumber() const;
    QString getSampleWidth() const;
    QString getUSBConnection() const;
    QString getFirmwareVersion() const;
    QString getDriverKernelVersion() const;
    QString getMidiClientName() const;

    // JACK diagnostics
    bool isJackRunning() const;
    QString getJackSamplerate() const;
    QString getJackBuffer() const;
    int getXrunCount() const;

    // MIDI diagnostics
    bool testMidiLoopback() const;

    // Bridge diagnostics
    bool isBridgeActive() const;

    // Sysmode diagnostics
    bool isSysmodeActive() const;

    // Auto-start diagnostics
    bool isAutostartEnabled() const;

    // Generate full diagnostics report
    QString generateReport() const;

    // Get log file path
    QString getLogFilePath() const;
    QString getLogContent() const;
    QStringList getLastXruns() const;

    // Get card info
    QString getCardInfo() const;
    QString getAlsaConfigInfo() const;

private:
    Config& m_config;

    QString getJackPID() const;
    QString getJackVersion() const;

    QStringList parseXruns(const QString& logContent) const;
};

#endif // DIAGNOSTICS_H