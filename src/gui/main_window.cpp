#include "main_window.h"
#include "core/utils.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QCoreApplication>
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QDesktopServices>
#include <QFileInfo>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_config(Config::instance()), m_isDashboardOpen(false) {

    // Initialize core components
    m_jackManager = new JackManager(m_config, this);
    m_pwBridge = new PipeWireBridge(m_config, this);
    m_sysmode = new Sysmode(m_config, this);
    m_mixer = new Mixer(m_config, this);
    m_preset = new Preset(m_config, this);
    m_watchdog = new Watchdog(*m_jackManager, m_config, this);
    m_diagnostics = new Diagnostics(m_config, this);

    // Setup UI
    setupUI();
    createMenuBar();
    createStatusBar();
    connectSignals();

    // Auto-refresh dashboard and status widgets
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(3000);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshDashboard);
    m_refreshTimer->start();

    // Show dashboard on startup
    showDashboard();
}

MainWindow::~MainWindow() {
    // Cleanup
}

void MainWindow::setupUI() {
    // Main widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Stacked widget for different views
    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget);

    // Create widgets
    m_dashboardWidget = new DashboardWidget(this);
    m_mixerWidget = new MixerWidget(m_mixer, this);
    m_presetWidget = new PresetWidget(m_preset, this);
    m_configWidget = new ConfigWidget(m_config, this);
    m_infoWidget = new InfoWidget(m_diagnostics, this);
    m_docsWidget = new DocumentationWidget(this);
    m_diagWidget = new DiagnosticsWidget(m_diagnostics, this);

    // Add widgets to stacked widget
    m_stackedWidget->addWidget(m_dashboardWidget);
    m_stackedWidget->addWidget(m_mixerWidget);
    m_stackedWidget->addWidget(m_presetWidget);
    m_stackedWidget->addWidget(m_configWidget);
    m_stackedWidget->addWidget(m_infoWidget);
    m_stackedWidget->addWidget(m_docsWidget);
    m_stackedWidget->addWidget(m_diagWidget);
}

