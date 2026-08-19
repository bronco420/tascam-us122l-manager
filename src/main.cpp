#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QStyleFactory>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QIcon>
#include <QProcess>
#include <QThread>

#include "core/jack_manager.h"
#include "core/pipewire_bridge.h"
#include "core/sysmode.h"
#include "core/mixer.h"
#include "core/preset.h"
#include "core/watchdog.h"
#include "core/diagnostics.h"
#include "core/config.h"
#include "core/utils.h"

#include "gui/main_window.h"

#include <iostream>

int main(int argc, char *argv[]) {
    // Initialize Qt application
    QApplication app(argc, argv);
    app.setApplicationName("Tascam US-122L Manager");
    app.setApplicationVersion("2.3.2");
    app.setOrganizationName("Tascam");
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setWindowIcon(QIcon(":/icons/tascam-us122l.png"));

    // Apply the 2026 dark theme (design tokens in resources/style.qss)
    QFile themeFile(":/styles/style.qss");
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
        themeFile.close();
    }

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Tascam US-122L Manager - Professional Audio Interface Control");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dashboardOption("dashboard", "Open the Control Panel (Qt dashboard)");
    parser.addOption(dashboardOption);

    QCommandLineOption startOption("start", "Start JACK with saved settings");
    parser.addOption(startOption);

    QCommandLineOption stopOption("stop", "Stop JACK");
    parser.addOption(stopOption);

    QCommandLineOption restartOption("restart", "Restart JACK");
    parser.addOption(restartOption);

    QCommandLineOption statusOption("status", "Show status (text)");
    parser.addOption(statusOption);

    QCommandLineOption diagOption("diag", "Show full diagnostics (report)");
    parser.addOption(diagOption);

    QCommandLineOption bridgeOption("bridge",
        "PipeWire-JACK bridge (on|off|toggle). Requires JACK running.",
        "mode");
    parser.addOption(bridgeOption);

    QCommandLineOption sysmodeOption("sysmode",
        "Use Tascam as system card (on|off|toggle). Exclusive with JACK.",
        "mode");
    parser.addOption(sysmodeOption);

    QCommandLineOption watchOption("watch", "Watchdog: restart JACK if it dies");
    parser.addOption(watchOption);

    QCommandLineOption watchStopOption("watch-stop", "Stop the watchdog");
    parser.addOption(watchStopOption);

    QCommandLineOption autostartOption("autostart",
        "Auto-start JACK at login (on|off)",
        "mode");
    parser.addOption(autostartOption);

    QCommandLineOption silentOption("silent", "No GUI windows (for systemd/cron)");
    parser.addOption(silentOption);

    parser.process(app);

    // Initialize core components
    Config config;
    JackManager jackManager(config);
    PipeWireBridge pwBridge(config);
    Sysmode sysmode(config);
    Mixer mixer(config);
    Preset preset(config);
    Watchdog watchdog(jackManager, config);
    Diagnostics diagnostics(config);

    // Handle command line options
    if (parser.isSet("status")) {
        // Text status output
        std::cout << "========================================\n";
        std::cout << "  Tascam US-122L Manager - Status\n";
        std::cout << "========================================\n";

        auto model = diagnostics.getCardModel();
        auto cardNum = diagnostics.getCardNumber();
        auto usbConn = diagnostics.getUSBConnection();
        auto firmware = diagnostics.getFirmwareVersion();
        auto sampleWidth = diagnostics.getSampleWidth();
        auto midiClient = diagnostics.getMidiClientName();

        std::cout << "Device:        " << model.toStdString()
                  << " (card " << cardNum.toStdString() << ")\n";
        std::cout << "Connection:    " << usbConn.toStdString() << "\n";
        std::cout << "Firmware:       " << firmware.toStdString()
                  << " | Sample: " << sampleWidth.toStdString() << "\n";
        std::cout << "Device MIDI:    " << midiClient.toStdString() << "\n";

        if (!cardNum.isEmpty()) {
            if (jackManager.isDriverLoaded()) {
                std::cout << "Driver:         loaded\n";
            } else {
                std::cout << "Driver:         NOT loaded\n";
            }
        }

        std::cout << "JACK server:    " << (jackManager.isRunning() ? "ACTIVE" : "INACTIVE") << "\n";

        if (jackManager.isRunning()) {
            auto sr = jackManager.getSampleRate();
            auto buf = jackManager.getBufferSize();
            auto pid = jackManager.getPID();

            std::cout << "  Sample Rate:     " << sr.toStdString() << " Hz\n";
            std::cout << "  Buffer Size:     " << buf.toStdString() << " frames\n";
            std::cout << "  PID:             " << pid.toStdString() << "\n";

            auto periods = jackManager.getPeriods();
            std::cout << "  Periods:         " << periods.toStdString() << "\n";
        }

        std::cout << "Bridge PipeWire:    "
                  << (pwBridge.isActive() ? "ON" : "OFF") << "\n";

        std::cout << "Sysmode:    "
                  << (sysmode.isActive() ? "ACTIVE (Tascam output)" : "OFF") << "\n";

        std::cout << "Watchdog:   "
                  << (watchdog.isRunning() ? "active" : "stopped") << "\n";

        if (config.asoundrcExists()) {
            std::cout << ".asoundrc:          configured\n";
        } else {
            std::cout << ".asoundrc:          NOT configured\n";
        }

        if (config.autostartEnabled()) {
            std::cout << "Auto-start:         enabled\n";
        } else {
            std::cout << "Auto-start:         disabled\n";
        }

        return 0;
    }

    if (parser.isSet("diag")) {
        // Full diagnostics report
        auto report = diagnostics.generateReport();
        std::cout << report.toStdString();
        return 0;
    }

    // Handle standalone CLI options
    if (parser.isSet("start")) {
        auto sr = config.getSampleRate();
        auto buf = config.getBufferSize();
        auto periods = config.getPeriods();

        std::cout << "Starting JACK (" << sr.toStdString() << "Hz / "
                  << buf.toStdString() << " / " << periods.toStdString() << ")...\n";
        if (!jackManager.start(sr, buf, periods)) {
            std::cerr << "Failed to start JACK. Check the log for details.\n";
            return 1;
        }
        std::cout << "JACK started (PID " << jackManager.getPID().toStdString() << ").\n";
        return 0;
    }

    if (parser.isSet("stop")) {
        jackManager.stop();
        std::cout << "JACK stopped.\n";
        return 0;
    }

    if (parser.isSet("restart")) {
        jackManager.stop();
        QThread::sleep(1);
        auto sr = config.getSampleRate();
        auto buf = config.getBufferSize();
        auto periods = config.getPeriods();
        if (!jackManager.start(sr, buf, periods)) {
            std::cerr << "Failed to restart JACK.\n";
            return 1;
        }
        std::cout << "JACK restarted.\n";
        return 0;
    }

    if (parser.isSet("watch")) {
        if (watchdog.isRunning()) {
            std::cout << "Watchdog already running.\n";
            return 0;
        }
        watchdog.start();
        if (!watchdog.isRunning()) {
            std::cerr << "Failed to start watchdog.\n";
            return 1;
        }
        std::cout << "Watchdog started (PID "
                  << QCoreApplication::applicationPid() << ").\n";
        // Keep the process alive so the watchdog's QTimers can fire.
        return app.exec();
    }

    if (parser.isSet("watch-stop")) {
        if (watchdog.isRunning()) {
            watchdog.stop();
            std::cout << "Watchdog stopped.\n";
            return 0;
        }
        // Stop a watchdog running in another process via its pid file.
        QString pidFile = config.getSettingsDir() + "/watchdog.pid";
        QFile pf(pidFile);
        if (pf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString pid = pf.readLine().trimmed();
            pf.close();
            if (!pid.isEmpty()) {
                QProcess killProc;
                killProc.start("kill", QStringList() << pid);
                killProc.waitForFinished(1000);
                if (killProc.exitCode() == 0) {
                    QThread::msleep(200);
                    std::cout << "Watchdog (PID " << pid.toStdString() << ") stopped.\n";
                } else {
                    std::cerr << "Failed to stop watchdog (PID " << pid.toStdString() << ").\n";
                }
            }
        }
        QFile::remove(pidFile);
        return 0;
    }

    if (parser.isSet("autostart")) {
        QString mode = parser.value("autostart");
        if (mode == "on") {
            config.setAutostart(true);
            std::cout << "Auto-start enabled.\n";
        } else if (mode == "off") {
            config.setAutostart(false);
            std::cout << "Auto-start disabled.\n";
        } else {
            std::cerr << "Usage: --autostart on|off\n";
            return 1;
        }
        return 0;
    }

    if (parser.isSet("bridge")) {
        QString mode = parser.value("bridge");
        if (!jackManager.isRunning()) {
            std::cerr << "Start JACK first! The bridge requires JACK to be running.\n";
            return 1;
        }
        if (mode == "on") {
            pwBridge.enable();
        } else if (mode == "off") {
            pwBridge.disable();
        } else {
            pwBridge.toggle();
        }
        std::cout << (pwBridge.isActive() ? "Bridge PipeWire: ON\n" : "Bridge PipeWire: OFF\n");
        return 0;
    }

    if (parser.isSet("sysmode")) {
        QString mode = parser.value("sysmode");
        if (mode == "on") {
            sysmode.enable();
        } else if (mode == "off") {
            sysmode.disable();
        } else {
            sysmode.toggle();
        }
        std::cout << (sysmode.isActive() ? "Sysmode: ACTIVE (Tascam output)\n" : "Sysmode: OFF\n");
        return 0;
    }


    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();

    // Non-blocking warning if the card is not detected
    if (!diagnostics.isCardDetected()) {
        QTimer::singleShot(0, [&mainWindow]() {
            QMessageBox::warning(&mainWindow, "Warning",
                "Tascam US-122L not detected!\n\n"
                "Make sure:\n"
                "- The card is connected via USB\n"
                "- No other app is using the card\n"
                "- The driver is loaded");
        });
    }

    return app.exec();
}