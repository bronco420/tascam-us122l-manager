#include "sysmode.h"
#include "config.h"
#include "utils.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QThread>

Sysmode::Sysmode(Config& config, QObject* parent)
    : QObject(parent), m_config(config) {

    // Load previous state from config
    m_previousSink.clear();
    m_previousSource.clear();
    m_previousProfile = "output:analog-stereo+input:analog-stereo";

    // Load saved IDs from state
    QString stateFile = m_config.getStateFilePath();
    QFile stateFileObj(stateFile);
    if (stateFileObj.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&stateFileObj);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("SYSMODE_PREV_SINK=")) {
                m_previousSink = line.mid(16);
            } else if (line.startsWith("SYSMODE_PREV_SOURCE=")) {
                m_previousSource = line.mid(19);
            } else if (line.startsWith("SYSMODE_PREV_PCH_PROFILE=")) {
                m_previousProfile = line.mid(25);
            } else if (line.startsWith("SYSMODE_SINK_ID=")) {
                m_sinkId = line.mid(16);
            } else if (line.startsWith("SYSMODE_SOURCE_ID=")) {
                m_sourceId = line.mid(18);
            }
        }
        stateFileObj.close();
    }

    // Sanitize: state.conf may hold leftovers from a previous release or a
    // broken run (e.g. sink "auto_null", profile "off"). Restoring those
    // would leave the system with no working audio output at all.
    if (m_previousSink.isEmpty() || m_previousSink == "auto_null"
        || m_previousSink.contains("US122L")
        || m_previousSink.contains("jack", Qt::CaseInsensitive)
        || !sinkNamedExists(m_previousSink)) {
        m_previousSink.clear();
    }
    if (m_previousSource.isEmpty() || m_previousSource == "auto_null"
        || m_previousSource.contains("US122L")
        || !sourceNamedExists(m_previousSource)) {
        m_previousSource.clear();
    }
    // Profile names look like "output:analog-stereo+input:analog-stereo";
    // a bare "off" (or anything unknown) is not a usable restore target.
    if (m_previousProfile.trimmed() == "off"
        || !m_previousProfile.contains(':')) {
        m_previousProfile.clear();
    }
}

Sysmode::~Sysmode() {
    // Keep state alive: do not auto-disable on process exit.
    // (In CLI mode the process ends right after an action; disabling here
    //  would tear down sysmode immediately after enabling it.)
}

bool Sysmode::isActive() const {
    // Leftover modules count as active so toggle()/disable() can clean up.
    return sinkExists() || sourceExists();
}

bool Sysmode::sourceExists() const {
    return sourceNamedExists(getSourceName());
}

bool Sysmode::sinkExists() const {
    return sinkNamedExists(getSinkName());
}

static QStringList pactlShort(const QString& what) {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << what);
    pactl.waitForFinished(1500);
    return QString::fromUtf8(pactl.readAllStandardOutput())
        .split('\n', Qt::SkipEmptyParts);
}

bool Sysmode::sinkNamedExists(const QString& name) const {
    if (name.isEmpty()) {
        return false;
    }
    for (const QString& line : pactlShort("sinks")) {
        const QStringList parts = line.split('\t');
        if (parts.size() >= 2 && parts.value(1).trimmed() == name) {
            return true;
        }
    }
    return false;
}

bool Sysmode::sourceNamedExists(const QString& name) const {
    if (name.isEmpty()) {
        return false;
    }
    for (const QString& line : pactlShort("sources")) {
        const QStringList parts = line.split('\t');
        if (parts.size() >= 2 && parts.value(1).trimmed() == name) {
            return true;
        }
    }
    return false;
}

bool Sysmode::moduleLoadedFor(const QString& needle) const {
    for (const QString& line : pactlShort("modules")) {
        if (line.contains(needle)) {
            return true;
        }
    }
    return false;
}

bool Sysmode::isFunctional() const {
    return sinkNamedExists(getSinkName())
        && sourceNamedExists(getSourceName())
        && moduleLoadedFor("sink_name=US122L_Out")
        && moduleLoadedFor("source_name=US122L_In");
}

