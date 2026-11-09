#ifndef PIPEWIRE_BRIDGE_H
#define PIPEWIRE_BRIDGE_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QProcess>

#include "core/config.h"

// Bridge JACK <-> PipeWire.
//
// Usa il modulo nativo di PipeWire `libpipewire-module-jack-tunnel`
// (pacchetto Arch `pipewire-jack-client`): crea un sink e una source
// PipeWire collegati al server JACK tramite libjack.
//
// I vecchi moduli PulseAudio `module-jack-sink`/`module-jack-source`
// NON sono supportati da pipewire-pulse e fallivano con
// "Failure: No such entity": sono stati rimossi.
//
// Il modulo viene caricato da un processo `pw-cli -m load-module`
// lasciato attivo: il modulo vive nel contesto di quel processo e i nodi
// vengono esportati nel daemon PipeWire. Per questo il bridge non appare
// in `pw-cli list-objects Module` del daemon e il rilascio avviene
// terminando il processo (PID salvato in state.conf come BRIDGE_PID).
class PipeWireBridge : public QObject {
    Q_OBJECT

public:
    explicit PipeWireBridge(Config& config, QObject* parent = nullptr);
    ~PipeWireBridge();

    // Status
    bool isActive() const;
    QString getSinkName() const;
    QString getSourceName() const;
    QString lastError() const { return m_lastError; }
    qint64 bridgePid() const { return m_bridgePid; }

    // Operations
    void enable();
    void disable();
    void toggle();

    // State management
    void savePreviousSink(const QString& sink);
    QString getPreviousSink() const;
    void clearPreviousSink();

signals:
    void bridgeEnabled();
    void bridgeDisabled();
    void bridgeError(const QString& message);

private:
    Config& m_config;
    bool m_previousSinkSaved;
    QString m_previousSink;
    qint64 m_bridgePid;
    mutable QString m_lastError;

    static QString tunnelModuleName();
    static QString tunnelSinkNodeName();
    static QString tunnelSourceNodeName();
    QString tunnelArgs() const;
    QString bridgeLogPath() const;

    bool tunnelModuleAvailable() const;
    bool jackRunning() const;
    QString findNodeByName(const QString& interface, const QString& name) const;

    bool loadJackTunnel(QString& error);
    void unloadJackTunnel();
    qint64 findBridgeProcessPid() const;
    void saveBridgePid(qint64 pid);
    void clearBridgePid();

    bool waitForSink(int timeoutMs = 8000) const;
    void waitForSinkGone(int timeoutMs = 3000) const;
};

#endif // PIPEWIRE_BRIDGE_H
