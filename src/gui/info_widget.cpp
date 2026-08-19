#include "info_widget.h"
#include "core/diagnostics.h"
#include "core/utils.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>

InfoWidget::InfoWidget(Diagnostics* diagnostics, QWidget* parent)
    : QWidget(parent), m_diagnostics(diagnostics) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Card info
    QGroupBox* cardGroup = new QGroupBox("Card Info");
    QVBoxLayout* cardLayout = new QVBoxLayout(cardGroup);
    m_cardText = new QTextEdit();
    m_cardText->setReadOnly(true);
    m_cardText->setMaximumHeight(240);
    cardLayout->addWidget(m_cardText);

    // MIDI details
    QGroupBox* midiGroup = new QGroupBox("MIDI");
    QVBoxLayout* midiLayout = new QVBoxLayout(midiGroup);
    m_detailText = new QTextEdit();
    m_detailText->setReadOnly(true);
    m_detailText->setMaximumHeight(120);
    midiLayout->addWidget(m_detailText);

    mainLayout->addWidget(cardGroup);
    mainLayout->addWidget(midiGroup);
    mainLayout->addStretch();

    // Initial refresh
    refresh();
}

InfoWidget::~InfoWidget() {
    // Cleanup
}

void InfoWidget::show() {
    QWidget::show();
    refresh();
}

void InfoWidget::refresh() {
    m_cardText->setText(m_diagnostics->getCardInfo());

    QString midi = m_diagnostics->getMidiClientName().trimmed();
    m_detailText->setText(midi.isEmpty()
        ? "No MIDI client detected."
        : QString("Client MIDI: %1\n\nPorta: 128:0 (Tascam US-122L)").arg(midi));
}

void InfoWidget::showMidi() {
    QWidget::show();
    QString info = Utils::getMidiInfo();
    m_cardText->setText(QString("=== MIDI - Tascam US-122L ===\n\n%1\n\n"
        "Usa 'Test MIDI' per inviare una nota di prova Do4.").arg(info));
    m_detailText->setText(m_diagnostics->getCardInfo());
}
