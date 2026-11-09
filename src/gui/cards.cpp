#include "cards.h"
#include "theme_tokens.h"
#include <QPainterPath>
#include <QPainter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QStyleOption>
#include <QApplication>
#include <cmath>

// ──────────────────────────────────────────────
// Card
// ──────────────────────────────────────────────
Card::Card(QWidget* parent)
    : QFrame(parent), m_layout(new QVBoxLayout(this)) {
    m_layout->setContentsMargins(16, 16, 16, 16);
    m_layout->setSpacing(12);
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Raised);

    // Title label at top
    m_titleLabel = new QLabel(this);
    m_titleLabel->setFont(UiTheme::fontLabel());
    m_titleLabel->setStyleSheet("color: #A7B4BA; font-size: 12px; letter-spacing: 1px;");
    m_layout->addWidget(m_titleLabel);
}

void Card::setTitle(const QString& title) { m_titleLabel->setText(title); }

QWidget* Card::contentWidget() const { return const_cast<Card*>(this); }
QVBoxLayout* Card::layout() { return m_layout; }

void Card::paintEvent(QPaintEvent*) {
    QPainter p(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    // Raised panel background
    p.fillRect(rect(), QColor(0x15, 0x1A, 0x1D));

    // Subtle border
    QPen pen(QColor(0x26, 0x31, 0x36), 1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(0, 0, -1, -1));
}

// ──────────────────────────────────────────────
// ChassisCard — TASCAM front panel silhouette
// ──────────────────────────────────────────────
ChassisCard::ChassisCard(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(200);
}

void ChassisCard::setJackStatus(bool running) { m_jackRunning = running; update(); }
void ChassisCard::setBridgeStatus(bool active) { m_bridgeActive = active; update(); }
void ChassisCard::setSysmodeStatus(bool active) { m_sysmodeActive = active; update(); }
void ChassisCard::setXrunCount(int count) { m_xrunCount = count; update(); }
void ChassisCard::setAudioOn(bool on) { m_audioOn = on; update(); }

void ChassisCard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();

    // Background — dark panel
    p.fillRect(rect(), QColor(0x0E, 0x11, 0x13));

    // TASCAM-style chassis outline
    int margin = 24;
    QRect chassis(margin, margin, w - 2*margin, h - 2*margin);

    // Chassis body
    p.setBrush(QColor(0x15, 0x1A, 0x1D));
    p.setPen(QPen(QColor(0x26, 0x31, 0x36), 1));
    p.drawRoundedRect(chassis, 8, 8);

    // TASCAM label area (top-left)
    QRect logoRect(chassis.x() + 12, chassis.y() + 8, 80, 16);
    p.setPen(QColor(0x6B, 0x7B, 0x82));
    p.setFont(UiTheme::fontCaption());
    p.drawText(logoRect, Qt::AlignVCenter, "TASCAM");

    // Model number (top-right)
    QRect modelRect(chassis.right() - 100, chassis.y() + 8, 90, 16);
    p.drawText(modelRect, Qt::AlignVCenter | Qt::AlignRight, "US-122L");

    // Knobs row (decorative — GAIN x4, MONO MIX, PHONES)
    int knobY = chassis.y() + chassis.height() / 2 - 10;
    int knobR = 10;
    QString knobLabels[] = { "GAIN", "GAIN", "GAIN", "GAIN", "MON MIX", "PHONES" };
    double spacing = double(chassis.width()) / (sizeof(knobLabels)/sizeof(knobLabels[0]) + 1);

    for (int i = 0; i < 6; ++i) {
        double cx = chassis.x() + spacing * (i + 1);
        // Knob circle
        p.setBrush(QColor(0x1C, 0x23, 0x27));
        p.setPen(QPen(QColor(0x35, 0x44, 0x4B), 1));
        p.drawEllipse(int(cx) - knobR, knobY - knobR, knobR*2, knobR*2);

        // Knob label below
        QRect lblRect(int(cx) - 20, knobY + knobR + 4, 40, 12);
        p.setPen(QColor(0x6B, 0x7B, 0x82));
        p.setFont(UiTheme::fontCaption());
        p.drawText(lblRect, Qt::AlignHCenter, knobLabels[i]);
    }

    // Status LEDs row (bottom of chassis)
    int ledY = chassis.bottom() - 16;
    struct { const char* label; bool active; QColor color; } leds[] = {
        { "JACK", m_jackRunning, QColor(0x34, 0xD3, 0x99) },
        { "BRG", m_bridgeActive, QColor(0x60, 0xA5, 0xFA) },
        { "SYS", m_sysmodeActive, QColor(0xFB, 0xBF, 0x24) },
        { "XRUN", m_xrunCount > 0, QColor(0xF8, 0x71, 0x71) },
    };

    int ledSpacing = chassis.width() / (sizeof(leds)/sizeof(leds[0]) + 1);
    for (int i = 0; i < 4; ++i) {
        double lx = chassis.x() + ledSpacing * (i + 1);
        // LED dot
        p.setBrush(leds[i].active ? leds[i].color : QColor(0x26, 0x31, 0x36));
        if (leds[i].active) {
            // Glow
            QPainterPath glow;
            glow.addEllipse(int(lx) - 6, ledY - 6, 12, 12);
            p.fillPath(glow, QColor(leds[i].color.red(), leds[i].color.green(),
                                    leds[i].color.blue(), 40));
        }
        QPainterPath dot;
        dot.addEllipse(int(lx) - 3, ledY - 3, 6, 6);
        p.fillPath(dot, leds[i].active ? leds[i].color : QColor(0x26, 0x31, 0x36));

        // Label
        QRect lblRect(int(lx) - 14, ledY + 8, 28, 10);
        p.setPen(QColor(0x6B, 0x7B, 0x82));
        p.setFont(UiTheme::fontCaption());
        p.drawText(lblRect, Qt::AlignHCenter, leds[i].label);
    }

    // XRUN count display
    if (m_xrunCount > 0) {
        QString xrunText = QString("XRUN: %1").arg(m_xrunCount);
        QRect xrunRect(chassis.x() + chassis.width()/2 - 30, ledY + 8, 60, 10);
        p.setPen(QColor(0xF8, 0x71, 0x71));
        p.setFont(UiTheme::fontCaption());
        p.drawText(xrunRect, Qt::AlignHCenter, xrunText);
    }

    // Audio-on indicator (subtle accent line at bottom)
    if (m_audioOn) {
        p.setPen(QPen(UiTheme::accentDefault(), 2));
        p.drawLine(chassis.x() + 8, chassis.bottom() - 2,
                   chassis.right() - 8, chassis.bottom() - 2);
    }
}

