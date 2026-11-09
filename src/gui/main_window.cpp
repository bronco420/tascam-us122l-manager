#include "main_window.h"
#include "core/utils.h"
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QMouseEvent>
#include <QIcon>
#include <QDebug>
#include <QScrollArea>

// ──────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_config(Config::instance()) {

    // Core objects (parented to this for lifecycle)
    m_jackManager = new JackManager(m_config, this);
    m_pwBridge = new PipeWireBridge(m_config, this);
    m_sysmode = new Sysmode(m_config, this);
    m_mixer = new Mixer(m_config, this);
    m_preset = new Preset(m_config, this);
    m_watchdog = new Watchdog(*m_jackManager, m_config, this);
    m_diagnostics = new Diagnostics(m_config, this);

    // Task runner (async)
    m_taskRunner = new TaskRunner(this);

    // Setup UI
    setupUI();
    connectSignals();

    // Periodic refresh (3s) — only updates labels, doesn't block
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(3000);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshTimer);
    m_refreshTimer->start();

    // Load saved geometry
    loadGeometry();

    // Initial state sync
    updateDashboardState();
}

MainWindow::~MainWindow() {
    saveGeometry();
    if (m_watchdog && m_watchdog->isRunning()) {
        m_watchdog->stop();
    }
}

// ──────────────────────────────────────────────
// setupUI — new dashboard shell
// ──────────────────────────────────────────────
void MainWindow::setupUI() {
    // Menu bar (legacy, kept for compatibility)
    QMenuBar* mb = menuBar();

    // File menu
    QMenu* fileMenu = mb->addMenu("File");
    auto* aDash = fileMenu->addAction("Dashboard");
    connect(aDash, &QAction::triggered, this, [this]() {
        if (m_pages) m_pages->setCurrentWidget(m_pages->widget(0));  // dashboard page
    });
    fileMenu->addSeparator();
    auto* aQuit = fileMenu->addAction("Esci");
    connect(aQuit, &QAction::triggered, this, &MainWindow::close);

    // JACK menu
    QMenu* jackMenu = mb->addMenu("JACK");
    auto* aStart = jackMenu->addAction("Avvia JACK");
    connect(aStart, &QAction::triggered, this, &MainWindow::onStartClicked);
    auto* aStop = jackMenu->addAction("Ferma JACK");
    connect(aStop, &QAction::triggered, this, &MainWindow::onStopClicked);
    jackMenu->addSeparator();
    auto* aBridge = jackMenu->addAction("Bridge PipeWire");
    connect(aBridge, &QAction::triggered, this, &MainWindow::onBridgeClicked);
    auto* aSysmode = jackMenu->addAction("Scheda di sistema");
    connect(aSysmode, &QAction::triggered, this, &MainWindow::onSysmodeClicked);

    // Tools menu
    QMenu* toolsMenu = mb->addMenu("Strumenti");
    auto* aMixer = toolsMenu->addAction("Mixer");
    connect(aMixer, &QAction::triggered, this, &MainWindow::onMixerClicked);
    auto* aPreset = toolsMenu->addAction("Preset");
    connect(aPreset, &QAction::triggered, this, [this]() {
        if (m_pages) m_pages->setCurrentWidget(m_pages->widget(4));  // preset page index
    });
    auto* aConfig = toolsMenu->addAction("Configurazione");
    connect(aConfig, &QAction::triggered, this, [this]() {
        if (m_pages) m_pages->setCurrentWidget(m_pages->widget(5));  // config page
    });
    toolsMenu->addSeparator();
    auto* aWatch = toolsMenu->addAction("Watchdog");
    connect(aWatch, &QAction::triggered, this, &MainWindow::onWatchdogClicked);

    // Info menu
    QMenu* infoMenu = mb->addMenu("Informazioni");
    auto* aInfo = infoMenu->addAction("Info scheda");
    connect(aInfo, &QAction::triggered, this, [this]() {
        if (m_pages) m_pages->setCurrentWidget(m_pages->widget(6));  // info page
    });
    auto* aMidi = infoMenu->addAction("Info MIDI");
    connect(aMidi, &QAction::triggered, this, [this]() {
        if (m_pages) { m_pages->setCurrentWidget(m_pages->widget(6)); m_infoWidget->showMidi(); }
    });
    auto* aDocs = infoMenu->addAction("Documentazione");
    connect(aDocs, &QAction::triggered, this, [this]() {
        if (m_pages) m_pages->setCurrentWidget(m_pages->widget(7));  // docs page
    });
    auto* aDiag = infoMenu->addAction("Diagnostica");
    connect(aDiag, &QAction::triggered, this, [this]() {
        if (m_pages) m_pages->setCurrentWidget(m_pages->widget(8));  // diag page
    });

    // Debug menu
    QMenu* debugMenu = mb->addMenu("Debug");
    auto* aRestart = debugMenu->addAction("Riavvia JACK");
    connect(aRestart, &QAction::triggered, this, [this]() {
        m_jackManager->restart();
    });
    auto* aOpenLog = debugMenu->addAction("Apri file di log");
    connect(aOpenLog, &QAction::triggered, this, [this]() {
        QString logPath = m_config.getLogFilePath();
        if (QFile::exists(logPath)) QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
    });

    // Status bar
    statusBar()->show();  // keep Qt's native status bar at bottom

    // ── Central widget with stacked pages ──
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Stacked widget: page 0 = dashboard, pages 1+ = existing widgets
    m_pages = new QStackedWidget(this);
    mainLayout->addWidget(m_pages);

    // ── Page 0: Dashboard (new shell) ──
    QWidget* dashPage = new QWidget();
    auto* dashLayout = new QVBoxLayout(dashPage);
    dashLayout->setContentsMargins(0, 0, 0, 0);
    dashLayout->setSpacing(0);

    // Top bar
    m_topBar = new TopBar(dashPage);
    connect(m_topBar, &TopBar::trayRequested, this, [this]() {
        hide();
    });
    connect(m_topBar, &TopBar::miniModeRequested, this, &MainWindow::showMiniMode);
    dashLayout->addWidget(m_topBar);

    // Scrollable body
    auto* scrollArea = new QScrollArea(dashPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget* scrollContent = new QWidget(scrollArea);
    auto* bodyLayout = new QVBoxLayout(scrollContent);
    bodyLayout->setContentsMargins(16, 12, 16, 12);
    bodyLayout->setSpacing(16);

    // Chassis card (visual TASCAM panel)
    m_chassisCard = new ChassisCard(scrollContent);
    bodyLayout->addWidget(m_chassisCard);

    // Engine card (JACK/Bridge/Sysmode controls)
    m_engineCard = new EngineCard(scrollContent);
    connect(m_engineCard, &EngineCard::actionRequested, this, &MainWindow::onTransportAction);
    bodyLayout->addWidget(m_engineCard);

    // Software mixer card
    m_mixerCard = new SoftwareMixerCard(scrollContent);
    connect(m_mixerCard, &SoftwareMixerCard::actionRequested, this, &MainWindow::onTransportAction);
    bodyLayout->addWidget(m_mixerCard);

    scrollArea->setWidget(scrollContent);
    dashLayout->addWidget(scrollArea);

    // Transport bar (preset selector)
    m_transportBar = new TransportBar(dashPage);
    connect(m_transportBar, &TransportBar::presetSelected, this, &MainWindow::onPresetSelected);
    dashLayout->addWidget(m_transportBar);

    // Presets list
    auto availablePresets = m_preset->getAvailablePresets();
    QStringList presetNames;
    for (const auto& p : availablePresets) {
        presetNames << p.name;
    }
    m_transportBar->setPresets(presetNames);

    m_pages->addWidget(dashPage);

    // ── Pages 1-6: Existing widgets (incapsulated) ──
    m_mixerWidget = new MixerWidget(m_mixer, this);
    m_presetWidget = new PresetWidget(m_preset, this);
    m_configWidget = new ConfigWidget(m_config, this);
    m_infoWidget = new InfoWidget(m_diagnostics, this);
    m_docsWidget = new DocumentationWidget(this);
    m_diagWidget = new DiagnosticsWidget(m_diagnostics, this);

    // Wrap each in a container with "← Pannello" header for back navigation
    auto makeBackPage = [this](QWidget* page, const QString& title) -> QWidget* {
        QWidget* wrapper = new QWidget();
        auto* wLayout = new QVBoxLayout(wrapper);
        wLayout->setContentsMargins(0, 0, 0, 0);
        wLayout->setSpacing(0);

        // Back bar
        auto* backBar = new QWidget();
        auto* backLayout = new QHBoxLayout(backBar);
        backLayout->setContentsMargins(16, 8, 16, 8);
        backLayout->setSpacing(8);

        auto* backBtn = new QPushButton("← Pannello", backBar);
        backBtn->setStyleSheet("QPushButton { "
            "background-color: #1C2327; border: 1px solid #263136; "
            "border-radius: 4px; color: #A7B4BA; font-size: 12px; padding: 4px 12px; } "
            "QPushButton:hover { background-color: #232C31; color: #E8EEF1; }");
        connect(backBtn, &QPushButton::clicked, this, [this]() {
            if (m_pages) m_pages->setCurrentWidget(m_pages->widget(0));  // dashboard
        });
        backLayout->addWidget(backBtn);

        auto* titleLabel = new QLabel(title, backBar);
        titleLabel->setFont(UiTheme::fontLabel());
        titleLabel->setStyleSheet("color: #E8EEF1; font-size: 14px;");
        backLayout->addWidget(titleLabel);
        backLayout->addStretch();

        wLayout->addWidget(backBar);
        wLayout->addWidget(page);
        return wrapper;
    };

    m_pages->addWidget(makeBackPage(m_mixerWidget, "Mixer"));
    m_pages->addWidget(makeBackPage(m_presetWidget, "Preset"));
    m_pages->addWidget(makeBackPage(m_configWidget, "Configurazione JACK"));
    m_pages->addWidget(makeBackPage(m_infoWidget, "Info Scheda"));
    m_pages->addWidget(makeBackPage(m_docsWidget, "Documentazione"));
    m_pages->addWidget(makeBackPage(m_diagWidget, "Diagnostica"));

    // ── Drawer (overlay) ──
    m_drawer = new Drawer(this);
    m_drawer->addSection("AUDIO", {"Mixer", "Preset"});
    m_drawer->addSection("SISTEMA", {"Configurazione JACK", "Watchdog", "Bridge PipeWire", "Scheda di sistema"});
    m_drawer->addSection("AIUTO", {"Info Scheda", "Info MIDI", "Documentazione", "Diagnostica"});
    connect(m_drawer, &Drawer::itemSelected, this, [this](const QString& action) {
        if (action == "Mixer") onMixerClicked();
        else if (action == "Preset") { if (m_pages) m_pages->setCurrentWidget(m_pages->widget(1)); }
        else if (action == "Configurazione JACK") { if (m_pages) m_pages->setCurrentWidget(m_pages->widget(2)); }
        else if (action == "Info Scheda") { if (m_pages) m_pages->setCurrentWidget(m_pages->widget(3)); }
        else if (action == "Info MIDI") { if (m_pages) { m_pages->setCurrentWidget(m_pages->widget(3)); m_infoWidget->showMidi(); } }
        else if (action == "Documentazione") { if (m_pages) m_pages->setCurrentWidget(m_pages->widget(4)); }
        else if (action == "Diagnostica") { if (m_pages) m_pages->setCurrentWidget(m_pages->widget(5)); }
        else if (action == "Bridge PipeWire") onBridgeClicked();
        else if (action == "Scheda di sistema") onSysmodeClicked();
        else if (action == "Watchdog") onWatchdogClicked();
    });

    // ── System tray icon ──
    m_trayIcon = new QSystemTrayIcon(this);
    QString trayIconPath = ":/icons/tascam-us122l.png";
    if (QFile::exists(trayIconPath)) {
        m_trayIcon->setIcon(QIcon(trayIconPath));
    } else {
        m_trayIcon->setIcon(QIcon::fromTheme("audio-card", QIcon()));
    }
    m_trayIcon->setToolTip("Tascam US-122L Manager");

    m_trayMenu = new QMenu();
    auto* showAction = m_trayMenu->addAction("Mostra Pannello");
    connect(showAction, &QAction::triggered, this, &MainWindow::onTrayShow);
    auto* miniAction = m_trayMenu->addAction("Modalità Mini DAW");
    connect(miniAction, &QAction::triggered, this, &MainWindow::onTrayMiniMode);
    m_trayMenu->addSeparator();
    auto* quitAction = m_trayMenu->addAction("Esci");
    connect(quitAction, &QAction::triggered, this, &MainWindow::onTrayQuit);
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayActivated);
}

