#ifndef JACK_MANAGER_H
#define JACK_MANAGER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QTimer>
#include <QSettings>
#include <QFile>
#include <QDir>

#include "core/config.h"

class JackManager : public QObject {
    Q_OBJECT

public:
    explicit JackManager(Config& config, QObject* parent = nullptr);
    ~JackManager();

    // Status
    bool isRunning() const;
    bool isDriverLoaded() const;
    QString getSampleRate() const;
    QString getBufferSize() const;
    QString getPeriods() const;
    QString getPID() const;

    // Operations
    bool start(const QString& sampleRate, const QString& bufferSize, const QString& periods);
    void stop();
    void restart();

    // Configuration
    void saveSettings(const QString& sr, const QString& buf, const QString& per);
    void loadSettings(QString& sr, QString& buf, QString& per);

signals:
    void started();
    void stopped();
    void errorOccurred(const QString& error);

private:
    Config& m_config;
    QProcess* m_jackProcess;
    QTimer* m_statusTimer;
    QString m_pidFile;
    QString m_logFile;
    bool m_isRunning;

    bool validateParams(const QString& sr, const QString& buf, const QString& per);
    bool checkCardDetected();
    void setupAlsaConfig();
    void cleanupOldJack();
    void rotateLog();
    bool waitForJackStart(int timeoutSeconds);
};

#endif // JACK_MANAGER_H