// ──────────────────────────────────────────────
// EngineCard
// ──────────────────────────────────────────────
EngineCard::EngineCard(QWidget* parent)
    : Card(parent) {
    setTitle("MOTORE AUDIO");

    // Top row: JACK params (knobs) + status chips
    auto* topLayout = new QHBoxLayout();
    m_knobSampleRate = new ArcKnob();
    m_knobSampleRate->setMinValue(44100);
    m_knobSampleRate->setMaxValue(96000);
    m_knobSampleRate->setValue(48000);
    m_knobSampleRate->setLabel("Frequenza");
    m_knobSampleRate->setUnits("Hz");
    topLayout->addWidget(m_knobSampleRate, 1);

    m_knobBufferSize = new ArcKnob();
    m_knobBufferSize->setMinValue(32);
    m_knobBufferSize->setMaxValue(1024);
    m_knobBufferSize->setValue(128);
    m_knobBufferSize->setLabel("Buffer");
    m_knobBufferSize->setUnits("frame");
    topLayout->addWidget(m_knobBufferSize, 1);

    // Spacer
    topLayout->addStretch();

    layout()->addLayout(topLayout);

    // Divider
    auto* divider = new QWidget();
    divider->setFixedHeight(1);
    divider->setStyleSheet("background-color: #263136;");
    layout()->addWidget(divider);

    // Status chips row
    auto* chipLayout = new QHBoxLayout();
    m_chipJack = new StatusChip();
    m_chipBridge = new StatusChip();
    m_chipSysmode = new StatusChip();
    m_chipWatchdog = new StatusChip();
    m_chipXrun = new StatusChip();
    chipLayout->addWidget(m_chipJack);
    chipLayout->addSpacing(8);
    chipLayout->addWidget(m_chipBridge);
    chipLayout->addSpacing(8);
    chipLayout->addWidget(m_chipSysmode);
    chipLayout->addSpacing(8);
    chipLayout->addWidget(m_chipWatchdog);
    chipLayout->addSpacing(16);
    chipLayout->addWidget(m_chipXrun);
    layout()->addLayout(chipLayout);

    // Jack detail label
    m_lblJackDetail = new QLabel("");
    m_lblJackDetail->setFont(UiTheme::fontCaption());
    m_lblJackDetail->setStyleSheet("color: #6B7B82;");
    layout()->addWidget(m_lblJackDetail);

    // Divider
    divider = new QWidget();
    divider->setFixedHeight(1);
    divider->setStyleSheet("background-color: #263136;");
    layout()->addWidget(divider);

    // Action buttons row
    auto* btnLayout = new QHBoxLayout();
    m_btnStart = new QPushButton("Avvia JACK");
    m_btnStart->setObjectName("startBtn");
    connect(m_btnStart, &QPushButton::clicked, this, &EngineCard::onStartClicked);
    btnLayout->addWidget(m_btnStart);

    m_btnStop = new QPushButton("Ferma JACK");
    connect(m_btnStop, &QPushButton::clicked, this, &EngineCard::onStopClicked);
    btnLayout->addWidget(m_btnStop);

    btnLayout->addSpacing(16);

    m_btnBridge = new QPushButton("Bridge PW");
    connect(m_btnBridge, &QPushButton::clicked, this, &EngineCard::onBridgeClicked);
    btnLayout->addWidget(m_btnBridge);

    m_btnSysmode = new QPushButton("Scheda di sistema");
    connect(m_btnSysmode, &QPushButton::clicked, this, &EngineCard::onSysmodeClicked);
    btnLayout->addWidget(m_btnSysmode);

    btnLayout->addSpacing(16);

    m_btnWatchdog = new QPushButton("Watchdog");
    connect(m_btnWatchdog, &QPushButton::clicked, this, &EngineCard::onWatchdogClicked);
    btnLayout->addWidget(m_btnWatchdog);

    layout()->addLayout(btnLayout);
}

