#include "utils.h"
#include "config.h"
#include <QDebug>
#include <QRegularExpression>
#include <QProcess>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QThread>
#include <QSysInfo>

static QString stateFilePath() {
    return Config::instance().getStateFilePath();
}

static QString asoundrcPath() {
    return Config::instance().getAsoundrcPath();
}

void Utils::logInfo(const QString& message) {
    qDebug() << "[INFO]" << message;
}

void Utils::logWarn(const QString& message) {
    qWarning() << "[WARN]" << message;
}

void Utils::logError(const QString& message) {
    qCritical() << "[ERR]" << message;
}

bool Utils::fileExists(const QString& path) {
    return QFile::exists(path);
}

bool Utils::fileRemove(const QString& path) {
    return QFile::remove(path);
}

bool Utils::directoryCreate(const QString& path) {
    return QDir().mkpath(path);
}

bool Utils::directoryExists(const QString& path) {
    return QDir(path).exists();
}

bool Utils::directoryRemove(const QString& path) {
    QDir dir(path);
    return dir.removeRecursively();
}

bool Utils::updateStateFile(const QString& path, const QMap<QString, QString>& updates) {
    QStringList lines;

    QFile inFile(path);
    if (inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&inFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            bool matched = false;
            for (auto it = updates.constBegin(); it != updates.constEnd(); ++it) {
                if (line.startsWith(it.key() + "=")) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                lines << line;
            }
        }
        inFile.close();
    }

    for (auto it = updates.constBegin(); it != updates.constEnd(); ++it) {
        if (!it.value().isEmpty()) {
            lines << it.key() + "=" + it.value();
        }
    }

    QFile outFile(path);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&outFile);
    for (const QString& line : lines) {
        out << line << "\n";
    }
    outFile.close();
    return true;
}

QString Utils::processExecute(const QString& program, const QStringList& args,
                              int timeoutMs, bool silent) {
    QProcess process;
    process.start(program, args);
    process.waitForFinished(timeoutMs);

    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    if (!silent && !error.isEmpty()) {
        qWarning() << "Process error:" << program << args << error;
    }

    return output.trimmed();
}

QStringList Utils::getLivePids(const QString& processName) {
    QStringList result;

    QProcess pgrep;
    pgrep.start("pgrep", QStringList() << "-x" << processName);
    if (!pgrep.waitForFinished(1000)) {
        return result;
    }

    QString output = pgrep.readAllStandardOutput().trimmed();
    if (output.isEmpty()) {
        return result;
    }

    for (const QString& pid : output.split('\n')) {
        // Exclude zombie (defunct) processes that still linger in the process table
        QFile statFile("/proc/" + pid + "/stat");
        if (statFile.open(QIODevice::ReadOnly)) {
            QByteArray stat = statFile.readAll();
            int close = stat.lastIndexOf(')');
            if (close >= 0 && close + 2 < stat.size() && stat.at(close + 2) == 'Z') {
                statFile.close();
                continue;
            }
            statFile.close();
        }
        result << pid;
    }

    return result;
}

QString Utils::trim(const QString& str) {
    return str.trimmed();
}

QStringList Utils::split(const QString& str, const QString& delimiter) {
    return str.split(delimiter, Qt::SkipEmptyParts);
}

QString Utils::getUSBNode() {
    for (const QString& d : QDir("/sys/bus/usb/devices/").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString path = "/sys/bus/usb/devices/" + d + "/";
        QFile vendorFile(path + "idVendor");
        QFile productFile(path + "idProduct");

        if (vendorFile.open(QIODevice::ReadOnly) && productFile.open(QIODevice::ReadOnly)) {
            QString vendor = vendorFile.readAll().trimmed();
            QString product = productFile.readAll().trimmed();

            if (vendor == "0644") {
                if (product == "800e" || product == "800f" || product == "8012") {
                    return path;
                }
            }
        }
    }
    return "";
}

