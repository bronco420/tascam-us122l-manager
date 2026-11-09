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
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

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

// Qt ships qtbase_it.qm but the catalog does not cover every parser string;
// this fills the remaining gaps so --help is fully Italian.
class CliTranslator : public QTranslator {
public:
    QString translate(const char* context, const char* sourceText,
                      const char* disambiguation = nullptr,
                      int n = -1) const override {
        Q_UNUSED(context);
        Q_UNUSED(disambiguation);
        Q_UNUSED(n);
        const QString s = QString::fromUtf8(sourceText);
        if (s == QLatin1String("Displays help on commandline options.")) {
            return QStringLiteral("Mostra la guida sulle opzioni della riga di comando.");
        }
        if (s == QLatin1String("Displays help, including generic Qt options.")) {
            return QStringLiteral("Mostra la guida, incluse le opzioni generiche di Qt.");
        }
        return QString();
    }
};


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

    // Load Qt's own Italian translations so that the parser's built-in help
    // and error strings ("Utilizzo:", "Valore mancante dopo...") stay in the
    // same language as the rest of the CLI.
    QTranslator qtTranslator;
    const QString qtTrPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (qtTranslator.load(QLocale(), "qtbase", "_", qtTrPath)) {
        app.installTranslator(&qtTranslator);
    }
    CliTranslator cliTranslator;
    app.installTranslator(&cliTranslator);

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Tascam US-122L Manager - Controllo interfaccia audio professionale");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dashboardOption("dashboard", "Apre il pannello di controllo (dashboard Qt)");
    parser.addOption(dashboardOption);

    QCommandLineOption startOption("start", "Avvia JACK con le impostazioni salvate");
    parser.addOption(startOption);

    QCommandLineOption stopOption("stop", "Ferma JACK");
    parser.addOption(stopOption);

    QCommandLineOption restartOption("restart", "Riavvia JACK");
    parser.addOption(restartOption);

    QCommandLineOption statusOption("status", "Mostra lo stato (testo)");
    parser.addOption(statusOption);

    QCommandLineOption diagOption("diag", "Mostra la diagnostica completa (report)");
    parser.addOption(diagOption);

    QCommandLineOption bridgeOption("bridge",
        "Bridge PipeWire-JACK (on|off|toggle). Richiede JACK attivo.",
        "mode");
    parser.addOption(bridgeOption);

    QCommandLineOption sysmodeOption("sysmode",
        "Usa la Tascam come scheda di sistema (on|off|toggle). Esclusivo con JACK.",
        "mode");
    parser.addOption(sysmodeOption);

    QCommandLineOption watchOption("watch", "Watchdog: riavvia JACK se si interrompe");
    parser.addOption(watchOption);

    QCommandLineOption watchStopOption("watch-stop", "Ferma il watchdog");
    parser.addOption(watchStopOption);

    QCommandLineOption autostartOption("autostart",
        "Avvio automatico di JACK al login (on|off)",
        "mode");
    parser.addOption(autostartOption);

    QCommandLineOption silentOption("silent", "Nessuna finestra GUI (per systemd/cron)");
    parser.addOption(silentOption);

    QCommandLineOption screenshotOption("screenshot",
        "Salva uno screenshot della GUI nel FILE indicato (per la documentazione)",
        "file");
    parser.addOption(screenshotOption);

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
        auto row = [](const QString& label, const QString& value) {
            std::cout << label.leftJustified(20, ' ').toStdString()
                      << value.toStdString() << "\n";
        };

        std::cout << "==================================================\n";
        std::cout << "  Tascam US-122L Manager - Stato\n";
        std::cout << "==================================================\n";

        auto model = diagnostics.getCardModel();
        auto cardNum = diagnostics.getCardNumber();
        auto usbConn = diagnostics.getUSBConnection();
        auto firmware = diagnostics.getFirmwareVersion();
        auto sampleWidth = diagnostics.getSampleWidth();
        auto midiClient = diagnostics.getMidiClientName();

        row("Dispositivo:", model + QString(" (scheda %1)").arg(
                cardNum.isEmpty() ? QString("?") : cardNum));
        row("Connessione:", usbConn);
        row("Firmware:", firmware + " | Campionamento: " + sampleWidth);
        row("MIDI dispositivo:", midiClient);

        if (!cardNum.isEmpty()) {
            row("Driver:", jackManager.isDriverLoaded()
                    ? "caricato" : "NON caricato");
        }

        row("Server JACK:", jackManager.isRunning() ? "ATTIVO" : "INATTIVO");

        if (jackManager.isRunning()) {
            auto sr = jackManager.getSampleRate();
            auto buf = jackManager.getBufferSize();
            auto pid = jackManager.getPID();

            row("  Frequenza:", sr + " Hz");
            row("  Buffer:", buf + " frame");
            row("  Periodi:", jackManager.getPeriods());
            row("  PID:", pid);
        }

        row("Bridge PipeWire:", pwBridge.isActive() ? "ON" : "OFF");

        QString sysmodeState;
        if (sysmode.isFunctional()) {
            sysmodeState = "ATTIVO (uscita Tascam)";
        } else if (sysmode.isActive()) {
            sysmodeState = "DEGRADATO (moduli incompleti: esegui "
                           "'--sysmode off' poi '--sysmode on')";
        } else {
            sysmodeState = "OFF";
        }
        row("Sysmode:", sysmodeState);

        row("Watchdog:", watchdog.isRunning() ? "attivo" : "fermo");
        row(".asoundrc:", config.asoundrcExists()
                ? "configurato" : "NON configurato");
        row("Avvio automatico:", config.autostartEnabled()
                ? "abilitato" : "disabilitato");

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

        std::cout << "Avvio JACK (" << sr.toStdString() << "Hz / "
                  << buf.toStdString() << " / " << periods.toStdString() << ")...\n";
        // In CLI mode nothing shows the GUI error banner: capture the reason
        // emitted by JackManager and print it here.
        QString cliError;
        QObject::connect(&jackManager, &JackManager::errorOccurred,
                         [&cliError](const QString& message) { cliError = message; });
        if (!jackManager.start(sr, buf, periods)) {
            if (!cliError.isEmpty()) {
                std::cerr << cliError.toStdString() << "\n";
            } else {
                std::cerr << "Impossibile avviare JACK. Dettagli nel log: "
                             "~/.config/tascam-us122l/jack.log\n";
            }
            return 1;
        }
        std::cout << "JACK avviato (PID " << jackManager.getPID().toStdString() << ").\n";
        return 0;
    }

    if (parser.isSet("stop")) {
        jackManager.stop();
        std::cout << "JACK fermato.\n";
        return 0;
    }

    if (parser.isSet("restart")) {
        jackManager.stop();
        QThread::sleep(1);
        auto sr = config.getSampleRate();
        auto buf = config.getBufferSize();
        auto periods = config.getPeriods();
        QString cliError;
        QObject::connect(&jackManager, &JackManager::errorOccurred,
                         [&cliError](const QString& message) { cliError = message; });
        if (!jackManager.start(sr, buf, periods)) {
            std::cerr << (cliError.isEmpty()
                    ? QString("Impossibile riavviare JACK.")
                    : cliError).toStdString() << "\n";
            return 1;
        }
        std::cout << "JACK riavviato.\n";
        return 0;
    }

    if (parser.isSet("watch")) {
        if (watchdog.isRunning()) {
            std::cout << "Watchdog gia' attivo.\n";
            return 0;
        }
        watchdog.start();
        if (!watchdog.isRunning()) {
            std::cerr << "Impossibile avviare il watchdog.\n";
            return 1;
        }
        std::cout << "Watchdog avviato (PID "
                  << QCoreApplication::applicationPid() << ").\n";
        // Keep the process alive so the watchdog's QTimers can fire.
        return app.exec();
    }

    if (parser.isSet("watch-stop")) {
        if (watchdog.isRunning()) {
            watchdog.stop();
            std::cout << "Watchdog fermato.\n";
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
                    std::cout << "Watchdog (PID " << pid.toStdString() << ") fermato.\n";
                } else {
                    std::cerr << "Impossibile fermare il watchdog (PID " << pid.toStdString() << ").\n";
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
            std::cout << "Avvio automatico abilitato.\n";
        } else if (mode == "off") {
            config.setAutostart(false);
            std::cout << "Avvio automatico disabilitato.\n";
        } else {
            std::cerr << "Uso: --autostart on|off\n";
            return 1;
        }
        return 0;
    }

    if (parser.isSet("bridge")) {
        QString mode = parser.value("bridge");
        if (mode != "off" && !jackManager.isRunning()) {
            std::cerr << "Avvia prima JACK: il bridge richiede JACK attivo.\n";
            return 1;
        }
        if (mode == "on") {
            pwBridge.enable();
        } else if (mode == "off") {
            pwBridge.disable();
        } else {
            pwBridge.toggle();
        }
        const bool active = pwBridge.isActive();
        std::cout << (active ? "Bridge PipeWire: ON\n" : "Bridge PipeWire: OFF\n");
        if (!active && !pwBridge.lastError().isEmpty()) {
            std::cerr << pwBridge.lastError().toStdString() << "\n";
            return 1;
        }
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
        std::cout << (sysmode.isActive() ? "Sysmode: ATTIVO (uscita Tascam)\n" : "Sysmode: OFF\n");
        return 0;
    }


    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();

    // Screenshot mode: render the window, then save and exit
    if (parser.isSet("screenshot")) {
        const QString outFile = parser.value("screenshot");
        mainWindow.resize(1040, 680);
        QTimer::singleShot(1200, [&]() {
            const QPixmap pm = mainWindow.grab();
            if (!pm.save(outFile)) {
                std::cerr << "Impossibile salvare lo screenshot in " << outFile.toStdString() << "\n";
                qApp->exit(1);
            } else {
                std::cout << "Screenshot salvato in " << outFile.toStdString() << "\n";
                qApp->exit(0);
            }
        });
    }

    // Non-blocking warning if the card is not detected
    if (!diagnostics.isCardDetected()) {
        QTimer::singleShot(0, [&mainWindow]() {
            QMessageBox::warning(&mainWindow, "Attenzione",
                "Tascam US-122L non rilevata!\n\n"
                "Verifica che:\n"
                "- la scheda sia collegata via USB\n"
                "- nessun'altra applicazione la stia usando\n"
                "- il driver sia caricato");
        });
    }

    return app.exec();
}