void EngineCard::updateEngineState(bool jackRunning, const QString& jackDetail,
                                    bool bridgeActive, bool sysmodeActive,
                                    bool watchdogRunning, int xrunCount) {
    m_chipJack->setText(jackRunning ? "JACK attivo" : "JACK spento");
    m_chipJack->setState(jackRunning ? StatusChip::Ok : StatusChip::Idle);

    m_chipBridge->setText(bridgeActive ? "Bridge attivo" : "Bridge spento");
    m_chipBridge->setState(bridgeActive ? StatusChip::Ok : StatusChip::Idle);

    m_chipSysmode->setText(sysmodeActive ? "Sysmode attivo" : "Sysmode spento");
    m_chipSysmode->setState(sysmodeActive ? StatusChip::Warn : StatusChip::Idle);

    m_chipWatchdog->setText(watchdogRunning ? "Watchdog attivo" : "Watchdog spento");
    m_chipWatchdog->setState(watchdogRunning ? StatusChip::Ok : StatusChip::Idle);

    m_chipXrun->setText(QString("XRUN: %1").arg(xrunCount));
    m_chipXrun->setState(xrunCount > 5 ? StatusChip::Error :
                         xrunCount > 0 ? StatusChip::Warn : StatusChip::Ok);

    m_lblJackDetail->setText(jackDetail);
}

void EngineCard::onStartClicked() { emit actionRequested("start"); }
void EngineCard::onStopClicked() { emit actionRequested("stop"); }
void EngineCard::onBridgeClicked() { emit actionRequested("bridge"); }
void EngineCard::onSysmodeClicked() { emit actionRequested("sysmode"); }
void EngineCard::onWatchdogClicked() { emit actionRequested("watch"); }