QString Utils::getUSBConnection() {
    QString node = getUSBNode();
    if (node.isEmpty()) {
        return "USB 2.0";
    }

    QFile speedFile(node + "speed");
    if (speedFile.open(QIODevice::ReadOnly)) {
        QString speed = speedFile.readAll().trimmed();
        if (speed == "480") {
            return "USB 2.0 (480 Mb/s)";
        } else if (speed == "12") {
            return "USB 1.1 (12 Mb/s)";
        } else if (speed == "1.5") {
            return "USB 1.0 (1.5 Mb/s)";
        } else if (speed == "5000") {
            return "USB 3.0 (5 Gb/s)";
        }
    }

    return "USB unknown";
}

QString Utils::formatFirmwareBCD(const QString& bcd) {
    if (bcd.isEmpty()) {
        return "-";
    }
    // bcdDevice is Binary-Coded Decimal: each character is a decimal digit
    // (e.g. "0100" -> 01.00 -> "1.00", "0111" -> 01.11 -> "1.11").
    // Convert as base-10 digits, never hex ("11" is 11, not 17).
    if (bcd.length() == 4 && bcd.contains(QRegularExpression("^[0-9]{4}$"))) {
        int hi = bcd.mid(0, 2).toInt(); // "01" -> 1
        int lo = bcd.mid(2, 2).toInt(); // "11" -> 11
        return QString("%1.%2").arg(hi).arg(lo, 2, 10, QChar('0'));
    }
    return bcd;
}

QString Utils::getFirmwareVersion() {
    QString node = getUSBNode();
    if (node.isEmpty()) {
        return "-";
    }

    QFile bcdFile(node + "bcdDevice");
    if (bcdFile.open(QIODevice::ReadOnly)) {
        QString bcd = bcdFile.readAll().trimmed();
        if (!bcd.isEmpty()) {
            return formatFirmwareBCD(bcd);
        }
    }

    return "-";
}

QString Utils::getDriverKernelVersion() {
    QString mod = "snd_usb_us122l";
    QProcess modinfo;
    modinfo.start("modinfo", QStringList() << "-F" << "version" << mod);
    modinfo.waitForFinished(1000);

    QString version = modinfo.readAllStandardOutput().trimmed();
    if (!version.isEmpty()) {
        return version;
    }

    // Fallback: get version from description
    QProcess descInfo;
    descInfo.start("modinfo", QStringList() << "-F" << "description" << mod);
    descInfo.waitForFinished(1000);
    QString desc = descInfo.readAllStandardOutput().trimmed();

    QRegularExpression re("version ([0-9.]+)");
    QRegularExpressionMatch match = re.match(desc);
    if (match.hasMatch()) {
        return match.captured(1);
    }

    return "-";
}

QString Utils::detectTascamModel() {
    QString node = getUSBNode();
    if (node.isEmpty()) {
        return "Non rilevata";
    }

    QFile productFile(node + "idProduct");
    if (productFile.open(QIODevice::ReadOnly)) {
        QString product = productFile.readAll().trimmed();
        if (product == "800e") {
            return "US-122L";
        } else if (product == "800f") {
            return "US-144";
        } else if (product == "8012") {
            return "US-144 Pro";
        }
    }

    return "US-122L";
}

QString Utils::getMidiClient() {
    QProcess aconnect;
    aconnect.start("aconnect", QStringList() << "-l");
    aconnect.waitForFinished(1000);

    QString output = aconnect.readAllStandardOutput();
    QRegularExpression re("TASCAM|US-122L|US-144|US122L");
    QRegularExpressionMatchIterator it = re.globalMatch(output);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString clientName = match.captured();
        if (!clientName.isEmpty()) {
            return clientName;
        }
    }

    return "";
}

