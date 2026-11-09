#include "config_widget.h"
#include "core/config.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>

ConfigWidget::ConfigWidget(Config& config, QWidget* parent)
    : QWidget(parent), m_config(config) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Sample rate
    QGroupBox* sampleRateGroup = new QGroupBox("Frequenza di campionamento");
    QFormLayout* sampleRateLayout = new QFormLayout(sampleRateGroup);
    m_sampleRate = new QComboBox();
    m_sampleRate->addItem("44100 Hz (CD)", "44100");
    m_sampleRate->addItem("48000 Hz (Standard Video)", "48000");
    m_sampleRate->addItem("88200 Hz (Hi-Res)", "88200");
    m_sampleRate->addItem("96000 Hz (Studio)", "96000");
    int srIdx = m_sampleRate->findData(m_config.getSampleRate());
    m_sampleRate->setCurrentIndex(srIdx >= 0 ? srIdx : 0);
    sampleRateLayout->addRow("Frequenza:", m_sampleRate);

    // Buffer size
    QGroupBox* bufferSizeGroup = new QGroupBox("Dimensione buffer");
    QFormLayout* bufferSizeLayout = new QFormLayout(bufferSizeGroup);
    m_bufferSize = new QComboBox();
    m_bufferSize->addItem("64 frames (Ultra low latency)", "64");
    m_bufferSize->addItem("128 frames (Low latency)", "128");
    m_bufferSize->addItem("256 frames (Medium latency)", "256");
    m_bufferSize->addItem("512 frames (High latency)", "512");
    int bufIdx = m_bufferSize->findData(m_config.getBufferSize());
    m_bufferSize->setCurrentIndex(bufIdx >= 0 ? bufIdx : 0);
    bufferSizeLayout->addRow("Buffer:", m_bufferSize);

    // Periods
    QGroupBox* periodsGroup = new QGroupBox("Periodi");
    QFormLayout* periodsLayout = new QFormLayout(periodsGroup);
    m_periods = new QSpinBox();
    m_periods->setRange(2, 4);
    m_periods->setValue(m_config.getPeriods().toInt());
    periodsLayout->addRow("Periodi:", m_periods);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_btnSave = new QPushButton("Salva impostazioni");
    m_btnSave->setProperty("type", "primary");
    m_btnApply = new QPushButton("Applica e riavvia JACK");
    m_btnApply->setProperty("type", "ghost");
    buttonLayout->addWidget(m_btnSave);
    buttonLayout->addWidget(m_btnApply);

    mainLayout->addWidget(sampleRateGroup);
    mainLayout->addWidget(bufferSizeGroup);
    mainLayout->addWidget(periodsGroup);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_btnSave, &QPushButton::clicked, this, &ConfigWidget::onSaveClicked);
    connect(m_btnApply, &QPushButton::clicked, this, [this]() {
        onSaveClicked();
        emit applyClicked();
    });
}

ConfigWidget::~ConfigWidget() {
    // Cleanup
}

void ConfigWidget::show() {
    QWidget::show();
}

void ConfigWidget::onSaveClicked() {
    m_config.setSampleRate(m_sampleRate->currentData().toString());
    m_config.setBufferSize(m_bufferSize->currentData().toString());
    m_config.setPeriods(QString::number(m_periods->value()));

    QMessageBox::information(this, "Impostazioni salvate",
        "Impostazioni di JACK salvate. Premi 'Applica e riavvia JACK' per attivarle.");
}