// ──────────────────────────────────────────────
// SoftwareMixerCard
// ──────────────────────────────────────────────
SoftwareMixerCard::SoftwareMixerCard(QWidget* parent)
    : Card(parent) {
    setTitle("MIXER SOFTWARE");

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(8);

    // Sink (output) column header
    m_lblSinkName = new QLabel("USCITA (Altoparlanti)");
    m_lblSinkName->setFont(UiTheme::fontLabel());
    m_lblSinkName->setStyleSheet("color: #A7B4BA; font-size: 11px;");
    grid->addWidget(m_lblSinkName, 0, 0, Qt::AlignHCenter);

    // Sink controls
    m_btnSinkUp = new QPushButton("▲");
    m_btnSinkDown = new QPushButton("▼");
    m_btnSinkMute = new LedToggle();
    m_btnSinkMute->setText("MUTED");
    m_lblSinkVol = new QLabel("0 dB");
    m_lblSinkVol->setFont(UiTheme::fontMono());
    m_lblSinkVol->setStyleSheet("color: #E8EEF1; font-size: 16px;");

    auto* sinkLayout = new QVBoxLayout();
    sinkLayout->addWidget(m_btnSinkUp);
    sinkLayout->addStretch(1);
    sinkLayout->addWidget(m_lblSinkVol, Qt::AlignHCenter);
    sinkLayout->addStretch(1);
    sinkLayout->addWidget(m_btnSinkDown);

    grid->addLayout(sinkLayout, 1, 0);
    grid->addWidget(m_btnSinkMute, 2, 0, Qt::AlignHCenter);

    connect(m_btnSinkUp, &QPushButton::clicked, this, &SoftwareMixerCard::onSinkUp);
    connect(m_btnSinkDown, &QPushButton::clicked, this, &SoftwareMixerCard::onSinkDown);
    connect(m_btnSinkMute, &LedToggle::toggled, this, &SoftwareMixerCard::onSinkMute);

    // Source (input) column header
    m_lblSourceName = new QLabel("INGRESSO (Tascam)");
    m_lblSourceName->setFont(UiTheme::fontLabel());
    m_lblSourceName->setStyleSheet("color: #A7B4BA; font-size: 11px;");
    grid->addWidget(m_lblSourceName, 0, 1, Qt::AlignHCenter);

    // Source controls
    m_btnSourceUp = new QPushButton("▲");
    m_btnSourceDown = new QPushButton("▼");
    m_btnSourceMute = new LedToggle();
    m_btnSourceMute->setText("MUTED");
    m_lblSourceVol = new QLabel("0 dB");
    m_lblSourceVol->setFont(UiTheme::fontMono());
    m_lblSourceVol->setStyleSheet("color: #E8EEF1; font-size: 16px;");

    auto* srcLayout = new QVBoxLayout();
    srcLayout->addWidget(m_btnSourceUp);
    srcLayout->addStretch(1);
    srcLayout->addWidget(m_lblSourceVol, Qt::AlignHCenter);
    srcLayout->addStretch(1);
    srcLayout->addWidget(m_btnSourceDown);

    grid->addLayout(srcLayout, 1, 1);
    grid->addWidget(m_btnSourceMute, 2, 1, Qt::AlignHCenter);

    connect(m_btnSourceUp, &QPushButton::clicked, this, &SoftwareMixerCard::onSourceUp);
    connect(m_btnSourceDown, &QPushButton::clicked, this, &SoftwareMixerCard::onSourceDown);
    connect(m_btnSourceMute, &LedToggle::toggled, this, &SoftwareMixerCard::onSourceMute);

    layout()->addLayout(grid);
}

void SoftwareMixerCard::updateMixerState(int sinkVol, int sourceVol,
                                          bool sinkMuted, bool sourceMuted) {
    m_lblSinkVol->setText(QString("%1 dB").arg(sinkVol));
    m_lblSourceVol->setText(QString("%1 dB").arg(sourceVol));
    m_btnSinkMute->setState(sinkMuted);
    m_btnSourceMute->setState(sourceMuted);

    // Color volume based on level
    if (sinkVol > 80) {
        m_lblSinkVol->setStyleSheet("color: #F87171; font-size: 16px;");
    } else if (sinkVol > 60) {
        m_lblSinkVol->setStyleSheet("color: #FBBF24; font-size: 16px;");
    } else {
        m_lblSinkVol->setStyleSheet("color: #E8EEF1; font-size: 16px;");
    }

    if (sourceVol > 80) {
        m_lblSourceVol->setStyleSheet("color: #F87171; font-size: 16px;");
    } else if (sourceVol > 60) {
        m_lblSourceVol->setStyleSheet("color: #FBBF24; font-size: 16px;");
    } else {
        m_lblSourceVol->setStyleSheet("color: #E8EEF1; font-size: 16px;");
    }
}

