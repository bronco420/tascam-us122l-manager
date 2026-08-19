#include "mixer.h"
#include "config.h"
#include "utils.h"
#include <QDebug>
#include <QRegularExpression>

Mixer::Mixer(Config& config, QObject* parent)
    : QObject(parent), m_config(config) {
}

Mixer::~Mixer() {
    // Cleanup
}

bool Mixer::isSinkAvailable() const {
    return !getSinkName().isEmpty();
}

bool Mixer::isSourceAvailable() const {
    return !getSourceName().isEmpty();
}

int Mixer::getSinkVolume() const {
    QString sink = getSinkName();
    if (sink.isEmpty()) {
        return 0;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "sinks");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    int volume = 0;
    bool inSink = false;
    for (const QString& line : lines) {
        if (line.contains("Name:") && line.contains(sink)) {
            inSink = true;
        } else if (inSink && line.contains("front-left:")) {
            int vol = parseVolume(line);
            if (vol > 0) {
                volume = vol;
            }
            break;
        }
    }

    return volume;
}

int Mixer::getSourceVolume() const {
    QString source = getSourceName();
    if (source.isEmpty()) {
        return 0;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "sources");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    int volume = 0;
    bool inSource = false;
    for (const QString& line : lines) {
        if (line.contains("Name:") && line.contains(source)) {
            inSource = true;
        } else if (inSource && line.contains("front-left:")) {
            int vol = parseVolume(line);
            if (vol > 0) {
                volume = vol;
            }
            break;
        }
    }

    return volume;
}

bool Mixer::isSinkMuted() const {
    QString sink = getSinkName();
    if (sink.isEmpty()) {
        return false;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "sinks");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    bool inSink = false;
    bool muted = false;
    for (const QString& line : lines) {
        if (line.contains("Name:") && line.contains(sink)) {
            inSink = true;
        } else if (inSink && line.contains("Mute:")) {
            muted = parseMute(line);
            break;
        }
    }

    return muted;
}

bool Mixer::isSourceMuted() const {
    QString source = getSourceName();
    if (source.isEmpty()) {
        return false;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "sources");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    bool inSource = false;
    bool muted = false;
    for (const QString& line : lines) {
        if (line.contains("Name:") && line.contains(source)) {
            inSource = true;
        } else if (inSource && line.contains("Mute:")) {
            muted = parseMute(line);
            break;
        }
    }

    return muted;
}

bool Mixer::setSinkVolume(int volume) {
    QString sink = getSinkName();
    if (sink.isEmpty()) {
        return false;
    }

    // Clamp volume to valid range
    volume = qBound(0, volume, 153);

    bool success = executePactl(QStringList() << "set-sink-volume" << sink << QString::number(volume) + "%");

    if (success) {
        emit sinkVolumeChanged(volume);
    }

    return success;
}

bool Mixer::setSourceVolume(int volume) {
    QString source = getSourceName();
    if (source.isEmpty()) {
        return false;
    }

    // Clamp volume to valid range
    volume = qBound(0, volume, 153);

    bool success = executePactl(QStringList() << "set-source-volume" << source << QString::number(volume) + "%");

    if (success) {
        emit sourceVolumeChanged(volume);
    }

    return success;
}

bool Mixer::adjustSinkVolume(int delta) {
    int current = getSinkVolume();
    int newVolume = current + delta;
    return setSinkVolume(newVolume);
}

bool Mixer::adjustSourceVolume(int delta) {
    int current = getSourceVolume();
    int newVolume = current + delta;
    return setSourceVolume(newVolume);
}

bool Mixer::toggleSinkMute() {
    QString sink = getSinkName();
    if (sink.isEmpty()) {
        return false;
    }

    bool success = executePactl(QStringList() << "set-sink-mute" << sink << "toggle");

    if (success) {
        bool muted = isSinkMuted();
        emit sinkMuteChanged(muted);
    }

    return success;
}

bool Mixer::toggleSourceMute() {
    QString source = getSourceName();
    if (source.isEmpty()) {
        return false;
    }

    bool success = executePactl(QStringList() << "set-source-mute" << source << "toggle");

    if (success) {
        bool muted = isSourceMuted();
        emit sourceMuteChanged(muted);
    }

    return success;
}

QString Mixer::getSinkName() const {
    return Utils::getMixerSink();
}

QString Mixer::getSourceName() const {
    return Utils::getMixerSource();
}

int Mixer::parseVolume(const QString& line) const {
    QRegularExpression re("\\d+%");
    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch()) {
        QString volStr = match.captured();
        return volStr.left(volStr.length() - 1).toInt();
    }
    return 0;
}

bool Mixer::parseMute(const QString& line) const {
    return line.trimmed().endsWith("yes") && line.contains("Mute:");
}

bool Mixer::executePactl(const QStringList& args, int timeoutMs) {
    QProcess pactl;
    pactl.start("pactl", args);
    bool success = pactl.waitForFinished(timeoutMs) && pactl.exitCode() == 0;

    if (!success) {
        QString error = pactl.readAllStandardError();
        if (!error.isEmpty()) {
            Utils::logError(QString("Pactl error: %1").arg(error));
        }
    }

    return success;
}