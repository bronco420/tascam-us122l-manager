#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QMap>

class Utils {
public:
    // Logging
    static void logInfo(const QString& message);
    static void logWarn(const QString& message);
    static void logError(const QString& message);

    // File operations
    static bool fileExists(const QString& path);
    static bool fileRemove(const QString& path);
    static bool directoryCreate(const QString& path);
    static bool directoryExists(const QString& path);
    static bool directoryRemove(const QString& path);
    // Update KEY=VALUE entries in a state file, preserving other lines.
    // An empty value removes the entry.
    static bool updateStateFile(const QString& path, const QMap<QString, QString>& updates);

    // Process operations
    static QString processExecute(const QString& program, const QStringList& args,
                                  int timeoutMs = 5000, bool silent = true);
    static QStringList getLivePids(const QString& processName);

    // String operations
    static QString trim(const QString& str);
    static QStringList split(const QString& str, const QString& delimiter);

    // Format a bcdDevice value (4 hex chars) as "hi.lo" (e.g. "0100" -> "1.00").
    static QString formatFirmwareBCD(const QString& bcd);

    // USB device detection
    static QString getUSBNode();
    static QString getUSBConnection();
    static QString getFirmwareVersion();
    static QString getDriverKernelVersion();

    // Tascam model detection
    static QString detectTascamModel();

    // MIDI operations
    static QString getMidiClient();
    static QString getMidiInfo();
    static bool testMidiLoopback();

    // ALSA operations
    static bool checkDriverLoaded();
    static bool checkAsoundrc();

    // JACK operations
    static bool isJackRunning();
    static QString getJackSamplerate();
    static QString getJackBuffer();

    // PipeWire operations
    static bool bridgeIsActive();
    static QString getBridgeSink();
    static QString getBridgeSource();

    // System mode operations
    static bool sysmodeIsActive();
    static QString getSysmodeSink();
    static QString getSysmodeSource();

    // Mixer operations
    static QString getMixerSink();
    static QString getMixerSource();
    static int getMixerSinkVolume();
    static int getMixerSourceVolume();
    static bool setMixerSinkVolume(int volume);
    static bool setMixerSourceVolume(int volume);
    static bool toggleMixerSinkMute();
    static bool toggleMixerSourceMute();

    // Presets
    static void applyPreset(const QString& presetName);

    // Diagnostics
    static QString generateDiagnosticsReport();
    static QString getCardModel();
    static QString getCardNumber();
    static QString getSampleWidth();
    static QString getMidiClientName();
};

#endif // UTILS_H