// ──────────────────────────────────────────────
// connectSignals
// ──────────────────────────────────────────────
void MainWindow::connectSignals() {
    // JACK signals
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

    // Watchdog signals
    connect(m_watchdog, &Watchdog::started, this, &MainWindow::onWatchdogStarted);
    connect(m_watchdog, &Watchdog::stopped, this, &MainWindow::onWatchdogStopped);
    connect(m_watchdog, &Watchdog::jackRestarted, this, &MainWindow::onWatchdogJackRestarted);

    // Mixer signals
    connect(m_mixer, &Mixer::sinkVolumeChanged, this, [this](int v) {
        updateDashboardState();
    });
    connect(m_mixer, &Mixer::sourceVolumeChanged, this, [this](int v) {
        updateDashboardState();
    });
    connect(m_mixer, &Mixer::sinkMuteChanged, this, [this](bool) {
        updateDashboardState();
    });
    connect(m_mixer, &Mixer::sourceMuteChanged, this, [this](bool) {
        updateDashboardState();
    });
}

// ──────────────────────────────────────────────
// Action handlers (async via TaskRunner)
// ──────────────────────────────────────────────
void MainWindow::onStartClicked() {
    m_taskRunner->run("start", [this]() {
        QString sr = m_config.getSampleRate();
        QString buf = m_config.getBufferSize();
        QString per = m_config.getPeriods();
        bool ok = m_jackManager->start(sr, buf, per);
        if (!ok) {
            QMetaObject::invokeMethod(this, [this]() {
                QMessageBox::critical(this, "Errore",
                    "Impossibile avviare JACK. Controlla il file di log.");
            });
        }
    }, [this]() {
        // done — state will refresh on next timer tick or signal
    });
}

