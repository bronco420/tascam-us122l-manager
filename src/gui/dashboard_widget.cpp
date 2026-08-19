#include "dashboard_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QIcon>
#include <QStyle>
#include <QMouseEvent>
#include <QPixmap>

// ============================= ActionCard =============================

ActionCard::ActionCard(const QString& iconPath, const QString& title,
                       const QString& subtitle, QWidget* parent)
    : QFrame(parent) {
    setObjectName("actionCard");
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(58);

    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 10, 12, 10);
    lay->setSpacing(12);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setObjectName("cardIcon");
    m_iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    lay->addWidget(m_iconLabel);

    QVBoxLayout* textBox = new QVBoxLayout();
    textBox->setSpacing(2);
    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setObjectName("cardTitle");
    m_titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_subtitleLabel = new QLabel(subtitle, this);
    m_subtitleLabel->setObjectName("cardSubtitle");
    m_subtitleLabel->setWordWrap(true);
    m_subtitleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    textBox->addWidget(m_titleLabel);
    textBox->addWidget(m_subtitleLabel);
    lay->addLayout(textBox, 1);

    m_badgeLabel = new QLabel(this);
    m_badgeLabel->setObjectName("cardBadge");
    m_badgeLabel->setAlignment(Qt::AlignCenter);
    m_badgeLabel->setFixedWidth(34);
    m_badgeLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    lay->addWidget(m_badgeLabel, 0, Qt::AlignVCenter);

    setIconPath(iconPath);
}

void ActionCard::setIconPath(const QString& path) {
    if (path.isEmpty()) {
        m_iconLabel->setPixmap(QPixmap());
        m_iconLabel->setText("●");
        return;
    }
    QPixmap pm(path);
    if (!pm.isNull()) {
        m_iconLabel->setText(QString());
        m_iconLabel->setPixmap(pm.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_iconLabel->setPixmap(QPixmap());
        m_iconLabel->setText("●");
    }
}

void ActionCard::setAccent(const QString& hue) {
    setProperty("accent", hue);
    m_iconLabel->setProperty("accent", hue);
    m_badgeLabel->setProperty("accent", hue);
    style()->unpolish(this);
    style()->polish(this);
    m_iconLabel->style()->unpolish(m_iconLabel);
    m_iconLabel->style()->polish(m_iconLabel);
    m_badgeLabel->style()->unpolish(m_badgeLabel);
    m_badgeLabel->style()->polish(m_badgeLabel);
}

void ActionCard::setTitle(const QString& title) {
    m_titleLabel->setText(title);
}

void ActionCard::setSubtitle(const QString& subtitle) {
    m_subtitleLabel->setText(subtitle);
}

void ActionCard::setBadge(const QString& text, bool active) {
    m_badgeLabel->setText(text);
    m_badgeLabel->setProperty("active", active);
    m_badgeLabel->style()->unpolish(m_badgeLabel);
    m_badgeLabel->style()->polish(m_badgeLabel);
    m_badgeLabel->setVisible(!text.isEmpty());
}

void ActionCard::setBadgeVisible(bool visible) {
    m_badgeLabel->setVisible(visible);
}

void ActionCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        setProperty("pressed", true);
        style()->unpolish(this);
        style()->polish(this);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void ActionCard::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        bool emitClick = m_pressed && rect().contains(event->pos());
        m_pressed = false;
        setProperty("pressed", false);
        style()->unpolish(this);
        style()->polish(this);
        if (emitClick) {
            emit clicked();
        }
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void ActionCard::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton && !rect().contains(event->pos())) {
        m_pressed = false;
    }
    QFrame::mouseMoveEvent(event);
}

// ============================= DashboardWidget =============================

