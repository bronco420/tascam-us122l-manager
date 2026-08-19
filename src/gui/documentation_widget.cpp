#include "documentation_widget.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>

const QString DOCUMENTATION_TEXT = R"(
==============================================================
TASCAM US-122L MANAGER - DOCUMENTAZIONE COMPLETA
==============================================================

CHE COSA FA QUESTO PROGRAMMA?
-----------------------------
Gestisce l'interfaccia audio Tascam US-122L su Linux (CachyOS).
Permette di avviare/fermare JACK, configurare i parametri audio
e instradare l'audio del sistema verso la scheda professionale.

PERCHE' LA US-122L HA BISOGNO DI CONFIGURAZIONE SPECIALE?
---------------------------------------------------------
La Tascam US-122L NON è una scheda audio USB standard. Non espone
dispositivi PCM ALSA normali. Funziona SOLO tramite il plugin
speciale "usb_stream" di ALSA.

Significa che:
  - Non appare nelle impostazioni audio di sistema (PipeWire)
  - Non funziona con i metodi standard "hw:" o "default:"
  - Richiede obbligatoriamente JACK Audio Connection Kit

Questo comportamento è documentato da ALSA Project, Briata's Notes
e tutti i forum pro-audio dal 2008. NON È UN BUG.

SPECIFICHE TECNICHE DELLA SCHEDA
--------------------------------
Ingressi:
  - 2x XLR/TRS combo (microfono con phantom power + strumento)
  - 2x S/PDIF coassiali digitali (RCA)
  - 1x MIDI IN

Uscite:
  - 2x TRS principali (stereo)
  - 2x S/PDIF coassiali digitali (RCA)
  - 1x MIDI OUT

Caratteristiche:
  - Convertitori AD/DA a 24-bit
  - Sample rate: fino a 192kHz (nativa) / 96kHz (USB 2.0)
  - Buffer size programmabile via software

ARCHITETTURA AUDIO
------------------
[US-122L] --> ALSA usb_stream --> JACK server
                                  |--> Ardour, Reaper, Cakewalk
                                  \--> PipeWire bridge (browser, app)

[Integrato PCH] --> PipeWire --> Sistema, App normali

COME USARE QUESTO PROGRAMMA
---------------------------
1. AVVIO RAPIDO: Clicca "Avvia JACK" per iniziare a usare la scheda.
2. CONFIGURAZIONE: Imposta sample rate e buffer size.
3. BRIDGE: Per ascoltare l'audio del browser sulla US-122L.
4. DAW: Apri Ardour/Reaper - vedrai le porte JACK della US-122L.

MODALITA' CLI (terminali, script, systemd)
------------------------------------------
  manager.sh --start                Avvia JACK
  manager.sh --stop                 Ferma JACK
  manager.sh --status               Mostra stato testuale
  manager.sh --diag                 Mostra la diagnostica completa (report)
  manager.sh --bridge on|off|toggle Gestisce il bridge
  manager.sh --sysmode on|off|toggle Usa la Tascam come scheda di sistema (PipeWire)
  manager.sh --watch                Watchdog: riavvia JACK se cade
  manager.sh --watch-stop           Ferma il watchdog in background
  manager.sh --autostart on|off     Abilita/disabilita l'auto-start di JACK al login
  manager.sh --version              Mostra la versione
  manager.sh --help                 Aiuto completo

WATCHDOG / AUTO-START
---------------------
- Watchdog (--watch o dal menu): monitora jackd e lo riavvia
  automaticamente se termina (utile con device USB instabili).
  Se la scheda è scollegata, dopo 3 tentativi falliti rallenta
  i controlli (ogni 30s) per non caricare la CPU. Per fermarlo:
  --watch-stop oppure la voce "Watchdog" nel menu.
- Auto-start (--autostart on o dal menu): installa un servizio
  systemd utente che avvia il WATCHDOG al login (riavvia JACK in
  loop anche se la scheda è collegata dopo l'avvio). Restart
  automatico su failure.

PARAMETRI CONSIGLIATI
---------------------
Registrazione voce/strumenti: 48000 Hz | 128 | 3
Mixing/mastering:              96000 Hz | 512 | 4
Live bassa latenza:            48000 Hz | 64-128 | 2

XRUNS (buffer underrun/overrun)
-------------------------------
1. Aumenta il buffer size (128 -> 256 o 512)
2. Aumenta i periodi (2 -> 3 o 4)
3. Chiudi app che consumano molta CPU
4. Usa il kernel realtime di CachyOS
5. Verifica che l'utente sia nel gruppo 'audio' e i limiti RT attivi

SCHEDA NON RILEVATA
-------------------
1. Prova una porta USB 2.0 invece di USB 3.0
2. Aggiorna il firmware da tascam.com
3. Riavvia con la scheda collegata
4. Controlla /proc/asound/cards (la scheda deve mostrare "US122L")

JACK NON SI AVVIA
-----------------
1. Controlla che nessun'altra app usi la US-122L
2. Ferma eventuali altri jackd attivi
3. Verifica il log: ~/.config/tascam-us122l/jack.log
4. Prova periodi=3 o buffer=256

FILE DI CONFIGURAZIONE
----------------------
~/.asoundrc          - Plugin usb_stream per ALSA
~/.config/tascam-us122l/settings.conf - Impostazioni salvate
~/.config/tascam-us122l/jack.log     - Log di JACK
~/.config/systemd/user/tascam-us122l.service - Auto-start

RIFERIMENTI UTILI
------------------
ALSA Project:   https://www.alsa-project.org/main/index.php/Matrix:Module-usb-us122l
JACK Audio:     https://jackaudio.org
Arch BBS:       https://bbs.archlinux.org/viewtopic.php?id=124964
==============================================================
)";

DocumentationWidget::DocumentationWidget(QWidget* parent)
    : QWidget(parent) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* docGroup = new QGroupBox("Documentation");
    QVBoxLayout* docLayout = new QVBoxLayout(docGroup);
    m_textEdit = new QTextEdit();
    m_textEdit->setReadOnly(true);
    m_textEdit->setMaximumHeight(600);
    docLayout->addWidget(m_textEdit);

    mainLayout->addWidget(docGroup);

    // Initial content
    m_textEdit->setText(DOCUMENTATION_TEXT);
}

DocumentationWidget::~DocumentationWidget() {
    // Cleanup
}

void DocumentationWidget::show() {
    QWidget::show();
    refresh();
}

void DocumentationWidget::refresh() {
    m_textEdit->setText(DOCUMENTATION_TEXT);
}