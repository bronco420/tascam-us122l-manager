#ifndef PIPEWIRE_BRIDGE_H
#define PIPEWIRE_BRIDGE_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QProcess>

#include "core/config.h"

class PipeWireBridge : public QObject {
    Q_OBJECT

public:
    explicit PipeWireBridge(Config& config, QObject* parent = nullptr);
    ~PipeWireBridge();

    // Status
    bool isActive() const;
    QString getSinkName() const;
    QString getSourceName() const;

    // Operations
    void enable();
    void disable();
    void toggle();

    // State management
    void savePreviousSink(const QString& sink);
    QString getPreviousSink() const;
    void clearPreviousSink();

    // Module management
    QString loadJackSinkModule();
    QString loadJackSourceModule();
    void unloadJackSinkModule(const QString& moduleId);
    void unloadJackSourceModule(const QString& moduleId);

signals:
    void bridgeEnabled();
    void bridgeDisabled();

private:
    Config& m_config;
    bool m_previousSinkSaved;
    QString m_previousSink;

    QString getJackSinkName() const;
    QString getJackSourceName() const;
    QString getJackModuleId(const QString& moduleName) const;
    bool waitForModuleLoad(int timeoutMs = 3000);
};

#endif // PIPEWIRE_BRIDGE_H