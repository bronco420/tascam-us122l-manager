#ifndef DASHBOARD_WIDGET_H
#define DASHBOARD_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QFrame>

// Clickable action card replicating the old HTML dashboard ("acard").
// Layout: [icon]  <b>Title</b>        [badge]
//                <i>Subtitle</i>
class ActionCard : public QFrame {
    Q_OBJECT

public:
    explicit ActionCard(const QString& iconPath = QString(),
                        const QString& title = QString(),
                        const QString& subtitle = QString(),
                        QWidget* parent = nullptr);

    void setIconPath(const QString& path);
    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);
    void setBadge(const QString& text, bool active = false);
    void setBadgeVisible(bool visible);
    void setAccent(const QString& hue);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLabel* m_badgeLabel;
    bool m_pressed = false;
};

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget* parent = nullptr);

    void updateStatus(bool jackRunning, const QString& jackDetail,
                      bool bridgeActive, bool sysmodeActive,
                      bool watchdogRunning, bool autostart,
                      const QString& device, const QString& driver,
                      const QString& firmware, const QString& usb,
                      const QString& midi, bool audioOn,
                      const QString& sinkVol, const QString& sourceVol);

signals:
    void actionRequested(const QString& action);

private:
    void setPill(QLabel* label, bool on);
    ActionCard* makeActionCard(const QString& icon, const QString& title,
                               const QString& subtitle, const QString& action);
    // LED row: label + value with colored led dot
    QWidget* makeLedRow(const QString& label, QLabel** value);
    void setLed(QLabel* led, bool on, bool idle = false);

    QLabel* m_statusLabel;
    QLabel* m_jackLabel;
    QLabel* m_jackDetailLabel;
    QLabel* m_bridgeLabel;
    QLabel* m_sysmodeLabel;
    QLabel* m_watchdogLabel;
    QLabel* m_deviceLabel;
    QLabel* m_driverLabel;
    QLabel* m_firmwareLabel;
    QLabel* m_usbLabel;
    QLabel* m_midiLabel;
    QLabel* m_audioLabel;

    // LED dots
    QLabel* m_ledDriver;
    QLabel* m_ledJack;
    QLabel* m_ledMidi;

    // Action cards
    ActionCard* m_cardStart;
    ActionCard* m_cardStop;
    ActionCard* m_cardBridge;
    ActionCard* m_cardSysmode;
    ActionCard* m_cardWatch;
    ActionCard* m_cardAutostart;
    ActionCard* m_cardConfig;
    ActionCard* m_cardMidi;
    ActionCard* m_cardMidiTest;
    ActionCard* m_cardInfo;
    ActionCard* m_cardDocs;
    ActionCard* m_cardDiag;
    ActionCard* m_cardQuit;

    // Mixer inline
    QPushButton* m_btnVolDown;
    QPushButton* m_btnVolUp;
    QPushButton* m_btnVolMute;
    QPushButton* m_btnSrcDown;
    QPushButton* m_btnSrcUp;
    QPushButton* m_btnSrcMute;
    QLabel* m_mixerSinkLabel;
    QLabel* m_mixerSourceLabel;

    // Presets
    ActionCard* m_cardPresetStudio;
    ActionCard* m_cardPresetStandard;
    ActionCard* m_cardPresetLive;
    ActionCard* m_cardPresetHiRes;
};

#endif // DASHBOARD_WIDGET_H