// Find a real (non-Tascam, non-dummy) sink to fall back to when the previously
// saved default sink is gone (e.g. "auto_null" after the PCH was disabled).
QString Sysmode::findFallbackSink() const {
    QString firstAny;
    for (const QString& line : pactlShort("sinks")) {
        const QStringList parts = line.split('\t');
        if (parts.size() < 2) {
            continue;
        }
        const QString name = parts.value(1).trimmed();
        if (name.isEmpty() || name.contains("US122L") || name == "auto_null"
            || name.contains("jack", Qt::CaseInsensitive)) {
            continue;
        }
        if (firstAny.isEmpty()) {
            firstAny = name;
        }
        // Prefer the analog PCH output.
        if (name.contains("analog-stereo")) {
            return name;
        }
    }
    return firstAny;
}

bool Sysmode::profileAvailable(const QString& profile) const {
    if (profile.isEmpty() || profile == "off") {
        return false;
    }
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "cards");
    pactl.waitForFinished(3000);

    const QStringList lines = QString::fromUtf8(pactl.readAllStandardOutput()).split('\n');
    bool inPch = false;
    for (const QString& line : lines) {
        if (line.contains("Name: alsa_card.pci-0000_00_1b.0")) {
            inPch = true;
            continue;
        }
        if (!inPch) {
            continue;
        }
        if (line.contains("Active Profile:")) {
            break; // end of the profile block for this card
        }
        // Profile lines look like: "\t\toutput:analog-stereo: ... (available: yes)"
        if (line.trimmed().startsWith(profile + ":")
            || line.trimmed().startsWith("\"" + profile + "\":")) {
            return line.contains("available: yes");
        }
    }
    return false;
}

// Pick an available output profile on the PCH card, preferring analog stereo.
QString Sysmode::findFallbackProfile() const {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "cards");
    pactl.waitForFinished(3000);

    const QStringList lines = QString::fromUtf8(pactl.readAllStandardOutput()).split('\n');
    bool inPch = false;
    QString firstAvailable;
    for (const QString& line : lines) {
        if (line.contains("Name: alsa_card.pci-0000_00_1b.0")) {
            inPch = true;
            continue;
        }
        if (!inPch) {
            continue;
        }
        if (line.contains("Active Profile:")) {
            break;
        }
        const QString trimmed = line.trimmed();
        if (!trimmed.contains("output:") || !trimmed.contains("available: yes")) {
            continue;
        }
        // Profile names themselves contain ':' (e.g.
        // "output:analog-stereo+input:analog-stereo"), so cut at the first
        // ": " (colon followed by space) which ends the name.
        const int sep = trimmed.indexOf(": ");
        if (sep <= 0) {
            continue;
        }
        const QString name = trimmed.left(sep);
        if (name.isEmpty()) {
            continue;
        }
        if (firstAvailable.isEmpty()) {
            firstAvailable = name;
        }
        if (name.contains("analog-stereo") && trimmed.contains("sinks: 1")) {
            return name;
        }
    }
    return firstAvailable.isEmpty()
        ? QString("output:analog-stereo+input:analog-stereo")
        : firstAvailable;
}

bool Sysmode::waitForSink(const QString& name, int timeoutMs) const {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (sinkNamedExists(name)) {
            return true;
        }
        QThread::msleep(100);
    }
    return false;
}

QString Sysmode::getSinkName() const {
    return "US122L_Out";
}

QString Sysmode::getSourceName() const {
    return "US122L_In";
}