void MainWindow::onStopClicked() {
    m_taskRunner->run("stop", [this]() {
        m_jackManager->stop();
    });
}

void MainWindow::onBridgeClicked() {
    m_taskRunner->run("bridge", [this]() {
        m_pwBridge->toggle();
    });
}

void MainWindow::onSysmodeClicked() {
    m_taskRunner->run("sysmode", [this]() {
        m_sysmode->toggle();
    });
}

void MainWindow::onWatchdogClicked() {
    if (m_watchdog->isRunning()) {
        m_taskRunner->run("watchdog-stop", [this]() {
            m_watchdog->stop();
        });
    } else {
        m_taskRunner->run("watchdog-start", [this]() {
            m_watchdog->start();
        });
    }
}

void MainWindow::onMixerClicked() {
    if (!m_mixer->isSinkAvailable() && !m_mixer->isSourceAvailable()) {
        QMessageBox::warning(this, "Attenzione",
            "Nessun nodo Tascam attivo in PipeWire.\n\n"
            "Abilita 'Scheda di sistema' o 'Bridge' per usare il mixer.");
        return;
    }
    if (m_pages) m_pages->setCurrentWidget(m_pages->widget(0));  // stay on dashboard, focus mixer card
}

void MainWindow::onPresetSelected(const QString& name) {
    m_taskRunner->run("preset-" + name, [this, name]() {
        m_preset->applyPreset(name);
    });
}

