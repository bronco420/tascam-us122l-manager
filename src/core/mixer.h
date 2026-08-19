#ifndef MIXER_H
#define MIXER_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QProcess>

class Config;

class Mixer : public QObject {
    Q_OBJECT

public:
    explicit Mixer(Config& config, QObject* parent = nullptr);
    ~Mixer();

    // Status
    bool isSinkAvailable() const;
    bool isSourceAvailable() const;
    int getSinkVolume() const;
    int getSourceVolume() const;
    bool isSinkMuted() const;
    bool isSourceMuted() const;

    // Volume operations
    bool setSinkVolume(int volume);
    bool setSourceVolume(int volume);
    bool adjustSinkVolume(int delta);
    bool adjustSourceVolume(int delta);

    // Mute operations
    bool toggleSinkMute();
    bool toggleSourceMute();

    // Get names
    QString getSinkName() const;
    QString getSourceName() const;

signals:
    void sinkVolumeChanged(int volume);
    void sourceVolumeChanged(int volume);
    void sinkMuteChanged(bool muted);
    void sourceMuteChanged(bool muted);

private:
    Config& m_config;

    int parseVolume(const QString& line) const;
    bool parseMute(const QString& line) const;
    bool executePactl(const QStringList& args, int timeoutMs = 4000);
};

#endif // MIXER_H