void MainWindow::createMenuBar() {
    QMenuBar* mb = menuBar();

    // File menu
    QMenu* fileMenu = mb->addMenu("File");

    m_actionDashboard = new QAction("Dashboard", this);
    connect(m_actionDashboard, &QAction::triggered, this, &MainWindow::showDashboard);
    fileMenu->addAction(m_actionDashboard);

    m_actionQuit = new QAction("Esci", this);
    connect(m_actionQuit, &QAction::triggered, this, &MainWindow::onActionQuit);
    fileMenu->addAction(m_actionQuit);

    // JACK menu
    QMenu* jackMenu = mb->addMenu("JACK");

    m_actionStart = new QAction("Avvia JACK", this);
    connect(m_actionStart, &QAction::triggered, this, &MainWindow::onActionStart);
    jackMenu->addAction(m_actionStart);

    m_actionStop = new QAction("Ferma JACK", this);
    connect(m_actionStop, &QAction::triggered, this, &MainWindow::onActionStop);
    jackMenu->addAction(m_actionStop);

    jackMenu->addSeparator();

    m_actionBridge = new QAction("Bridge PipeWire", this);
    connect(m_actionBridge, &QAction::triggered, this, &MainWindow::onActionBridge);
    jackMenu->addAction(m_actionBridge);

    m_actionSysmode = new QAction("Scheda di sistema", this);
    connect(m_actionSysmode, &QAction::triggered, this, &MainWindow::onActionSysmode);
    jackMenu->addAction(m_actionSysmode);

    // Mixer menu
    QMenu* mixerMenu = mb->addMenu("Mixer");

    m_actionMixer = new QAction("Apri mixer", this);
    connect(m_actionMixer, &QAction::triggered, this, &MainWindow::onActionMixer);
    mixerMenu->addAction(m_actionMixer);

    // Preset menu
    QMenu* presetMenu = mb->addMenu("Presets");

    m_actionPreset = new QAction("Apri preset", this);
    connect(m_actionPreset, &QAction::triggered, this, &MainWindow::onActionPreset);
    presetMenu->addAction(m_actionPreset);

    // Config menu
    QMenu* configMenu = mb->addMenu("Configurazione");

    m_actionConfig = new QAction("Configura JACK", this);
    connect(m_actionConfig, &QAction::triggered, this, &MainWindow::onActionConfig);
    configMenu->addAction(m_actionConfig);

    // Tools menu
    QMenu* toolsMenu = mb->addMenu("Strumenti");

    m_actionWatch = new QAction("Watchdog", this);
    connect(m_actionWatch, &QAction::triggered, this, &MainWindow::onActionWatch);
    toolsMenu->addAction(m_actionWatch);

    m_actionWatchStop = new QAction("Ferma watchdog", this);
    connect(m_actionWatchStop, &QAction::triggered, this, &MainWindow::onActionWatchStop);
    toolsMenu->addAction(m_actionWatchStop);

    toolsMenu->addSeparator();

    m_actionAutostart = new QAction("Avvio automatico", this);
    connect(m_actionAutostart, &QAction::triggered, this, &MainWindow::onActionAutostart);
    toolsMenu->addAction(m_actionAutostart);

    // Info menu
    QMenu* infoMenu = mb->addMenu("Informazioni");

    m_actionInfo = new QAction("Info scheda", this);
    connect(m_actionInfo, &QAction::triggered, this, &MainWindow::onActionInfo);
    infoMenu->addAction(m_actionInfo);

    m_actionMidi = new QAction("Info MIDI", this);
    connect(m_actionMidi, &QAction::triggered, this, &MainWindow::onActionMidi);
    infoMenu->addAction(m_actionMidi);

    m_actionMidiTest = new QAction("Prova MIDI", this);
    connect(m_actionMidiTest, &QAction::triggered, this, &MainWindow::onActionMidiTest);
    infoMenu->addAction(m_actionMidiTest);

    m_actionDocs = new QAction("Documentazione", this);
    connect(m_actionDocs, &QAction::triggered, this, &MainWindow::onActionDocs);
    infoMenu->addAction(m_actionDocs);

    m_actionDiag = new QAction("Diagnostica", this);
    connect(m_actionDiag, &QAction::triggered, this, &MainWindow::onActionDiag);
    infoMenu->addAction(m_actionDiag);

    infoMenu->addSeparator();

    m_actionAbout = new QAction("Informazioni su", this);
    connect(m_actionAbout, &QAction::triggered, this, &MainWindow::onActionAbout);
    infoMenu->addAction(m_actionAbout);

    // Debug menu
    QMenu* debugMenu = mb->addMenu("Debug");

    m_actionRestart = new QAction("Riavvia JACK", this);
    connect(m_actionRestart, &QAction::triggered, this, &MainWindow::onActionRestart);
    debugMenu->addAction(m_actionRestart);

    m_actionOpenLog = new QAction("Apri file di log", this);
    connect(m_actionOpenLog, &QAction::triggered, this, &MainWindow::onActionOpenLog);
    debugMenu->addAction(m_actionOpenLog);

    m_actionOpenAsound = new QAction("Apri configurazione ALSA", this);
    connect(m_actionOpenAsound, &QAction::triggered, this, &MainWindow::onActionOpenAsound);
    debugMenu->addAction(m_actionOpenAsound);

    m_actionOpenTerminal = new QAction("Apri terminale", this);
    connect(m_actionOpenTerminal, &QAction::triggered, this, &MainWindow::onActionOpenTerminal);
    debugMenu->addAction(m_actionOpenTerminal);
}

void MainWindow::createStatusBar() {
    m_statusLabel = new QLabel("Pronto", this);
    statusBar()->addWidget(m_statusLabel);
}

