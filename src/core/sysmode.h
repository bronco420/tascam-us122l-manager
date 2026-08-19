#ifndef SYSMODE_H
#define SYSMODE_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QProcess>

#include "core/config.h"

class Sysmode : public QObject {
    Q_OBJECT

public:
    explicit Sysmode(Config& config, QObject* parent = nullptr);
    ~Sysmode();

    // Status
    bool isActive() const;
    bool sourceExists() const;
    QString getSinkName() const;
    QString getSourceName() const;

    // Operations
    void enable();
    void disable();
    void toggle();

    // State management
    void savePreviousState();
    void restorePreviousState();

    // Module management
    QString loadSinkModule();
    QString loadSourceModule();
    void unloadSinkModule(const QString& moduleId);
    void unloadSourceModule(const QString& moduleId);

    // Persist module IDs across CLI invocations
    void saveModuleIds();
    void unloadModulesByName(const QString& needle);

    // Ensure ~/.asoundrc declares the usb_stream PCM with explicit
    // rate + period_size (required to avoid timeouts / no sound / no mic)
    void ensureUsbStreamConfig();

signals:
    void sysmodeEnabled();
    void sysmodeDisabled();

private:
    Config& m_config;
    QString m_previousSink;
    QString m_previousSource;
    QString m_previousProfile;
    QString m_sinkId;
    QString m_sourceId;

    QString getCardProfile() const;
    void setCardProfile(const QString& profile);
    bool waitForSysmodeActive(int timeoutMs = 4000);
};

#endif // SYSMODE_H