void MainWindow::onTransportAction(const QString& action) {
    if (action == "start") onStartClicked();
    else if (action == "stop") onStopClicked();
    else if (action == "bridge") onBridgeClicked();
    else if (action == "sysmode") onSysmodeClicked();
    else if (action == "watch") onWatchdogClicked();
    else if (action == "mixer") onMixerClicked();
}

// ──────────────────────────────────────────────
// Signal handlers (update UI)
// ──────────────────────────────────────────────
void MainWindow::onJackStarted() { updateDashboardState(); }
void MainWindow::onJackStopped() { updateDashboardState(); }
void MainWindow::onJackError(const QString& error) {
    QMessageBox::critical(this, "Errore JACK", error);
    updateDashboardState();
}

void MainWindow::onBridgeEnabled() { updateDashboardState(); }
void MainWindow::onBridgeDisabled() { updateDashboardState(); }
void MainWindow::onBridgeError(const QString& error) {
    QMessageBox::warning(this, "Bridge PipeWire", error);
    updateDashboardState();
}

void MainWindow::onSysmodeEnabled() { updateDashboardState(); }
void MainWindow::onSysmodeDisabled() { updateDashboardState(); }

void MainWindow::onPresetApplied(const QString& name) {
    if (m_transportBar) m_transportBar->setCurrentPreset(name);
    updateDashboardState();
}

void MainWindow::onWatchdogStarted() { updateDashboardState(); }
void MainWindow::onWatchdogStopped() { updateDashboardState(); }
void MainWindow::onWatchdogJackRestarted() { updateDashboardState(); }

// ──────────────────────────────────────────────
// Periodic refresh
// ──────────────────────────────────────────────
void MainWindow::onRefreshTimer() {
    updateDashboardState();
}