QString Utils::getMidiInfo() {
    QProcess aconnect;
    aconnect.start("aconnect", QStringList() << "-l");
    aconnect.waitForFinished(1500);

    QString output = aconnect.readAllStandardOutput();
    QRegularExpression re("client\\s+(\\d+):\\s+'([^']*)'");
    QRegularExpressionMatchIterator it = re.globalMatch(output);

    QString clientId;
    QString name;
    QStringList ports;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        const QString id = match.captured(1);
        const QString nm = match.captured(2);
        if (nm.contains("TASCAM", Qt::CaseInsensitive)
            || nm.contains("US-122L", Qt::CaseInsensitive)
            || nm.contains("US122L", Qt::CaseInsensitive)
            || nm.contains("US-144", Qt::CaseInsensitive)) {
            clientId = id;
            name = nm;
            // Collect ports owned by this client
            const QStringList lines = output.split('\n');
            for (int i = 0; i < lines.size(); ++i) {
                const QString& line = lines[i];
                if (line.trimmed().startsWith(clientId + ":")) {
                    QStringList parts = line.split('\t', Qt::SkipEmptyParts);
                    if (!parts.isEmpty()) {
                        ports << parts.first().trimmed();
                    }
                }
            }
            break;
        }
    }

    if (clientId.isEmpty()) {
        return "Nessun client MIDI TASCAM rilevato.";
    }

    QString result = QString("Client MIDI: %1 (client %2)\n\nPorte:").arg(name).arg(clientId);
    if (ports.isEmpty()) {
        result += "\n  (nessuna porta)";
    } else {
        for (const QString& port : ports) {
            result += "\n  " + port;
        }
    }
    return result;
}

bool Utils::testMidiLoopback() {
    QProcess amidi;
    amidi.start("amidi", QStringList() << "-l");
    amidi.waitForFinished(1000);

    QString output = amidi.readAllStandardOutput();
    QRegularExpression re("TASCAM|US-122L|US-144");
    QRegularExpressionMatchIterator it = re.globalMatch(output);

    QString hwport;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString line = match.captured();
        QStringList lines = output.split('\n');
        for (const QString& line : lines) {
            if (line.contains(match.captured())) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    hwport = parts[1];
                    break;
                }
            }
        }
    }

    if (hwport.isEmpty()) {
        return false;
    }

    // Test MIDI loopback
    QProcess noteOn;
    noteOn.start("amidi", QStringList() << "-p" << hwport << "-S" << "90 3c 64");
    noteOn.waitForFinished(1000);

    QProcess noteOff;
    noteOff.start("amidi", QStringList() << "-p" << hwport << "-S" << "80 3c 64");
    noteOff.waitForFinished(1000);

    return (noteOn.exitCode() == 0 && noteOff.exitCode() == 0);
}

bool Utils::checkDriverLoaded() {
    QFile modulesFile("/proc/modules");
    if (!modulesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString data = QString::fromUtf8(modulesFile.readAll());
    return data.contains("snd_usb_us122l");
}

bool Utils::checkAsoundrc() {
    QFile file(asoundrcPath());
    return file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text);
}

bool Utils::isJackRunning() {
    // Check PID file
    QString pidFile = Config::instance().getSettingsDir() + "/jack.pid";
    QFile pidFileObj(pidFile);
    if (pidFileObj.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&pidFileObj);
        QString pid = in.readLine().trimmed();

        if (!pid.isEmpty()) {
            if (getLivePids("jackd").contains(pid)) {
                return true;
            }
        }
        pidFileObj.close();
    }

    // Fallback: pgrep
    return !getLivePids("jackd").isEmpty();
}

QString Utils::getJackSamplerate() {
    QString sr = "48000";
    if (isJackRunning()) {
        sr = Config::instance().getSampleRate();
    }
    return sr;
}

QString Utils::getJackBuffer() {
    QString buf = "128";
    if (isJackRunning()) {
        buf = Config::instance().getBufferSize();
    }
    return buf;
}

bool Utils::bridgeIsActive() {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "modules");
    pactl.waitForFinished(1000);

    QString output = pactl.readAllStandardOutput();
    return output.contains("module-jack");
}

QString Utils::getBridgeSink() {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "sinks");
    pactl.waitForFinished(1000);

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

QString Utils::getBridgeSource() {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "sources");
    pactl.waitForFinished(1000);

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

