#ifndef PRESET_H
#define PRESET_H

#include <QObject>
#include <QString>
#include <QSettings>

class Config;

class Preset : public QObject {
    Q_OBJECT

public:
    explicit Preset(Config& config, QObject* parent = nullptr);
    ~Preset();

    // Preset definitions
    struct PresetConfig {
        QString name;
        QString sampleRate;
        QString bufferSize;
        QString periods;
    };

    // Get available presets
    QList<PresetConfig> getAvailablePresets() const;

    // Apply a preset
    bool applyPreset(const QString& presetName);

    // Get current config
    QString getSampleRate() const;
    void setSampleRate(const QString& sr);

    QString getBufferSize() const;
    void setBufferSize(const QString& buf);

    QString getPeriods() const;
    void setPeriods(const QString& per);

    // Save current config
    void saveCurrentConfig();

    // Load config
    void loadConfig(QString& sr, QString& buf, QString& per) const;

signals:
    void presetApplied(const QString& name);

private:
    Config& m_config;

    bool validatePreset(const QString& name) const;
    bool restartJACKWithPreset(const PresetConfig& preset);
};

#endif // PRESET_H