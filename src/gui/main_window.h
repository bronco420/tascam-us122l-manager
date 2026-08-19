#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QFileDialog>
#include <QTimer>

#include "core/jack_manager.h"
#include "core/pipewire_bridge.h"
#include "core/sysmode.h"
#include "core/mixer.h"
#include "core/preset.h"
#include "core/watchdog.h"
#include "core/diagnostics.h"
#include "core/config.h"

#include "gui/dashboard_widget.h"
#include "gui/mixer_widget.h"
#include "gui/preset_widget.h"
#include "gui/config_widget.h"
#include "gui/info_widget.h"
#include "gui/documentation_widget.h"
#include "gui/diagnostics_widget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void showDashboard();
    void refreshDashboard();

private slots:
    void onActionRequested(const QString& action);

    void onDashboardClosed();
    void onActionStart();
    void onActionStop();
    void onActionBridge();
    void onActionSysmode();
    void onActionWatch();
    void onActionWatchStop();
    void onActionAutostart();
    void onActionMixer();
    void onActionPreset();
    void onActionConfig();
    void onActionInfo();
    void onActionMidi();
    void onActionMidiTest();
    void onActionDocs();
    void onActionDiag();
    void onActionRestart();
    void onActionOpenLog();
    void onActionOpenAsound();
    void onActionOpenTerminal();
    void onActionQuit();

    // Signal handlers
    void onJackStarted();
    void onJackStopped();
    void onJackError(const QString& error);
    void onBridgeEnabled();
    void onBridgeDisabled();
    void onSysmodeEnabled();
    void onSysmodeDisabled();
    void onPresetApplied(const QString& name);
    void onWatchdogStarted();
    void onWatchdogStopped();
    void onWatchdogJackRestarted();

private:
    void setupUI();
    void createMenuBar();
    void createStatusBar();
    void connectSignals();
    void updateStatusWidgets();

    Config& m_config;
    JackManager* m_jackManager;
    PipeWireBridge* m_pwBridge;
    Sysmode* m_sysmode;
    Mixer* m_mixer;
    Preset* m_preset;
    Watchdog* m_watchdog;
    Diagnostics* m_diagnostics;

    QStackedWidget* m_stackedWidget;
    DashboardWidget* m_dashboardWidget;
    MixerWidget* m_mixerWidget;
    PresetWidget* m_presetWidget;
    ConfigWidget* m_configWidget;
    InfoWidget* m_infoWidget;
    DocumentationWidget* m_docsWidget;
    DiagnosticsWidget* m_diagWidget;

    QLabel* m_statusLabel;
    QTimer* m_refreshTimer;
    bool m_isDashboardOpen;

    // Menu actions
    QAction* m_actionDashboard;
    QAction* m_actionStart;
    QAction* m_actionStop;
    QAction* m_actionBridge;
    QAction* m_actionSysmode;
    QAction* m_actionWatch;
    QAction* m_actionWatchStop;
    QAction* m_actionAutostart;
    QAction* m_actionMixer;
    QAction* m_actionPreset;
    QAction* m_actionConfig;
    QAction* m_actionInfo;
    QAction* m_actionMidi;
    QAction* m_actionMidiTest;
    QAction* m_actionDocs;
    QAction* m_actionDiag;
    QAction* m_actionRestart;
    QAction* m_actionOpenLog;
    QAction* m_actionOpenAsound;
    QAction* m_actionOpenTerminal;
    QAction* m_actionQuit;
};

#endif // MAIN_WINDOW_H