void MainWindow::connectSignals() {
    // Dashboard action dispatch
    connect(m_dashboardWidget, &DashboardWidget::actionRequested,
            this, &MainWindow::onActionRequested);

    // Jack signals
    connect(m_jackManager, &JackManager::started, this, &MainWindow::onJackStarted);
    connect(m_jackManager, &JackManager::stopped, this, &MainWindow::onJackStopped);
    connect(m_jackManager, &JackManager::errorOccurred, this, &MainWindow::onJackError);

    // Bridge signals
    connect(m_pwBridge, &PipeWireBridge::bridgeEnabled, this, &MainWindow::onBridgeEnabled);
    connect(m_pwBridge, &PipeWireBridge::bridgeDisabled, this, &MainWindow::onBridgeDisabled);
    connect(m_pwBridge, &PipeWireBridge::bridgeError, this, &MainWindow::onBridgeError);

    // Sysmode signals
    connect(m_sysmode, &Sysmode::sysmodeEnabled, this, &MainWindow::onSysmodeEnabled);
    connect(m_sysmode, &Sysmode::sysmodeDisabled, this, &MainWindow::onSysmodeDisabled);

    // Preset signals
    connect(m_preset, &Preset::presetApplied, this, &MainWindow::onPresetApplied);

    // Config: apply = save + restart JACK
    connect(m_configWidget, &ConfigWidget::applyClicked,
            this, &MainWindow::onActionRestart);

    // Watchdog signals
    connect(m_watchdog, &Watchdog::started, this, &MainWindow::onWatchdogStarted);
    connect(m_watchdog, &Watchdog::stopped, this, &MainWindow::onWatchdogStopped);
    connect(m_watchdog, &Watchdog::jackRestarted, this, &MainWindow::onWatchdogJackRestarted);
}

void MainWindow::showDashboard() {
    // Qt-native dashboard (converted from the old HTML Control Panel)
    m_dashboardWidget->show();
    updateStatusWidgets();
    m_stackedWidget->setCurrentWidget(m_dashboardWidget);
    m_isDashboardOpen = true;
    m_statusLabel->setText("Dashboard");
}

void MainWindow::refreshDashboard() {
    if (m_isDashboardOpen) {
        updateStatusWidgets();
    }
}

void MainWindow::onDashboardClosed() {
    m_isDashboardOpen = false;
}

void MainWindow::onActionStart() {
    QString sr = m_config.getSampleRate();
    QString buf = m_config.getBufferSize();
    QString per = m_config.getPeriods();

    if (!m_jackManager->start(sr, buf, per)) {
        QMessageBox::critical(this, "Errore",
            "Impossibile avviare JACK. Controlla il file di log.");
    }
}

void MainWindow::onActionStop() {
    m_jackManager->stop();
}

void MainWindow::onActionBridge() {
    m_pwBridge->toggle();
}

void MainWindow::onActionSysmode() {
    m_sysmode->toggle();
}

void MainWindow::onActionWatch() {
    if (m_watchdog->isRunning()) {
        m_watchdog->stop();
        QMessageBox::information(this, "Watchdog", "Watchdog fermato.");
    } else {
        m_watchdog->start();
        QMessageBox::information(this, "Watchdog", "Watchdog avviato in background.");
    }
}

void MainWindow::onActionWatchStop() {
    m_watchdog->stop();
    QMessageBox::information(this, "Watchdog", "Watchdog fermato.");
}

void MainWindow::onActionAutostart() {
    if (m_config.autostartEnabled()) {
        m_config.setAutostart(false);
        QMessageBox::information(this, "Avvio automatico", "Avvio automatico disabilitato.");
    } else {
        m_config.setAutostart(true);
        QMessageBox::information(this, "Avvio automatico", "Avvio automatico abilitato.");
    }
}

void MainWindow::onActionMixer() {
    if (!m_mixer->isSinkAvailable() && !m_mixer->isSourceAvailable()) {
        QMessageBox::warning(this, "Attenzione",
            "Nessun nodo Tascam attivo in PipeWire.\n\nAbilita 'Scheda di sistema' o 'Bridge' per usare il mixer.");
        return;
    }

    m_mixerWidget->show();
    m_stackedWidget->setCurrentWidget(m_mixerWidget);
    m_statusLabel->setText("Mixer");
}

void MainWindow::onActionPreset() {
    m_presetWidget->show();
    m_stackedWidget->setCurrentWidget(m_presetWidget);
    m_statusLabel->setText("Preset");
}

void MainWindow::onActionConfig() {
    m_configWidget->show();
    m_stackedWidget->setCurrentWidget(m_configWidget);
    m_statusLabel->setText("Configurazione");
}

