#ifndef CARDS_H
#define CARDS_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include "controls.h"

// ──────────────────────────────────────────────
// Card — base panel with subtle border and raised bg
// ──────────────────────────────────────────────
class Card : public QFrame {
    Q_OBJECT

public:
    explicit Card(QWidget* parent = nullptr);
    void setTitle(const QString& title);
    QWidget* contentWidget() const;
    QVBoxLayout* layout();

protected:
    void paintEvent(QPaintEvent*) override;

    QVBoxLayout* m_layout;
    QLabel* m_titleLabel;
};

// ──────────────────────────────────────────────
// ChassisCard — stylized TASCAM US-122L front panel silhouette
// ──────────────────────────────────────────────
class ChassisCard : public QWidget {
    Q_OBJECT

public:
    explicit ChassisCard(QWidget* parent = nullptr);

    void setJackStatus(bool running);
    void setBridgeStatus(bool active);
    void setSysmodeStatus(bool active);
    void setXrunCount(int count);
    void setAudioOn(bool on);

signals:
    void actionRequested(const QString& action);

private:
    void paintEvent(QPaintEvent*) override;

    bool m_jackRunning = false;
    bool m_bridgeActive = false;
    bool m_sysmodeActive = false;
    int m_xrunCount = 0;
    bool m_audioOn = false;
};

// ──────────────────────────────────────────────
// EngineCard — JACK / PipeWire / Sysmode status + controls
// ──────────────────────────────────────────────
class EngineCard : public Card {
    Q_OBJECT

public:
    explicit EngineCard(QWidget* parent = nullptr);

    void updateEngineState(bool jackRunning, const QString& jackDetail,
                           bool bridgeActive, bool sysmodeActive,
                           bool watchdogRunning, int xrunCount);

signals:
    void actionRequested(const QString& action);

private slots:
    void onStartClicked();
    void onStopClicked();
    void onBridgeClicked();
    void onSysmodeClicked();
    void onWatchdogClicked();

private:
    ArcKnob* m_knobSampleRate;
    ArcKnob* m_knobBufferSize;
    StatusChip* m_chipJack;
    StatusChip* m_chipBridge;
    StatusChip* m_chipSysmode;
    StatusChip* m_chipWatchdog;
    StatusChip* m_chipXrun;
    QPushButton* m_btnStart;
    QPushButton* m_btnStop;
    QPushButton* m_btnBridge;
    QPushButton* m_btnSysmode;
    QPushButton* m_btnWatchdog;
    QLabel* m_lblJackDetail;
};

// ──────────────────────────────────────────────
// SoftwareMixerCard — volume/mute controls for PipeWire
// ──────────────────────────────────────────────
class SoftwareMixerCard : public Card {
    Q_OBJECT

public:
    explicit SoftwareMixerCard(QWidget* parent = nullptr);

    void updateMixerState(int sinkVol, int sourceVol,
                          bool sinkMuted, bool sourceMuted);

signals:
    void actionRequested(const QString& action);

private slots:
    void onSinkUp();
    void onSinkDown();
    void onSinkMute(bool on);
    void onSourceUp();
    void onSourceDown();
    void onSourceMute(bool on);

private:
    QPushButton* m_btnSinkUp;
    QPushButton* m_btnSinkDown;
    LedToggle* m_btnSinkMute;
    QLabel* m_lblSinkVol;
    QPushButton* m_btnSourceUp;
    QPushButton* m_btnSourceDown;
    LedToggle* m_btnSourceMute;
    QLabel* m_lblSourceVol;
    QLabel* m_lblSinkName;
    QLabel* m_lblSourceName;
};

// ──────────────────────────────────────────────
// TopBar — top application bar with title, status chips, tray button
// ──────────────────────────────────────────────
class TopBar : public QWidget {
    Q_OBJECT

public:
    explicit TopBar(QWidget* parent = nullptr);

    void setJackStatus(bool running);
    void setBridgeStatus(bool active);
    void setSysmodeStatus(bool active);
    void setXrunCount(int count);
    void setWindowTitleCustom(const QString& title);

signals:
    void trayRequested();
    void miniModeRequested();
    void actionRequested(const QString& action);

private:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

    QLabel* m_titleLabel;
    StatusChip* m_chipJack;
    StatusChip* m_chipBridge;
    StatusChip* m_chipSysmode;
    StatusChip* m_chipXrun;
};

// ──────────────────────────────────────────────
// TransportBar — play/stop/rec + preset selector
// ──────────────────────────────────────────────
class TransportBar : public QWidget {
    Q_OBJECT

public:
    explicit TransportBar(QWidget* parent = nullptr);

    void setPresets(const QStringList& presets);
    void setCurrentPreset(const QString& name);

signals:
    void actionRequested(const QString& action);
    void presetSelected(const QString& name);

private slots:
    void onPresetClicked(int index);

private:
    SegmentedSelector* m_presetSelector;
};

// ──────────────────────────────────────────────
// CompactWidget — mini DAW bar (for compact mode)
// ──────────────────────────────────────────────
class CompactWidget : public QWidget {
    Q_OBJECT

public:
    explicit CompactWidget(QWidget* parent = nullptr);

    void setJackStatus(bool running);
    void setBridgeStatus(bool active);
    void setSysmodeStatus(bool active);
    void setSinkVolume(int vol);
    void setSourceVolume(int vol);

signals:
    void actionRequested(const QString& action);

private:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

    StatusChip* m_chipJack;
    StatusChip* m_chipBridge;
    StatusChip* m_chipSysmode;
    QPushButton* m_btnMixer;
    QPushButton* m_btnExpand;
    QLabel* m_lblSinkVol;
    QLabel* m_lblSourceVol;
};

#endif // CARDS_H