void Sysmode::enable() {
    if (isActive()) {
        Utils::logWarn("Sysmode is already active");
        return;
    }

    // Check if JACK is running and stop it
    if (Utils::isJackRunning()) {
        Utils::logInfo("JACK is running, stopping it before enabling sysmode...");
        QProcess stopProcess;
        stopProcess.start("pkill", QStringList() << "-x" << "jackd");
        stopProcess.waitForFinished(1000);
        QThread::sleep(1);
    }

    Utils::logInfo("Enabling sysmode (Tascam as system card)...");

    // The usb_stream PCM requires explicit rate + period_size, otherwise the
    // sink/source time out (no sound, no mic). Make sure the config is right
    // before loading the ALSA modules.
    ensureUsbStreamConfig();

    // Save current state
    savePreviousState();

    // Capture whether a source was already present before we load modules,
    // so we can preserve its volume (anti-feedback) vs. using the safe default
    // for a freshly created source.
    const bool sourcePreExisted = sourceExists();

    // Load JACK sink module
    m_sinkId = loadSinkModule();
    if (m_sinkId.isEmpty()) {
        Utils::logError("Failed to load ALSA sink module");
        return;
    }

    // Load ALSA source module
    m_sourceId = loadSourceModule();
    if (m_sourceId.isEmpty()) {
        Utils::logError("Failed to load ALSA source module");
        unloadSinkModule(m_sinkId);
        return;
    }

    // Wait for modules to load
    if (!waitForSysmodeActive()) {
        Utils::logError("Failed to activate sysmode");
        unloadSinkModule(m_sinkId);
        unloadSourceModule(m_sourceId);
        return;
    }

    // Persist module IDs for subsequent CLI invocations
    saveModuleIds();

    // Disable PCH card
    setCardProfile("off");

    // Set Tascam as default
    QProcess pactl;
    pactl.start("pactl", QStringList() << "set-default-sink" << "US122L_Out");
    pactl.waitForFinished(3000);

    pactl.start("pactl", QStringList() << "set-default-source" << "US122L_In");
    pactl.waitForFinished(3000);

    // Unmute so audio actually comes out of the Tascam.
    pactl.start("pactl", QStringList() << "set-sink-mute" << "US122L_Out" << "0");
    pactl.waitForFinished(2000);
    pactl.start("pactl", QStringList() << "set-source-mute" << "US122L_In" << "0");
    pactl.waitForFinished(2000);
    pactl.start("pactl", QStringList() << "set-sink-volume" << "US122L_Out" << "100%");
    pactl.waitForFinished(2000);

    // Microphone at 100% gain + speakers at 100% volume → acoustic feedback
    // (howling/whistling). For a freshly created source, start at a safe
    // default level; if the source already existed, keep the user's volume.
    if (!sourcePreExisted) {
        const int defaultVol = m_config.getSourceVolumeDefault();
        pactl.start("pactl", QStringList()
            << "set-source-volume" << "US122L_In" << QString("%1%").arg(defaultVol));
        pactl.waitForFinished(2000);
    }

    Utils::logInfo("Sysmode enabled");
    emit sysmodeEnabled();
}

void Sysmode::ensureUsbStreamConfig() {
    QString asoundrcPath = m_config.getAsoundrcPath();

    QString existingData;
    QFile existing(asoundrcPath);
    if (existing.exists()) {
        if (existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
			existingData = QString::fromUtf8(existing.readAll());
            existing.close();
        }
        if (existingData.contains("period_size")
            && existingData.contains("rate 48000")) {
            return; // already complete
        }
    }

    if (!QFile::exists(asoundrcPath + ".bak")) {
        QFile::copy(asoundrcPath, asoundrcPath + ".bak");
    }

    Utils::logInfo("Repairing ~/.asoundrc: adding rate + period_size to usb_stream");

    // Build the required usb_stream block.
    QString usbStreamBlock;
    {
        QTextStream out(&usbStreamBlock);
        out << "# Tascam US-122L - Plugin usb_stream for JACK Audio Connection Kit\n";
        out << "# This plugin is REQUIRED for the US-122L on Linux.\n";
        out << "# The card does NOT work with standard PCM devices (hw:, default:)\n";
        out << "# rate and period_size MUST be explicit: without period_size the\n";
        out << "# usb_stream plugin times out on long streams (no sound, no mic).\n";
        out << "pcm.!usb_stream {\n";
        out << "    @args [ CARD ]\n";
        out << "    @args.CARD {\n";
        out << "        type string\n";
        out << "        default \"US122L\"\n";
        out << "    }\n";
        out << "    type usb_stream\n";
        out << "    card $CARD\n";
        out << "    rate 48000\n";
        out << "    period_size 128\n";
        out << "}\n";
        out << "\n";
        out << "ctl.!usb_stream {\n";
        out << "    @args [ CARD ]\n";
        out << "    @args.CARD {\n";
        out << "        type string\n";
        out << "        default \"US122L\"\n";
        out << "    }\n";
        out << "    type hw\n";
        out << "    card $CARD\n";
        out << "}\n";
    }

    // Append (never clobber) so a custom ~/.asoundrc is preserved.
    QFile file(asoundrcPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        if (!existingData.isEmpty() && !existingData.endsWith("\n")) {
            out << "\n";
        }
        out << usbStreamBlock;
        file.close();
    }
}