// ──────────────────────────────────────────────
// updateDashboardState — syncs core state → UI
// ──────────────────────────────────────────────
void MainWindow::updateDashboardState() {
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
        jackDetail = QString("Frequenza: %1 Hz | Buffer: %2 frame (config)")
                         .arg(m_config.getSampleRate())
                         .arg(m_config.getBufferSize());
    }

    int xrunCount = m_diagnostics->getXrunCount();
    bool audioOn = jackRunning || sysmodeActive || bridgeActive;

    // Update top bar
    if (m_topBar) {
        m_topBar->setJackStatus(jackRunning);
        m_topBar->setBridgeStatus(bridgeActive);
        m_topBar->setSysmodeStatus(sysmodeActive);
        m_topBar->setXrunCount(xrunCount);
    }

    // Update chassis card (visual)
    if (m_chassisCard) {
        m_chassisCard->setJackStatus(jackRunning);
        m_chassisCard->setBridgeStatus(bridgeActive);
        m_chassisCard->setSysmodeStatus(sysmodeActive);
        m_chassisCard->setXrunCount(xrunCount);
        m_chassisCard->setAudioOn(audioOn);
    }

    // Update engine card
    if (m_engineCard) {
        m_engineCard->updateEngineState(jackRunning, jackDetail,
                                         bridgeActive, sysmodeActive,
                                         watchdogRunning, xrunCount);
    }

    // Update mixer card
    if (m_mixerCard) {
        m_mixerCard->updateMixerState(
            m_mixer->getSinkVolume(),
            m_mixer->getSourceVolume(),
            m_mixer->isSinkMuted(),
            m_mixer->isSourceMuted());
    }

    // Update status bar
    QString statusText = "Pronto";
    if (jackRunning) statusText = "JACK attivo";
    else if (sysmodeActive) statusText = "Scheda di sistema attiva";
    else if (bridgeActive) statusText = "Bridge PipeWire attivo";
    statusBar()->showMessage(statusText, 3000);
}

// ──────────────────────────────────────────────
// Tray handlers
// ──────────────────────────────────────────────
void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        if (isHidden() || m_miniMode) {
            onTrayShow();
        }
    }
}

void MainWindow::onTrayShow() {
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::onTrayMiniMode() {
    showMiniMode();
}

void MainWindow::onTrayQuit() {
    close();
}

// ──────────────────────────────────────────────
// Mini DAW mode — compact horizontal bar
// ──────────────────────────────────────────────
void MainWindow::showMiniMode() {
    m_miniMode = true;
    setFixedSize(480, 56);
    setMinimumHeight(56);
    setMaximumHeight(56);

    // Hide everything except top bar with compact layout
    if (m_topBar) {
        m_topBar->setFixedHeight(56);
        m_topBar->layout()->itemAt(0)->widget()->setHidden(true);  // hide title
        m_topBar->setStyleSheet("background-color: #0E1113;");
    }

    // Hide body and transport, show status chips inline in top bar
    if (m_chassisCard) m_chassisCard->setHidden(true);
    if (m_engineCard) m_engineCard->setHidden(true);
    if (m_mixerCard) m_mixerCard->setHidden(true);
    if (m_transportBar) m_transportBar->setHidden(true);

    updateDashboardState();
}

void MainWindow::restoreNormalMode() {
    m_miniMode = false;
    setFixedSize(720, 640);
    setMinimumSize(720, 640);

    if (m_topBar) {
        m_topBar->setFixedHeight(48);
        m_topBar->layout()->itemAt(0)->widget()->setHidden(false);
    }
    if (m_chassisCard) m_chassisCard->setHidden(false);
    if (m_engineCard) m_engineCard->setHidden(false);
    if (m_mixerCard) m_mixerCard->setHidden(false);
    if (m_transportBar) m_transportBar->setHidden(false);

    updateDashboardState();
}

// ──────────────────────────────────────────────
// Geometry persistence
// ──────────────────────────────────────────────
void MainWindow::loadGeometry() {
    QSettings settings(m_config.getSettingsDir() + "/gui.conf", QSettings::IniFormat);
    QByteArray geo = settings.value("geometry").toByteArray();
    if (!geo.isEmpty()) {
        restoreGeometry(geo);
    } else {
        resize(720, 640);
    }
}

void MainWindow::saveGeometry() {
    QSettings settings(m_config.getSettingsDir() + "/gui.conf", QSettings::IniFormat);
    settings.setValue("geometry", QWidget::saveGeometry());
}

// ──────────────────────────────────────────────
// Close event
// ──────────────────────────────────────────────
void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_watchdog && m_watchdog->isRunning()) {
        m_watchdog->stop();
    }
    saveGeometry();
    hide();  // hide to tray instead of close
    event->ignore();
}