bool Utils::sysmodeIsActive() {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "sinks");
    pactl.waitForFinished(1000);

    QString output = pactl.readAllStandardOutput();
    return output.contains("US122L_Out");
}

QString Utils::getSysmodeSink() {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "sinks");
    pactl.waitForFinished(1000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    for (const QString& line : lines) {
        if (line.contains("US122L_Out")) {
            QStringList parts = line.split('\t');
            if (parts.size() >= 2) {
                return parts[1];
            }
        }
    }

    return "";
}

QString Utils::getSysmodeSource() {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "sources");
    pactl.waitForFinished(1000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    for (const QString& line : lines) {
        if (line.contains("US122L_In")) {
            QStringList parts = line.split('\t');
            if (parts.size() >= 2) {
                return parts[1];
            }
        }
    }

    return "";
}

QString Utils::getMixerSink() {
    if (sysmodeIsActive()) {
        return getSysmodeSink();
    } else if (bridgeIsActive()) {
        return getBridgeSink();
    }
    return "";
}

QString Utils::getMixerSource() {
    if (sysmodeIsActive()) {
        return getSysmodeSource();
    } else if (bridgeIsActive()) {
        return getBridgeSource();
    }
    return "";
}

int Utils::getMixerSinkVolume() {
    QString sink = getMixerSink();
    if (sink.isEmpty()) {
        return 0;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "sinks");
    pactl.waitForFinished(5000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    int volume = 0;
    bool inSink = false;
    for (const QString& line : lines) {
        if (line.contains("Name:") && line.contains(sink)) {
            inSink = true;
        } else if (inSink && line.contains("front-left:")) {
            QRegularExpression re("\\d+%");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                QString volStr = match.captured();
                volume = volStr.left(volStr.length() - 1).toInt();
                break;
            }
        }
    }

    return volume;
}

int Utils::getMixerSourceVolume() {
    QString source = getMixerSource();
    if (source.isEmpty()) {
        return 0;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "sources");
    pactl.waitForFinished(5000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    int volume = 0;
    bool inSource = false;
    for (const QString& line : lines) {
        if (line.contains("Name:") && line.contains(source)) {
            inSource = true;
        } else if (inSource && line.contains("front-left:")) {
            QRegularExpression re("\\d+%");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                QString volStr = match.captured();
                volume = volStr.left(volStr.length() - 1).toInt();
                break;
            }
        }
    }

    return volume;
}

bool Utils::setMixerSinkVolume(int volume) {
    QString sink = getMixerSink();
    if (sink.isEmpty()) {
        return false;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "set-sink-volume" << sink << QString::number(volume) + "%");
    return pactl.waitForFinished(4000) && pactl.exitCode() == 0;
}

bool Utils::setMixerSourceVolume(int volume) {
    QString source = getMixerSource();
    if (source.isEmpty()) {
        return false;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "set-source-volume" << source << QString::number(volume) + "%");
    return pactl.waitForFinished(4000) && pactl.exitCode() == 0;
}

bool Utils::toggleMixerSinkMute() {
    QString sink = getMixerSink();
    if (sink.isEmpty()) {
        return false;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "set-sink-mute" << sink << "toggle");
    return pactl.waitForFinished(4000) && pactl.exitCode() == 0;
}

bool Utils::toggleMixerSourceMute() {
    QString source = getMixerSource();
    if (source.isEmpty()) {
        return false;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "set-source-mute" << source << "toggle");
    return pactl.waitForFinished(4000) && pactl.exitCode() == 0;
}