DashboardWidget::DashboardWidget(QWidget* parent)
    : QWidget(parent) {
    setWindowTitle("Tascam US-122L Manager");
    resize(1040, 680);

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- Masthead (like the HTML .masthead) ----
    QFrame* header = new QFrame(this);
    header->setObjectName("dashHeader");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 14, 20, 14);
    headerLayout->setSpacing(14);

    QLabel* logo = new QLabel(header);
    logo->setObjectName("brandLogo");
    QPixmap logoPm(":/icons/tascam-us122l.png");
    logo->setPixmap(logoPm.scaled(44, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setFixedSize(44, 44);
    headerLayout->addWidget(logo);

    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);
    QLabel* title = new QLabel("Tascam US-122L", header);
    title->setObjectName("appTitle");
    QLabel* sub = new QLabel("Control Panel  ·  Interface Audio USB 2.0", header);
    sub->setObjectName("appSub");
    titleBox->addWidget(title);
    titleBox->addWidget(sub);
    headerLayout->addLayout(titleBox, 1);

    QLabel* versionChip = new QLabel("v2.3.1", header);
    versionChip->setObjectName("versionChip");
    headerLayout->addWidget(versionChip);

    m_statusLabel = new QLabel("READY", header);
    m_statusLabel->setProperty("pill", "off");
    headerLayout->addWidget(m_statusLabel);

    rootLayout->addWidget(header);

    // Accent bar (like HTML accent line)
    QFrame* accent = new QFrame(this);
    accent->setObjectName("accentBar");
    accent->setFixedHeight(3);
    rootLayout->addWidget(accent);

    // ---- Body ----
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget* body = new QWidget(scroll);
    body->setStyleSheet("background: transparent;");
    QHBoxLayout* bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(18, 18, 18, 18);
    bodyLayout->setSpacing(16);

    // ============ LEFT PANEL (status + product + note) ============
    QVBoxLayout* leftCol = new QVBoxLayout();
    leftCol->setSpacing(14);

    // Card: Stato Scheda (with LED rows)
    QFrame* statusCard = new QFrame();
    statusCard->setObjectName("panelCard");
    QVBoxLayout* statusLay = new QVBoxLayout(statusCard);
    statusLay->setContentsMargins(14, 12, 14, 12);
    statusLay->setSpacing(4);

    QLabel* statusTitle = new QLabel("STATO SCHEDA", statusCard);
    statusTitle->setObjectName("panelCardTitle");
    statusLay->addWidget(statusTitle);

    auto addLedRow = [this](QVBoxLayout* lay, const QString& label, QLabel** value) {
        QWidget* row = new QWidget();
        row->setObjectName("ledRow");
        QHBoxLayout* r = new QHBoxLayout(row);
        r->setContentsMargins(0, 2, 0, 2);
        r->setSpacing(8);
        QLabel* key = new QLabel(label, row);
        key->setProperty("dim", true);
        r->addWidget(key);
        r->addStretch();
        QLabel* led = new QLabel(row);
        led->setProperty("led", "off");
        led->setAlignment(Qt::AlignCenter);
        *value = new QLabel("-", row);
        (*value)->setProperty("strong", true);
        r->addWidget(led);
        r->addWidget(*value);
        lay->addWidget(row);
        return led;
    };

    addLedRow(statusLay, "Dispositivo", &m_deviceLabel);
    m_ledDriver = addLedRow(statusLay, "Driver ALSA", &m_driverLabel);
    addLedRow(statusLay, "Firmware", &m_firmwareLabel);
    addLedRow(statusLay, "Bus", &m_usbLabel);
    m_ledMidi = addLedRow(statusLay, "MIDI", &m_midiLabel);
    m_ledJack = addLedRow(statusLay, "Audio", &m_audioLabel);
    leftCol->addWidget(statusCard);

    // Card: Product photo
    QFrame* productCard = new QFrame();
    productCard->setObjectName("panelCard");
    QVBoxLayout* productLay = new QVBoxLayout(productCard);
    productLay->setContentsMargins(12, 12, 12, 12);
    QLabel* productTitle = new QLabel("TASCAM US-122L", productCard);
    productTitle->setObjectName("panelCardTitle");
    productLay->addWidget(productTitle);
    QLabel* photo = new QLabel(productCard);
    QPixmap front(":/assets/product/us122l_front.jpg");
    if (!front.isNull()) {
        photo->setPixmap(front.scaled(300, 170, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        photo->setText("Immagine non disponibile");
        photo->setProperty("muted", true);
    }
    photo->setAlignment(Qt::AlignCenter);
    photo->setObjectName("productPhoto");
    productLay->addWidget(photo);
    leftCol->addWidget(productCard);

    // Card: Nota
    QFrame* noteCard = new QFrame();
    noteCard->setObjectName("panelCard");
    QVBoxLayout* noteLay = new QVBoxLayout(noteCard);
    noteLay->setContentsMargins(14, 12, 14, 12);
    QLabel* noteTitle = new QLabel("NOTA", noteCard);
    noteTitle->setObjectName("panelCardTitle");
    noteLay->addWidget(noteTitle);
    QLabel* note = new QLabel("La US-122L usa il driver ALSA usb_stream e richiede JACK. "
        "Massime prestazioni su porta USB 2.0.", noteCard);
    note->setWordWrap(true);
    note->setProperty("muted", true);
    noteLay->addWidget(note);
    leftCol->addWidget(noteCard);

    leftCol->addStretch();
    bodyLayout->addLayout(leftCol, 1);

    // ============ RIGHT PANEL (action cards) ============
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(10);

    // JACK bar (like HTML .jackbar)
    QFrame* jackBar = new QFrame();
    jackBar->setObjectName("jackBar");
    QHBoxLayout* jackBarLay = new QHBoxLayout(jackBar);
    jackBarLay->setContentsMargins(14, 8, 14, 8);
    QLabel* jl = new QLabel("JACK", jackBar);
    jl->setObjectName("jackLabel");
    jackBarLay->addWidget(jl);
    jackBarLay->addStretch();
    m_jackLabel = new QLabel("● OFF", jackBar);
    m_jackLabel->setObjectName("jackValue");
    m_jackLabel->setProperty("active", false);
    jackBarLay->addWidget(m_jackLabel);
    rightCol->addWidget(jackBar);

    m_jackDetailLabel = new QLabel("Sample Rate: - Hz | Buffer: -");
    m_jackDetailLabel->setProperty("muted", true);
    rightCol->addWidget(m_jackDetailLabel);

    auto makeActionCard = [this](const QString& icon, const QString& title,
                                 const QString& subtitle, const QString& action) {
        ActionCard* card = new ActionCard(icon, title, subtitle, this);
        connect(card, &ActionCard::clicked, this, [this, action]() {
            emit actionRequested(action);
        });
        return card;
    };

    // JACK controls
    m_cardStart = makeActionCard(":/icons/start.png", "Avvia JACK", "driver usb_stream · 48000 Hz", "start");
    m_cardStart->setAccent("green");
    m_cardStop = makeActionCard(":/icons/stop.png", "Ferma JACK", "non attivo", "stop");
    m_cardStop->setAccent("red");
    m_cardStop->setEnabled(false);
    rightCol->addWidget(m_cardStart);
    rightCol->addWidget(m_cardStop);

    // Bridge + Sysmode
    m_cardBridge = makeActionCard(":/icons/bridge.png", "Bridge PipeWire", "audio di sistema ↔ JACK", "bridge");
    m_cardBridge->setAccent("cyan");
    m_cardBridge->setEnabled(false);
    rightCol->addWidget(m_cardBridge);
    m_cardSysmode = makeActionCard(":/icons/sysmode.png", "Scheda di Sistema", "Usa Tascam come audio di sistema", "sysmode");
    m_cardSysmode->setAccent("indigo");
    rightCol->addWidget(m_cardSysmode);

    // Mixer inline (like HTML .acard.mixer)
    QFrame* mixerCard = new QFrame();
    mixerCard->setObjectName("actionCard");
    QVBoxLayout* mixerLay = new QVBoxLayout(mixerCard);
    mixerLay->setContentsMargins(12, 10, 12, 10);
    mixerLay->setSpacing(8);

    auto addMixerRow = [](QVBoxLayout* lay, const QString& name, QLabel** value,
                          QPushButton** down, QPushButton** up, QPushButton** mute, QWidget* parent) {
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(8);
        QLabel* key = new QLabel(name, parent);
        key->setProperty("strong", true);
        row->addWidget(key);
        *value = new QLabel("--", parent);
        (*value)->setProperty("muted", true);
        row->addWidget(*value);
        row->addStretch();
        *down = new QPushButton("-", parent);
        (*down)->setFixedSize(34, 28);
        (*down)->setProperty("type", "ghost");
        *up = new QPushButton("+", parent);
        (*up)->setFixedSize(34, 28);
        (*up)->setProperty("type", "ghost");
        *mute = new QPushButton("M", parent);
        (*mute)->setFixedSize(34, 28);
        (*mute)->setProperty("type", "ghost");
        row->addWidget(*down);
        row->addWidget(*up);
        row->addWidget(*mute);
        lay->addLayout(row);
    };

    addMixerRow(mixerLay, "Uscita", &m_mixerSinkLabel, &m_btnVolDown, &m_btnVolUp, &m_btnVolMute, this);
    addMixerRow(mixerLay, "Ingresso", &m_mixerSourceLabel, &m_btnSrcDown, &m_btnSrcUp, &m_btnSrcMute, this);
    m_btnVolDown->setToolTip("Abbassa volume di uscita");
    m_btnVolUp->setToolTip("Alza volume di uscita");
    m_btnVolMute->setToolTip("Mute di uscita");
    m_btnSrcDown->setToolTip("Abbassa volume di ingresso");
    m_btnSrcUp->setToolTip("Alza volume di ingresso");
    m_btnSrcMute->setToolTip("Mute di ingresso");
    connect(m_btnVolDown, &QPushButton::clicked, this, [this]() { emit actionRequested("vol-down"); });
    connect(m_btnVolUp, &QPushButton::clicked, this, [this]() { emit actionRequested("vol-up"); });
    connect(m_btnVolMute, &QPushButton::clicked, this, [this]() { emit actionRequested("vol-mute"); });
    connect(m_btnSrcDown, &QPushButton::clicked, this, [this]() { emit actionRequested("src-down"); });
    connect(m_btnSrcUp, &QPushButton::clicked, this, [this]() { emit actionRequested("src-up"); });
    connect(m_btnSrcMute, &QPushButton::clicked, this, [this]() { emit actionRequested("src-mute"); });
    rightCol->addWidget(mixerCard);

    // Presets (like HTML preset cards)
    QFrame* presetCard = new QFrame();
    presetCard->setObjectName("actionCard");
    QVBoxLayout* presetLay = new QVBoxLayout(presetCard);
    presetLay->setContentsMargins(12, 10, 12, 10);
    presetLay->setSpacing(6);
    QLabel* presetTitle = new QLabel("Preset Rapidi", presetCard);
    presetTitle->setObjectName("cardTitle");
    presetLay->addWidget(presetTitle);

    m_cardPresetStudio = makeActionCard(":/icons/config.png", "Preset Studio", "44.1 kHz · buffer 256", "preset-studio");
    m_cardPresetStudio->setAccent("blue");
    m_cardPresetStandard = makeActionCard(":/icons/config.png", "Preset Standard", "48 kHz · buffer 128", "preset-standard");
    m_cardPresetStandard->setAccent("teal");
    m_cardPresetLive = makeActionCard(":/icons/config.png", "Preset Live", "48 kHz · buffer 64", "preset-live");
    m_cardPresetLive->setAccent("amber");
    m_cardPresetHiRes = makeActionCard(":/icons/config.png", "Preset Hi-Res", "96 kHz · buffer 256", "preset-hi-res");
    m_cardPresetHiRes->setAccent("violet");
    presetLay->addWidget(m_cardPresetStudio);
    presetLay->addWidget(m_cardPresetStandard);
    presetLay->addWidget(m_cardPresetLive);
    presetLay->addWidget(m_cardPresetHiRes);
    rightCol->addWidget(presetCard);

    // Watchdog + Autostart
    m_cardWatch = makeActionCard(":/icons/watch.png", "Watchdog JACK", "riavvio automatico se cade", "watch");
    m_cardWatch->setAccent("amber");
    m_cardAutostart = makeActionCard(":/icons/autostart.png", "Auto-start", "JACK al login", "autostart");
    m_cardAutostart->setAccent("violet");
    rightCol->addWidget(m_cardWatch);
    rightCol->addWidget(m_cardAutostart);

    // Configura
    m_cardConfig = makeActionCard(":/icons/config.png", "Configura", "sample rate / buffer", "config");
    m_cardConfig->setAccent("blue");
    rightCol->addWidget(m_cardConfig);

    // Tools grid (2 cols) like HTML
    QFrame* toolsCard = new QFrame();
    toolsCard->setObjectName("actionCard");
    QGridLayout* toolsGrid = new QGridLayout(toolsCard);
    toolsGrid->setContentsMargins(12, 10, 12, 10);
    toolsGrid->setSpacing(6);

    m_cardInfo = makeActionCard(":/icons/info.png", "Info Scheda", "sistema e hardware", "info");
    m_cardInfo->setAccent("sky");
    m_cardMidi = makeActionCard(":/icons/midi.png", "Info MIDI", "porte e connessioni", "midi");
    m_cardMidi->setAccent("pink");
    m_cardMidiTest = makeActionCard(":/icons/miditest.png", "Test MIDI", "nota di prova Do4", "miditest");
    m_cardMidiTest->setAccent("orange");
    m_cardDocs = makeActionCard(":/icons/docs.png", "Documentazione", "guida completa", "docs");
    m_cardDocs->setAccent("cyan");
    m_cardDiag = makeActionCard(":/icons/diag.png", "Diagnostica", "report completo", "diag");
    m_cardDiag->setAccent("teal");
    m_cardQuit = makeActionCard(":/icons/stop.png", "Quit", "esci", "quit");
    m_cardQuit->setAccent("red");

    toolsGrid->addWidget(m_cardInfo, 0, 0);
    toolsGrid->addWidget(m_cardMidi, 0, 1);
    toolsGrid->addWidget(m_cardMidiTest, 1, 0);
    toolsGrid->addWidget(m_cardDocs, 1, 1);
    toolsGrid->addWidget(m_cardDiag, 2, 0);
    toolsGrid->addWidget(m_cardQuit, 2, 1);
    rightCol->addWidget(toolsCard);

    rightCol->addStretch();
    bodyLayout->addLayout(rightCol, 2);

    scroll->setWidget(body);
    rootLayout->addWidget(scroll, 1);
}

void DashboardWidget::setPill(QLabel* label, bool on) {
    label->setProperty("pill", on ? "on" : "off");
    label->style()->unpolish(label);
    label->style()->polish(label);
}

QWidget* DashboardWidget::makeLedRow(const QString& label, QLabel** value) {
    Q_UNUSED(label);
    Q_UNUSED(value);
    return nullptr;
}

void DashboardWidget::setLed(QLabel* led, bool on, bool idle) {
    led->setProperty("led", idle ? "idle" : (on ? "on" : "off"));
    led->style()->unpolish(led);
    led->style()->polish(led);
}

void DashboardWidget::updateStatus(bool jackRunning, const QString& jackDetail,
                                   bool bridgeActive, bool sysmodeActive,
                                   bool watchdogRunning, bool autostart,
                                   const QString& device, const QString& driver,
                                   const QString& firmware, const QString& usb,
                                   const QString& midi, bool audioOn,
                                   const QString& sinkVol, const QString& sourceVol) {
    m_deviceLabel->setText(device);
    m_driverLabel->setText(driver);
    m_firmwareLabel->setText(firmware);
    m_usbLabel->setText(usb);
    m_midiLabel->setText(midi);

    setLed(m_ledDriver, driver.contains("loaded"));
    setLed(m_ledJack, audioOn);
    setLed(m_ledMidi, !midi.isEmpty() && midi != "assente", true);

    setPill(m_statusLabel, jackRunning);
    m_statusLabel->setText(jackRunning ? "JACK ACTIVE" : "READY");

    m_jackLabel->setText(jackRunning ? "ON" : "OFF");
    m_jackLabel->setProperty("active", jackRunning);
    m_jackLabel->style()->unpolish(m_jackLabel);
    m_jackLabel->style()->polish(m_jackLabel);
    m_jackDetailLabel->setText(jackDetail);

    // JACK cards
    m_cardStart->setEnabled(!jackRunning);
    m_cardStop->setEnabled(jackRunning);
    m_cardStart->setBadge(jackRunning ? "ON" : "→", jackRunning);
    m_cardStop->setBadge(jackRunning ? "ON" : "—", false);
    if (jackRunning) {
        m_cardStart->setSubtitle("già attivo");
        m_cardStop->setSubtitle("in esecuzione");
    } else {
        m_cardStart->setSubtitle("driver usb_stream");
        m_cardStop->setSubtitle("non attivo");
    }

    // Bridge + Sysmode
    m_cardBridge->setEnabled(jackRunning);
    m_cardBridge->setBadge(bridgeActive ? "ON" : "→", bridgeActive);
    m_cardBridge->setSubtitle(bridgeActive ? "collegato" : "audio di sistema ↔ JACK");
    m_cardSysmode->setBadge(sysmodeActive ? "ON" : "→", sysmodeActive);
    m_cardSysmode->setSubtitle(sysmodeActive ? "solo uscita" : "Usa Tascam come audio di sistema");

    // Watchdog + Autostart
    m_cardWatch->setBadge(watchdogRunning ? "ON" : "→", watchdogRunning);
    m_cardWatch->setSubtitle(watchdogRunning ? "attivo" : "riavvio automatico se cade");
    m_cardAutostart->setBadge(autostart ? "ON" : "→", autostart);
    m_cardAutostart->setSubtitle(autostart ? "abilitato al login" : "JACK al login");

    m_mixerSinkLabel->setText(sinkVol);
    m_mixerSourceLabel->setText(sourceVol);
}
