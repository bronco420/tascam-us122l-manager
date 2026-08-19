#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QEvent>
#include <QCoreApplication>

class JackManager;
class Config;

class Watchdog : public QObject {
    Q_OBJECT

public:
    explicit Watchdog(JackManager& jackManager, Config& config, QObject* parent = nullptr);
    ~Watchdog();

    // Status
    bool isRunning() const;
    bool isSelfRestart() const;
    void setSelfRestart(bool enabled);

    // Operations
    void start();
    void stop();
    void restart();

    // Event handling for application quit
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void started();
    void stopped();
    void jackRestarted();
    void sysmodeAutoRestore();

private:
    JackManager& m_jackManager;
    Config& m_config;
    QTimer* m_watchTimer;
    QTimer* m_backoffTimer;
    int m_restartCount;
    int m_failureCount;
    int m_backoffDelay;
    bool m_isRunning;
    bool m_isSelfRestart;
    QString m_pidFile;
    QString m_watchdogPidFile;

    void initialize();
    void startWatchLoop();
    void stopWatchLoop();
    void checkJackStatus();
    void restartJack();
    void increaseBackoff();
    void resetBackoff();
    void saveWatchdogPid();
    void removeWatchdogPid();
    void handleSysmodeAutoRestore();
};

#endif // WATCHDOG_H