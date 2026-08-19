#include "preset.h"
#include "config.h"
#include "utils.h"
#include <QDebug>
#include <QThread>

Preset::Preset(Config& config, QObject* parent)
    : QObject(parent), m_config(config) {

    // Initialize settings if they don't exist
    QString settingsFile = m_config.getSettingsDir() + "/settings.conf";
    if (!QFile::exists(settingsFile)) {
        saveCurrentConfig();
    }
}

Preset::~Preset() {
    // Cleanup
}

QList<Preset::PresetConfig> Preset::getAvailablePresets() const {
    QList<PresetConfig> presets;

    PresetConfig studio;
    studio.name = "Studio";
    studio.sampleRate = "44100";
    studio.bufferSize = "256";
    studio.periods = "3";
    presets.append(studio);

    PresetConfig standard;
    standard.name = "Standard";
    standard.sampleRate = "48000";
    standard.bufferSize = "128";
    standard.periods = "2";
    presets.append(standard);

    PresetConfig live;
    live.name = "Live";
    live.sampleRate = "48000";
    live.bufferSize = "64";
    live.periods = "3";
    presets.append(live);

    PresetConfig hiRes;
    hiRes.name = "Hi-Res";
    hiRes.sampleRate = "96000";
    hiRes.bufferSize = "256";
    hiRes.periods = "3";
    presets.append(hiRes);

    return presets;
}

bool Preset::applyPreset(const QString& presetName) {
    if (!validatePreset(presetName)) {
        Utils::logError(QString("Unknown preset: %1").arg(presetName));
        return false;
    }

    PresetConfig preset;

    if (presetName == "studio") {
        preset.name = "Studio";
        preset.sampleRate = "44100";
        preset.bufferSize = "256";
        preset.periods = "3";
    } else if (presetName == "standard") {
        preset.name = "Standard";
        preset.sampleRate = "48000";
        preset.bufferSize = "128";
        preset.periods = "2";
    } else if (presetName == "live") {
        preset.name = "Live";
        preset.sampleRate = "48000";
        preset.bufferSize = "64";
        preset.periods = "3";
    } else if (presetName == "hi-ri" || presetName == "hi-res") {
        preset.name = "Hi-Res";
        preset.sampleRate = "96000";
        preset.bufferSize = "256";
        preset.periods = "3";
    }

    Utils::logInfo(QString("Applying preset '%1': %2Hz / %3 / %4 periods")
                   .arg(presetName).arg(preset.sampleRate)
                   .arg(preset.bufferSize).arg(preset.periods));

    // Save settings
    setSampleRate(preset.sampleRate);
    setBufferSize(preset.bufferSize);
    setPeriods(preset.periods);

    // Restart JACK if running
    if (Utils::isJackRunning()) {
        if (!restartJACKWithPreset(preset)) {
            Utils::logError("Failed to restart JACK with new preset");
            return false;
        }
    }

    Utils::logInfo(QString("Preset '%1' applied successfully").arg(presetName));
    emit presetApplied(presetName);

    return true;
}

QString Preset::getSampleRate() const {
    return m_config.getSampleRate();
}

void Preset::setSampleRate(const QString& sr) {
    m_config.setSampleRate(sr);
    saveCurrentConfig();
}

QString Preset::getBufferSize() const {
    return m_config.getBufferSize();
}

void Preset::setBufferSize(const QString& buf) {
    m_config.setBufferSize(buf);
    saveCurrentConfig();
}

QString Preset::getPeriods() const {
    return m_config.getPeriods();
}

void Preset::setPeriods(const QString& per) {
    m_config.setPeriods(per);
    saveCurrentConfig();
}

void Preset::saveCurrentConfig() {
    QString settingsFile = m_config.getSettingsDir() + "/settings.conf";
    QSettings settings(settingsFile, QSettings::IniFormat);

    settings.setValue("SAMPLE_RATE", m_config.getSampleRate());
    settings.setValue("BUFFER_SIZE", m_config.getBufferSize());
    settings.setValue("PERIODS", m_config.getPeriods());
    settings.sync();
}

void Preset::loadConfig(QString& sr, QString& buf, QString& per) const {
    sr = m_config.getSampleRate();
    buf = m_config.getBufferSize();
    per = m_config.getPeriods();
}

bool Preset::validatePreset(const QString& name) const {
    QString normalized = name.toLower();
    if (normalized == "hi-ri" || normalized == "hi-res") {
        return true;
    }
    QList<PresetConfig> presets = getAvailablePresets();
    for (const PresetConfig& preset : presets) {
        if (preset.name.toLower() == normalized) {
            return true;
        }
    }
    return false;
}

bool Preset::restartJACKWithPreset(const PresetConfig& preset) {
    // Stop JACK
    QProcess stopProcess;
    stopProcess.start("pkill", QStringList() << "-x" << "jackd");
    stopProcess.waitForFinished(1000);

    if (stopProcess.exitCode() != 0) {
        Utils::logWarn("Failed to stop JACK gracefully");
    }

    QThread::sleep(1);

    // Start JACK with new preset
    QString launcher = m_config.getLauncherPath();
    QString pidFile = m_config.getSettingsDir() + "/jack.pid";

    QStringList jackdArgs;
    jackdArgs << "-vR" << "-P50" << "-t20000" << "-S";
    jackdArgs << "-dalsa";
    jackdArgs << "-dusb_stream:0";
    jackdArgs << "-r" + preset.sampleRate;
    jackdArgs << "-p" + preset.bufferSize;
    jackdArgs << "-n" + preset.periods;

    // The launcher expects: --pidfile FILE -- jackd [args...]
    QStringList launcherArgs;
    launcherArgs << "--pidfile" << pidFile << "--" << "jackd" << jackdArgs;

    qint64 launcherPid = 0;
    bool started = QProcess::startDetached(
        "python3", QStringList() << launcher << launcherArgs, QString(), &launcherPid);

    if (!started) {
        Utils::logError("Failed to start JACK with new preset");
        return false;
    }

    // Wait for JACK to start
    for (int i = 0; i < 5; i++) {
        if (Utils::isJackRunning()) {
            Utils::logInfo("JACK restarted successfully with new preset");
            return true;
        }
        QThread::sleep(1);
    }

    Utils::logError("JACK failed to start with new preset");
    return false;
}