void Utils::applyPreset(const QString& presetName) {
    QString sr, buf, per;

    if (presetName == "studio") {
        sr = "44100";
        buf = "256";
        per = "3";
    } else if (presetName == "standard") {
        sr = "48000";
        buf = "128";
        per = "2";
    } else if (presetName == "live") {
        sr = "48000";
        buf = "64";
        per = "3";
    } else if (presetName == "hi-ri") {
        sr = "96000";
        buf = "256";
        per = "3";
    } else {
        return;
    }

    // Save settings
    QSettings settings(Config::instance().getSettingsDir() + "/settings.conf", QSettings::IniFormat);
    settings.setValue("SAMPLE_RATE", sr);
    settings.setValue("BUFFER_SIZE", buf);
    settings.setValue("PERIODS", per);
    settings.sync();

    Utils::logInfo(QString("Preset '%1' applied: %2Hz / %3 / %4 periods")
                   .arg(presetName).arg(sr).arg(buf).arg(per));

    // Restart JACK if running
    if (isJackRunning()) {
        QProcess stopProcess;
        stopProcess.start("pkill", QStringList() << "-x" << "jackd");
        stopProcess.waitForFinished(1000);
        QThread::sleep(1);

        QProcess startProcess;
        startProcess.start("jackd", QStringList()
                          << "-vR" << "-P50" << "-t20000" << "-S"
                          << "-dalsa" << "-dusb_stream:0"
                          << "-r" << sr
                          << "-p" << buf
                          << "-n" << per);
        startProcess.waitForStarted(5000);

        if (startProcess.exitCode() == 0) {
            Utils::logInfo("JACK restarted with new preset");
        }
    }
}

QString Utils::generateDiagnosticsReport() {
    QString report;
    report += "===== TASCAM DIAGNOSTICA =====\n\n";

    report += "--- Sistema ---\n";
    report += "Kernel:  " + QSysInfo::kernelVersion() + "\n";
    report += "Distro:  CachyOS (Arch Linux)\n";
    report += "GUI:     yad\n\n";

    report += "--- Dispositivo USB ---\n";
    report += "Modello: " + getCardModel() + "\n";
    report += "Card:    " + getCardNumber() + "\n";
    report += "USB:     " + getUSBConnection() + "\n";
    report += "Firmware: " + getFirmwareVersion() + "\n";
    report += "Sample:  24-bit\n\n";

    report += "--- ALSA ---\n";
    report += "Driver: " + QString(checkDriverLoaded() ? "loaded" : "NOT loaded") + "\n";
    report += "Asoundrc: " + QString(checkAsoundrc() ? "configured" : "NOT configured") + "\n\n";

    report += "--- JACK ---\n";
    report += "Status: " + QString(isJackRunning() ? "ACTIVE" : "INACTIVE") + "\n";
    report += "Sample Rate: " + getJackSamplerate() + " Hz\n";
    report += "Buffer Size: " + getJackBuffer() + " frames\n\n";

    report += "--- PipeWire ---\n";
    report += "Bridge: " + QString(bridgeIsActive() ? "active" : "inactive") + "\n";
    report += "Sysmode: " + QString(sysmodeIsActive() ? "active" : "inactive") + "\n\n";

    report += "--- MIDI ---\n";
    report += "Client: " + getMidiClient() + "\n";
    report += "Loopback test: " + QString(testMidiLoopback() ? "OK" : "FAIL") + "\n\n";

    return report;
}

QString Utils::getCardModel() {
    QString node = getUSBNode();
    if (node.isEmpty()) {
        return "Non rilevata";
    }

    QFile productFile(node + "idProduct");
    if (productFile.open(QIODevice::ReadOnly)) {
        QString product = productFile.readAll().trimmed();
        if (product == "800e") {
            return "US-122L";
        } else if (product == "800f") {
            return "US-144";
        } else if (product == "8012") {
            return "US-144 Pro";
        }
    }

    return "US-122L";
}

QString Utils::getCardNumber() {
    QFile cardsFile("/proc/asound/cards");
    if (!cardsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "-";
    }

    QString data = QString::fromUtf8(cardsFile.readAll());
    QRegularExpression re("^\\s*(\\d+)\\s+\\[\\s*(\\S+)\\s*\\]");
    const QStringList lines = data.split('\n');
    for (const QString& line : lines) {
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            QString name = match.captured(2);
            if (name.contains("US122L") || name.contains("US-144")) {
                return match.captured(1);
            }
        }
    }
    return "-";
}

QString Utils::getSampleWidth() {
    return "24-bit";
}

QString Utils::getMidiClientName() {
    return getMidiClient();
}