void SoftwareMixerCard::onSinkUp() { emit actionRequested("vol-up"); }
void SoftwareMixerCard::onSinkDown() { emit actionRequested("vol-down"); }
void SoftwareMixerCard::onSinkMute(bool on) {
    emit actionRequested(on ? "vol-mute" : "vol-unmute");
}
void SoftwareMixerCard::onSourceUp() { emit actionRequested("src-up"); }
void SoftwareMixerCard::onSourceDown() { emit actionRequested("src-down"); }
void SoftwareMixerCard::onSourceMute(bool on) {
    emit actionRequested(on ? "src-mute" : "src-unmute");
}

// ──────────────────────────────────────────────
// TopBar
// ──────────────────────────────────────────────
TopBar::TopBar(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(48);
    setMinimumHeight(48);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(12);

    // Title
    m_titleLabel = new QLabel("Tascam US-122L Manager", this);
    m_titleLabel->setFont(UiTheme::fontLabel());
    m_titleLabel->setStyleSheet("color: #E8EEF1; font-size: 14px;");
    layout->addWidget(m_titleLabel);

    // Status chips
    m_chipJack = new StatusChip(this);
    m_chipBridge = new StatusChip(this);
    m_chipSysmode = new StatusChip(this);
    m_chipXrun = new StatusChip(this);
    layout->addSpacing(16);
    layout->addWidget(m_chipJack);
    layout->addSpacing(4);
    layout->addWidget(m_chipBridge);
    layout->addSpacing(4);
    layout->addWidget(m_chipSysmode);
    layout->addSpacing(4);
    layout->addWidget(m_chipXrun);

    layout->addStretch();

    // Tray button (tray icon)
    auto* trayBtn = new QPushButton("─", this);
    trayBtn->setFixedSize(32, 28);
    trayBtn->setStyleSheet("QPushButton { background: transparent; color: #6B7B82; font-size: 16px; border: none; }"
                           "QPushButton:hover { color: #E8EEF1; }");
    trayBtn->setToolTip("Nascondi nel vassoio");
    connect(trayBtn, &QPushButton::clicked, this, [&]() { emit trayRequested(); });
    layout->addWidget(trayBtn);

    // Mini mode button
    auto* miniBtn = new QPushButton("◻", this);
    miniBtn->setFixedSize(32, 28);
    miniBtn->setStyleSheet("QPushButton { background: transparent; color: #6B7B82; font-size: 14px; border: none; }"
                           "QPushButton:hover { color: #E8EEF1; }");
    miniBtn->setToolTip("Modalità mini DAW");
    connect(miniBtn, &QPushButton::clicked, this, [&]() { emit miniModeRequested(); });
    layout->addWidget(miniBtn);
}

void TopBar::setJackStatus(bool running) {
    m_chipJack->setText(running ? "JACK" : "JACK");
    m_chipJack->setState(running ? StatusChip::Ok : StatusChip::Idle);
}

void TopBar::setBridgeStatus(bool active) {
    m_chipBridge->setText(active ? "BRIDGE" : "BRIDGE");
    m_chipBridge->setState(active ? StatusChip::Ok : StatusChip::Idle);
}

void TopBar::setSysmodeStatus(bool active) {
    m_chipSysmode->setText(active ? "SYS" : "SYS");
    m_chipSysmode->setState(active ? StatusChip::Warn : StatusChip::Idle);
}

void TopBar::setXrunCount(int count) {
    m_chipXrun->setText(QString("XRUN:%1").arg(count));
    m_chipXrun->setState(count > 5 ? StatusChip::Error :
                         count > 0 ? StatusChip::Warn : StatusChip::Ok);
}

void TopBar::setWindowTitleCustom(const QString& title) {
    m_titleLabel->setText(title);
}

void TopBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    // Dark top bar background
    p.fillRect(rect(), QColor(0x0E, 0x11, 0x13));
    // Bottom border line
    p.setPen(QColor(0x26, 0x31, 0x36));
    p.drawLine(0, height() - 1, width(), height() - 1);
}

void TopBar::mousePressEvent(QMouseEvent*) {
    // Allow dragging the top bar to move the window (native frame)
    if (window()) {
        window()->setFocus();
    }
}

