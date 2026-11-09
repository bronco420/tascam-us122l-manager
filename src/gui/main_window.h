#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>
#include <QSettings>

// Core
#include "core/jack_manager.h"
#include "core/pipewire_bridge.h"
#include "core/sysmode.h"
#include "core/mixer.h"
#include "core/preset.h"
#include "core/watchdog.h"
#include "core/diagnostics.h"
#include "core/config.h"
#include "core/task_runner.h"

// GUI — new shell
#include "cards.h"
#include "drawer.h"
#include "theme_tokens.h"

// Existing pages (kept for backward compat)
#include "mixer_widget.h"
#include "preset_widget.h"
#include "config_widget.h"
#include "info_widget.h"
#include "documentation_widget.h"
#include "diagnostics_widget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Tray actions
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayQuit();
    void onTrayShow();
    void onTrayMiniMode();

    // Dashboard actions
    void onStartClicked();
    void onStopClicked();
    void onBridgeClicked();
    void onSysmodeClicked();
    void onWatchdogClicked();
    void onMixerClicked();
    void onPresetSelected(const QString& name);
    void onTransportAction(const QString& action);

    // Core signal handlers (async via TaskRunner)
    void onJackStarted();
    void onJackStopped();
    void onJackError(const QString& error);
    void onBridgeEnabled();
    void onBridgeDisabled();
    void onBridgeError(const QString& error);
    void onSysmodeEnabled();
    void onSysmodeDisabled();
    void onPresetApplied(const QString& name);
    void onWatchdogStarted();
    void onWatchdogStopped();
    void onWatchdogJackRestarted();

    // Periodic refresh
    void onRefreshTimer();

private:
    void setupUI();
    void connectSignals();
    void updateDashboardState();
    void loadGeometry();
    void saveGeometry();
    void showMiniMode();
    void restoreNormalMode();

    // Core objects
    JackManager* m_jackManager = nullptr;
    PipeWireBridge* m_pwBridge = nullptr;
    Sysmode* m_sysmode = nullptr;
    Mixer* m_mixer = nullptr;
    Preset* m_preset = nullptr;
    Watchdog* m_watchdog = nullptr;
    Diagnostics* m_diagnostics = nullptr;
    Config& m_config;

    // Task runner for async operations
    TaskRunner* m_taskRunner = nullptr;

    // Dashboard components
    TopBar* m_topBar = nullptr;
    ChassisCard* m_chassisCard = nullptr;
    EngineCard* m_engineCard = nullptr;
    SoftwareMixerCard* m_mixerCard = nullptr;
    TransportBar* m_transportBar = nullptr;

    // Stacked widget for pages (dashboard + existing widgets)
    QStackedWidget* m_pages = nullptr;

    // Existing page widgets
    MixerWidget* m_mixerWidget = nullptr;
    PresetWidget* m_presetWidget = nullptr;
    ConfigWidget* m_configWidget = nullptr;
    InfoWidget* m_infoWidget = nullptr;
    DocumentationWidget* m_docsWidget = nullptr;
    DiagnosticsWidget* m_diagWidget = nullptr;

    // Drawer
    Drawer* m_drawer = nullptr;

    // Tray
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;

    // Timer for periodic refresh
    QTimer* m_refreshTimer = nullptr;

    // State flags
    bool m_miniMode = false;
};

#endif // MAIN_WINDOW_H
