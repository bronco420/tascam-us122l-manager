#include "pipewire_bridge.h"
#include "config.h"
#include "utils.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>
#include <QThread>

PipeWireBridge::PipeWireBridge(Config& config, QObject* parent)
    : QObject(parent), m_config(config), m_previousSinkSaved(false) {

    // Load previous sink from state
    QString stateFile = m_config.getStateFilePath();
    QFile stateFileObj(stateFile);
    if (stateFileObj.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&stateFileObj);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("PREV_SINK=")) {
                m_previousSink = line.mid(10);
                m_previousSinkSaved = true;
                break;
            }
        }
        stateFileObj.close();
    }
}

PipeWireBridge::~PipeWireBridge() {
    // Keep bridge state alive: do not auto-disable on process exit.
}

bool PipeWireBridge::isActive() const {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "modules");
    pactl.waitForFinished(1000);

    QString output = pactl.readAllStandardOutput();
    return output.contains("module-jack");
}

QString PipeWireBridge::getSinkName() const {
    return getJackSinkName();
}

QString PipeWireBridge::getSourceName() const {
    return getJackSourceName();
}

void PipeWireBridge::enable() {
    if (isActive()) {
        Utils::logWarn("Bridge is already active");
        return;
    }

    Utils::logInfo("Enabling PipeWire-JACK bridge...");

    // Save current default sink
    QString currentSink;
    QProcess pactl;
    pactl.start("pactl", QStringList() << "get-default-sink");
    pactl.waitForFinished(1000);

    QString output = pactl.readAllStandardOutput().trimmed();
    QStringList parts = output.split(' ');
    if (parts.size() >= 1) {
        currentSink = parts[0];
        savePreviousSink(currentSink);
        Utils::logInfo(QString("Saved previous sink: %1").arg(currentSink));
    }

    // Load JACK sink module
    QString sinkId = loadJackSinkModule();
    if (sinkId.isEmpty()) {
        Utils::logError("Failed to load JACK sink module");
        return;
    }

    // Load JACK source module
    QString sourceId = loadJackSourceModule();
    if (sourceId.isEmpty()) {
        Utils::logError("Failed to load JACK source module");
        unloadJackSinkModule(sinkId);
        return;
    }

    // Set JACK sink as default
    QString jackSink = getJackSinkName();
    if (!jackSink.isEmpty()) {
        QProcess setSink;
        setSink.start("pactl", QStringList() << "set-default-sink" << jackSink);
        setSink.waitForFinished(3000);
    }

    Utils::logInfo("PipeWire-JACK bridge enabled");
    emit bridgeEnabled();
}

void PipeWireBridge::disable() {
    if (!isActive()) {
        Utils::logWarn("Bridge is not active");
        return;
    }

    Utils::logInfo("Disabling PipeWire-JACK bridge...");

    // Unload JACK modules (by module ID, not sink/source name)
    unloadJackSinkModule(getJackModuleId("module-jack-sink"));
    unloadJackSourceModule(getJackModuleId("module-jack-source"));

    // Restore previous sink
    QString prevSink = getPreviousSink();
    if (!prevSink.isEmpty()) {
        QProcess setSink;
        setSink.start("pactl", QStringList() << "set-default-sink" << prevSink);
        setSink.waitForFinished(3000);
        Utils::logInfo(QString("Restored previous sink: %1").arg(prevSink));
    }

    // Clear saved sink
    clearPreviousSink();

    Utils::logInfo("PipeWire-JACK bridge disabled");
    emit bridgeDisabled();
}

void PipeWireBridge::toggle() {
    if (isActive()) {
        disable();
    } else {
        enable();
    }
}

void PipeWireBridge::savePreviousSink(const QString& sink) {
    m_previousSink = sink;
    m_previousSinkSaved = true;

    QMap<QString, QString> updates;
    updates["PREV_SINK"] = sink;
    Utils::updateStateFile(m_config.getStateFilePath(), updates);
}

QString PipeWireBridge::getPreviousSink() const {
    return m_previousSink;
}

void PipeWireBridge::clearPreviousSink() {
    m_previousSinkSaved = false;
    m_previousSink.clear();

    QMap<QString, QString> updates;
    updates["PREV_SINK"] = QString();
    Utils::updateStateFile(m_config.getStateFilePath(), updates);
}

QString PipeWireBridge::getJackSinkName() const {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "sinks");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    for (const QString& line : lines) {
        if (line.contains("jack", Qt::CaseInsensitive)) {
            QStringList parts = line.split('\t');
            if (parts.size() >= 2) {
                return parts[1];
            }
        }
    }

    return "";
}

QString PipeWireBridge::getJackSourceName() const {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "sources");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    for (const QString& line : lines) {
        if (line.contains("jack", Qt::CaseInsensitive)) {
            QStringList parts = line.split('\t');
            if (parts.size() >= 2) {
                return parts[1];
            }
        }
    }

    return "";
}

QString PipeWireBridge::getJackModuleId(const QString& moduleName) const {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "modules");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput();
    for (const QString& line : output.split('\n')) {
        // Format: <module-N>\t<name>...
        if (line.contains(moduleName)) {
            QString id = line.split('\t').value(0).trimmed();
            if (!id.isEmpty()) {
                return id;
            }
        }
    }
    return "";
}

bool PipeWireBridge::waitForModuleLoad(int timeoutMs) {
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        if (isActive()) {
            return true;
        }
        QThread::msleep(100);
    }

    return false;
}

QString PipeWireBridge::loadJackSinkModule() {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "load-module" << "module-jack-sink" << "channels=2");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput().trimmed();
    QString id = output;
    if (id.startsWith("module-")) {
        id = id.mid(7);
    }
    if (!id.isEmpty() && pactl.exitCode() == 0) {
        return id;
    }

    Utils::logError(QString("Failed to load JACK sink module: %1").arg(output));
    return "";
}

QString PipeWireBridge::loadJackSourceModule() {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "load-module" << "module-jack-source" << "channels=2");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput().trimmed();
    QString id = output;
    if (id.startsWith("module-")) {
        id = id.mid(7);
    }
    if (!id.isEmpty() && pactl.exitCode() == 0) {
        return id;
    }

    Utils::logError(QString("Failed to load JACK source module: %1").arg(output));
    return "";
}

void PipeWireBridge::unloadJackSinkModule(const QString& moduleId) {
    if (moduleId.isEmpty()) {
        return;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "unload-module" << moduleId);
    pactl.waitForFinished(3000);

    if (pactl.exitCode() != 0) {
        Utils::logWarn(QString("Failed to unload JACK sink module: %1").arg(moduleId));
    }
}

void PipeWireBridge::unloadJackSourceModule(const QString& moduleId) {
    if (moduleId.isEmpty()) {
        return;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "unload-module" << moduleId);
    pactl.waitForFinished(3000);

    if (pactl.exitCode() != 0) {
        Utils::logWarn(QString("Failed to unload JACK source module: %1").arg(moduleId));
    }
}