void MainWindow::onActionInfo() {
    m_infoWidget->show();
    m_stackedWidget->setCurrentWidget(m_infoWidget);
    m_statusLabel->setText("Info scheda");
}

void MainWindow::onActionMidi() {
    m_infoWidget->show();
    m_stackedWidget->setCurrentWidget(m_infoWidget);
    m_infoWidget->showMidi();
    m_statusLabel->setText("Info MIDI");
}

void MainWindow::onActionMidiTest() {
    auto result = m_diagnostics->testMidiLoopback();
    if (result) {
        QMessageBox::information(this, "Prova MIDI",
            "Nota di prova inviata.\n\nSe hai un synth collegato all'INGRESSO sentirai un Do4.");
    } else {
        QMessageBox::critical(this, "Prova MIDI",
            "Invio della nota di prova alla porta MIDI non riuscito.");
    }
}

void MainWindow::onActionDocs() {
    m_docsWidget->show();
    m_stackedWidget->setCurrentWidget(m_docsWidget);
    m_statusLabel->setText("Documentazione");
}

void MainWindow::onActionDiag() {
    m_diagWidget->show();
    m_stackedWidget->setCurrentWidget(m_diagWidget);
    m_statusLabel->setText("Diagnostica");
}

void MainWindow::onActionAbout() {
    QMessageBox box(this);
    box.setWindowTitle("Informazioni su");
    box.setIconPixmap(QIcon(":/icons/tascam-us122l.png").pixmap(64, 64));
    box.setText(QString("<h3>Tascam US-122L Manager %1</h3>"
                        "<p>Strumento di controllo per l'interfaccia audio "
                        "Tascam US-122L su Linux.</p>"
                        "<p><b>Autore:</b> Bronco (bronco420)</p>"
                        "<p>Sviluppato da zero - nessun codice esterno convertito.</p>"
                        "<p><b>License:</b> MIT - https://github.com/bronco420"
                        "/tascam-us122l-manager</p>")
                    .arg(QCoreApplication::applicationVersion()));
    box.setInformativeText("Realizzato con:\n"
                           "- Qt " QT_VERSION_STR " (LGPL v3)\n"
                           "- JACK Audio Connection Kit\n"
                           "- PipeWire\n"
                           "- ALSA usb_stream\n\n"
                           "Le fotografie del prodotto e il logo Tascam sono di proprieta' "
                           "di TEAC Corporation / Tascam.");
    box.exec();
}

void MainWindow::onActionRestart() {
    m_jackManager->restart();
}

void MainWindow::onActionOpenLog() {
    QString logPath = m_config.getLogFilePath();
    QFileInfo fileInfo(logPath);
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
    } else {
        QMessageBox::warning(this, "File di log", "File di log non trovato.");
    }
}

void MainWindow::onActionOpenAsound() {
    QString asoundrcPath = m_config.getAsoundrcPath();
    QFileInfo fileInfo(asoundrcPath);
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(asoundrcPath));
    } else {
        QMessageBox::warning(this, "Configurazione ALSA", "File di configurazione non trovato.");
    }
}

void MainWindow::onActionOpenTerminal() {
    QString terminal = QProcessEnvironment::systemEnvironment().value("TERMINAL", "xterm");
    QProcess::startDetached(terminal, QStringList());
}

void MainWindow::onActionQuit() {
    if (m_watchdog->isRunning()) {
        m_watchdog->stop();
    }
    close();
}

void MainWindow::onJackStarted() {
    m_statusLabel->setText("JACK avviato");
}

void MainWindow::onJackStopped() {
    m_statusLabel->setText("JACK fermato");
}

void MainWindow::onJackError(const QString& error) {
    QMessageBox::critical(this, "Errore JACK", error);
    m_statusLabel->setText("Errore JACK");
}

void MainWindow::onBridgeEnabled() {
    m_statusLabel->setText("Bridge abilitato");
}

void MainWindow::onBridgeDisabled() {
    m_statusLabel->setText("Bridge disabilitato");
}

void MainWindow::onBridgeError(const QString& error) {
    QMessageBox::warning(this, "Bridge PipeWire", error);
    m_statusLabel->setText("Bridge: errore");
}