void Sysmode::disable() {
    if (!isActive()) {
        Utils::logWarn("Sysmode is not active");
        return;
    }

    Utils::logInfo("Disabling sysmode...");

    // Unload modules (saved ID first, then fall back to name discovery)
    if (!m_sinkId.isEmpty()) {
        unloadSinkModule(m_sinkId);
    } else {
        unloadModulesByName("US122L_Out");
    }
    if (!m_sourceId.isEmpty()) {
        unloadSourceModule(m_sourceId);
    } else {
        unloadModulesByName("US122L_In");
    }

    // Restore PCH card profile
    restorePreviousState();

    // Clear saved state
    m_sinkId.clear();
    m_sourceId.clear();

    QMap<QString, QString> updates;
    updates["SYSMODE_SINK_ID"] = QString();
    updates["SYSMODE_SOURCE_ID"] = QString();
    Utils::updateStateFile(m_config.getStateFilePath(), updates);

    Utils::logInfo("Sysmode disabled");
    emit sysmodeDisabled();
}

void Sysmode::toggle() {
    if (isActive()) {
        disable();
    } else {
        enable();
    }
}

void Sysmode::savePreviousState() {
    // Get current default sink and source
    QProcess pactl;
    pactl.start("pactl", QStringList() << "get-default-sink");
    pactl.waitForFinished(1000);
    QString currentSink = pactl.readAllStandardOutput().trimmed();

    pactl.start("pactl", QStringList() << "get-default-source");
    pactl.waitForFinished(1000);
    QString currentSource = pactl.readAllStandardOutput().trimmed();

    // Only persist a sink that actually exists and is not the Tascam itself:
    // "auto_null" (no hardware sink) would restore to silence.
    if (currentSink.isEmpty() || currentSink == "auto_null"
        || currentSink.contains("US122L") || !sinkNamedExists(currentSink)) {
        currentSink = findFallbackSink();
    }
    if (currentSource.isEmpty() || currentSource == "auto_null"
        || currentSource.contains("US122L")) {
        currentSource.clear();
    }

    // Get current PCH profile
    QString pchProfile = getCardProfile();
    if (pchProfile.trimmed() == "off" || !pchProfile.contains(':')) {
        pchProfile = findFallbackProfile();
    }

    // Remember in memory too (the process may keep running, e.g. GUI).
    m_previousSink = currentSink;
    m_previousSource = currentSource;
    m_previousProfile = pchProfile;

    // Save to state file (preserving all other entries)
    QString stateFile = m_config.getStateFilePath();
    QMap<QString, QString> updates;
    updates["SYSMODE_PREV_SINK"] = currentSink;
    updates["SYSMODE_PREV_SOURCE"] = currentSource;
    updates["SYSMODE_PREV_PCH_PROFILE"] = pchProfile;
    updates["SYSMODE_SINK_ID"] = m_sinkId;
    updates["SYSMODE_SOURCE_ID"] = m_sourceId;
    Utils::updateStateFile(stateFile, updates);
}

void Sysmode::restorePreviousState() {
    // Restore PCH card profile first: switching the profile is what recreates
    // the PCH sinks, so the sink we restore afterwards actually exists.
    QString profile = m_previousProfile;
    if (profile.isEmpty() || profile.trimmed() == "off"
        || !profileAvailable(profile)) {
        profile = findFallbackProfile();
    }
    if (!profile.isEmpty()) {
        setCardProfile(profile);
    }

    // Restore previous sink, falling back to any real hardware sink.
    QString sink = m_previousSink;
    if (sink.isEmpty() || !waitForSink(sink, 1500)) {
        sink = findFallbackSink();
    }
    if (!sink.isEmpty()) {
        QProcess pactl;
        pactl.start("pactl", QStringList() << "set-default-sink" << sink);
        pactl.waitForFinished(3000);
        Utils::logInfo("Default sink ripristinato: " + sink);
    } else {
        Utils::logWarn("Nessun sink disponibile da ripristinare: "
                       "l'audio di sistema restera' muto finche' non viene "
                       "selezionata manualmente un'uscita.");
    }

    if (!m_previousSource.isEmpty() && sourceNamedExists(m_previousSource)) {
        QProcess pactl;
        pactl.start("pactl", QStringList() << "set-default-source" << m_previousSource);
        pactl.waitForFinished(3000);
    }
}