// ──────────────────────────────────────────────
// TransportBar
// ──────────────────────────────────────────────
TransportBar::TransportBar(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(40);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 4, 16, 4);
    layout->setSpacing(12);

    m_presetSelector = new SegmentedSelector();
    connect(m_presetSelector, &SegmentedSelector::currentIndexChanged,
            this, &TransportBar::onPresetClicked);
    layout->addWidget(m_presetSelector);
    layout->addStretch();
}

void TransportBar::setPresets(const QStringList& presets) {
    m_presetSelector->clear();  // need to add items
    for (const auto& p : presets) {
        m_presetSelector->addItem(p);
    }
}

void TransportBar::setCurrentPreset(const QString& name) {
    for (int i = 0; i < m_presetSelector->items().size(); ++i) {
        if (m_presetSelector->items()[i] == name) {
            m_presetSelector->setCurrentIndex(i);
            return;
        }
    }
}

void TransportBar::onPresetClicked(int index) {
    if (index >= 0 && index < m_presetSelector->items().size()) {
        emit presetSelected(m_presetSelector->items()[index]);
    }
}

// ──────────────────────────────────────────────
// CompactWidget — mini DAW bar
// ──────────────────────────────────────────────
CompactWidget::CompactWidget(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(56);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    m_chipJack = new StatusChip(this);
    m_chipBridge = new StatusChip(this);
    m_chipSysmode = new StatusChip(this);
    layout->addWidget(m_chipJack);
    layout->addSpacing(4);
    layout->addWidget(m_chipBridge);
    layout->addSpacing(4);
    layout->addWidget(m_chipSysmode);

    layout->addStretch();

    m_lblSinkVol = new QLabel("0 dB");
    m_lblSinkVol->setFont(UiTheme::fontMono());
    m_lblSinkVol->setStyleSheet("color: #14B8A6; font-size: 12px;");
    layout->addWidget(m_lblSinkVol);

    m_lblSourceVol = new QLabel("0 dB");
    m_lblSourceVol->setFont(UiTheme::fontMono());
    m_lblSourceVol->setStyleSheet("color: #14B8A6; font-size: 12px;");
    layout->addWidget(m_lblSourceVol);

    layout->addSpacing(8);

    m_btnMixer = new QPushButton("MIX");
    m_btnMixer->setFixedSize(36, 28);
    connect(m_btnMixer, &QPushButton::clicked, this, [&]() {
        emit actionRequested("mixer");
    });
    layout->addWidget(m_btnMixer);

    m_btnExpand = new QPushButton("▣");
    m_btnExpand->setFixedSize(36, 28);
    connect(m_btnExpand, &QPushButton::clicked, this, [&]() {
        emit actionRequested("expand");
    });
    layout->addWidget(m_btnExpand);
}

void CompactWidget::setJackStatus(bool running) {
    m_chipJack->setText(running ? "J" : "J");
    m_chipJack->setState(running ? StatusChip::Ok : StatusChip::Idle);
}

void CompactWidget::setBridgeStatus(bool active) {
    m_chipBridge->setText(active ? "B" : "B");
    m_chipBridge->setState(active ? StatusChip::Ok : StatusChip::Idle);
}

void CompactWidget::setSysmodeStatus(bool active) {
    m_chipSysmode->setText(active ? "S" : "S");
    m_chipSysmode->setState(active ? StatusChip::Warn : StatusChip::Idle);
}

void CompactWidget::setSinkVolume(int vol) {
    m_lblSinkVol->setText(QString("%1 dB").arg(vol));
    if (vol > 80) m_lblSinkVol->setStyleSheet("color: #F87171; font-size: 12px;");
    else m_lblSinkVol->setStyleSheet("color: #14B8A6; font-size: 12px;");
}

void CompactWidget::setSourceVolume(int vol) {
    m_lblSourceVol->setText(QString("%1 dB").arg(vol));
    if (vol > 80) m_lblSourceVol->setStyleSheet("color: #F87171; font-size: 12px;");
    else m_lblSourceVol->setStyleSheet("color: #14B8A6; font-size: 12px;");
}

void CompactWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0x0E, 0x11, 0x13));
    p.setPen(QColor(0x26, 0x31, 0x36));
    p.drawLine(0, height() - 1, width(), height() - 1);
}

void CompactWidget::mousePressEvent(QMouseEvent*) {
    if (window()) window()->setFocus();
}