void MainWindow::onSysmodeEnabled() {
    m_statusLabel->setText("Scheda di sistema abilitata");
}

void MainWindow::onSysmodeDisabled() {
    m_statusLabel->setText("Scheda di sistema disabilitata");
}

void MainWindow::onPresetApplied(const QString& name) {
    m_statusLabel->setText("Preset applicato: " + name);
}

void MainWindow::onWatchdogStarted() {
    m_statusLabel->setText("Watchdog avviato");
}

void MainWindow::onWatchdogStopped() {
    m_statusLabel->setText("Watchdog fermato");
}

void MainWindow::onWatchdogJackRestarted() {
    m_statusLabel->setText("JACK riavviato dal watchdog");
}

// Dispatch dashboard actions to the core modules in-process.
// Preserves every feature the original program had.
void MainWindow::onActionRequested(const QString& action) {
    if (action == "start") {
        onActionStart();
    } else if (action == "stop") {
        onActionStop();
    } else if (action == "restart") {
        onActionRestart();
    } else if (action == "bridge") {
        onActionBridge();
    } else if (action == "sysmode") {
        onActionSysmode();
    } else if (action == "watch") {
        onActionWatch();
    } else if (action == "watch-stop") {
        onActionWatchStop();
    } else if (action == "autostart") {
        onActionAutostart();
    } else if (action == "vol-up") {
        m_mixer->adjustSinkVolume(5);
    } else if (action == "vol-down") {
        m_mixer->adjustSinkVolume(-5);
    } else if (action == "vol-mute") {
        m_mixer->toggleSinkMute();
    } else if (action == "src-up") {
        m_mixer->adjustSourceVolume(5);
    } else if (action == "src-down") {
        m_mixer->adjustSourceVolume(-5);
    } else if (action == "src-mute") {
        m_mixer->toggleSourceMute();
    } else if (action.startsWith("preset-")) {
        QString presetName = action.mid(7);
        m_preset->applyPreset(presetName);
    } else if (action == "mixer") {
        onActionMixer();
    } else if (action == "preset") {
        onActionPreset();
    } else if (action == "config") {
        onActionConfig();
    } else if (action == "info") {
        onActionInfo();
    } else if (action == "midi") {
        onActionMidi();
    } else if (action == "miditest") {
        onActionMidiTest();
    } else if (action == "docs") {
        onActionDocs();
    } else if (action == "diag") {
        onActionDiag();
    } else if (action == "refresh") {
        refreshDashboard();
    } else if (action == "quit") {
        onActionQuit();
    }
}

void MainWindow::updateStatusWidgets() {
    bool jackRunning = m_jackManager->isRunning();
    bool bridgeActive = m_pwBridge->isActive();
    bool sysmodeActive = m_sysmode->isActive();
    bool watchdogRunning = m_watchdog->isRunning();

    QString jackDetail;
    if (jackRunning) {
        jackDetail = QString("Frequenza: %1 Hz | Buffer: %2 frame")
                     .arg(m_jackManager->getSampleRate())
                     .arg(m_jackManager->getBufferSize());
    } else {
        jackDetail = QString("Frequenza: %1 Hz | Buffer: %2 frame")
                     .arg(m_config.getSampleRate())
                     .arg(m_config.getBufferSize());
    }

    QString midiClient = Utils::getMidiClient();
    if (midiClient.isEmpty()) {
        midiClient = "assente";
    }

    bool audioOn = jackRunning || sysmodeActive || bridgeActive;

    QString sinkVol = QString("%1%").arg(m_mixer->getSinkVolume());
    QString sourceVol = QString("%1%").arg(m_mixer->getSourceVolume());

    m_dashboardWidget->updateStatus(
        jackRunning, jackDetail,
        bridgeActive, sysmodeActive,
        watchdogRunning, m_config.autostartEnabled(),
        m_diagnostics->getCardModel(),
        m_jackManager->isDriverLoaded() ? "caricato" : "non caricato",
        m_diagnostics->getFirmwareVersion(),
        m_diagnostics->getUSBConnection(),
        midiClient, audioOn,
        sinkVol, sourceVol);
}