QString Sysmode::getCardProfile() const {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "cards");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput();
    QStringList lines = output.split('\n');

    QString pchProfile;
    bool inPchCard = false;

    for (const QString& line : lines) {
        if (line.contains("Name: alsa_card.pci-0000_00_1b.0")) {
            inPchCard = true;
        } else if (inPchCard && line.contains("Active Profile:")) {
            pchProfile = line.split(':').last().trimmed();
            break;
        }
    }

    return pchProfile;
}

void Sysmode::setCardProfile(const QString& profile) {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "set-card-profile" << "alsa_card.pci-0000_00_1b.0" << profile);
    pactl.waitForFinished(3000);

    if (pactl.exitCode() != 0) {
        Utils::logWarn(QString("Failed to set card profile: %1").arg(profile));
    }
}

bool Sysmode::waitForSysmodeActive(int timeoutMs) {
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        if (isActive()) {
            return true;
        }
        QThread::msleep(200);
    }

    return false;
}

// pactl load-module returns either the bare numeric id ("536870916") or a
// "module-536870916" string depending on the server. Normalize both forms.
static QString buildModuleId(const QString& output) {
    QString id = output.trimmed();
    if (id.startsWith("module-")) {
        return id;
    }
    // Bare numeric id (PipeWire's pulse server)
    if (!id.isEmpty()) {
        return id;
    }
    return "";
}

QString Sysmode::loadSinkModule() {
    QProcess pactl;
    pactl.start("pactl", QStringList()
        << "load-module" << "module-alsa-sink"
        << "device=usb_stream"
        << "sink_name=US122L_Out"
        << "sink_properties=device.description=\"Tascam US-122L (Uscita)\"");
    pactl.waitForFinished(4000);

    QString output = pactl.readAllStandardOutput().trimmed();
    QString id = buildModuleId(output);
    if (!id.isEmpty()) {
        return id;
    }

    Utils::logError(QString("Failed to load ALSA sink module: %1").arg(output));
    return "";
}

QString Sysmode::loadSourceModule() {
    QProcess pactl;
    pactl.start("pactl", QStringList()
        << "load-module" << "module-alsa-source"
        << "device=usb_stream"
        << "source_name=US122L_In"
        << "source_properties=device.description=\"Tascam US-122L (Microfono)\"");
    pactl.waitForFinished(4000);

    QString output = pactl.readAllStandardOutput().trimmed();
    QString id = buildModuleId(output);
    if (!id.isEmpty()) {
        return id;
    }

    Utils::logError(QString("Failed to load ALSA source module: %1").arg(output));
    return "";
}

void Sysmode::unloadSinkModule(const QString& moduleId) {
    if (moduleId.isEmpty()) {
        return;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "unload-module" << moduleId);
    pactl.waitForFinished(4000);

    if (pactl.exitCode() != 0) {
        Utils::logWarn(QString("Failed to unload ALSA sink module: %1").arg(moduleId));
    }
}

void Sysmode::unloadSourceModule(const QString& moduleId) {
    if (moduleId.isEmpty()) {
        return;
    }

    QProcess pactl;
    pactl.start("pactl", QStringList() << "unload-module" << moduleId);
    pactl.waitForFinished(4000);

    if (pactl.exitCode() != 0) {
        Utils::logWarn(QString("Failed to unload ALSA source module: %1").arg(moduleId));
    }
}

void Sysmode::saveModuleIds() {
    QString stateFile = m_config.getStateFilePath();

    QMap<QString, QString> updates;
    updates["SYSMODE_SINK_ID"] = m_sinkId;
    updates["SYSMODE_SOURCE_ID"] = m_sourceId;
    Utils::updateStateFile(stateFile, updates);
    Utils::logInfo("Sysmode module IDs saved");
}

void Sysmode::unloadModulesByName(const QString& needle) {
    QProcess pactl;
    pactl.start("pactl", QStringList() << "list" << "short" << "modules");
    pactl.waitForFinished(3000);

    QString output = pactl.readAllStandardOutput();
    for (const QString& line : output.split('\n')) {
        if (line.contains(needle)) {
            QString id = line.split('\t').value(0).trimmed();
            if (!id.isEmpty()) {
                Utils::logInfo(QString("Unloading module %1 (%2)").arg(id).arg(needle));
                unloadSinkModule(id);
            }
        }
    }
}