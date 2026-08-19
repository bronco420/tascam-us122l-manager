#include "mixer_widget.h"
#include "core/mixer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QSlider>
#include <QSignalBlocker>
#include <QStyle>

MixerWidget::MixerWidget(Mixer* mixer, QWidget* parent)
    : QWidget(parent), m_mixer(mixer) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Sink controls
    QGroupBox* sinkGroup = new QGroupBox("Output Volume");
    QVBoxLayout* sinkLayout = new QVBoxLayout(sinkGroup);

    QHBoxLayout* sinkVolumeLayout = new QHBoxLayout();
    m_sinkLabel = new QLabel("0%");
    m_sinkBar = new QProgressBar();
    m_sinkBar->setRange(0, 153);
    m_sinkBar->setValue(0);
    m_sinkSlider = new QSlider(Qt::Horizontal);
    m_sinkSlider->setRange(0, 153);
    m_sinkSlider->setValue(0);
    connect(m_sinkSlider, &QSlider::valueChanged, this, [this](int v) {
        if (m_mixer->isSinkAvailable()) {
            m_mixer->setSinkVolume(v);
        }
    });
    sinkVolumeLayout->addWidget(m_sinkLabel);
    sinkVolumeLayout->addWidget(m_sinkBar, 1);
    sinkLayout->addLayout(sinkVolumeLayout);
    sinkLayout->addWidget(m_sinkSlider);

    QHBoxLayout* sinkButtonLayout = new QHBoxLayout();
    m_sinkUp = new QPushButton("+");
    m_sinkDown = new QPushButton("-");
    m_sinkMute = new QPushButton("Mute");
    m_sinkUp->setProperty("type", "ghost");
    m_sinkDown->setProperty("type", "ghost");
    sinkButtonLayout->addWidget(m_sinkDown);
    sinkButtonLayout->addWidget(m_sinkMute);
    sinkButtonLayout->addWidget(m_sinkUp);
    sinkLayout->addLayout(sinkButtonLayout);

    // Source controls
    QGroupBox* sourceGroup = new QGroupBox("Input Volume");
    QVBoxLayout* sourceLayout = new QVBoxLayout(sourceGroup);

    QHBoxLayout* sourceVolumeLayout = new QHBoxLayout();
    m_sourceLabel = new QLabel("0%");
    m_sourceBar = new QProgressBar();
    m_sourceBar->setRange(0, 153);
    m_sourceBar->setValue(0);
    m_sourceSlider = new QSlider(Qt::Horizontal);
    m_sourceSlider->setRange(0, 153);
    m_sourceSlider->setValue(0);
    connect(m_sourceSlider, &QSlider::valueChanged, this, [this](int v) {
        if (m_mixer->isSourceAvailable()) {
            m_mixer->setSourceVolume(v);
        }
    });
    sourceVolumeLayout->addWidget(m_sourceLabel);
    sourceVolumeLayout->addWidget(m_sourceBar, 1);
    sourceLayout->addLayout(sourceVolumeLayout);
    sourceLayout->addWidget(m_sourceSlider);

    QHBoxLayout* sourceButtonLayout = new QHBoxLayout();
    m_sourceUp = new QPushButton("+");
    m_sourceDown = new QPushButton("-");
    m_sourceMute = new QPushButton("Mute");
    m_sourceUp->setProperty("type", "ghost");
    m_sourceDown->setProperty("type", "ghost");
    sourceButtonLayout->addWidget(m_sourceDown);
    sourceButtonLayout->addWidget(m_sourceMute);
    sourceButtonLayout->addWidget(m_sourceUp);
    sourceLayout->addLayout(sourceButtonLayout);

    mainLayout->addWidget(sinkGroup);
    mainLayout->addWidget(sourceGroup);
    mainLayout->addStretch();

    // Connect signals
    connect(m_mixer, &Mixer::sinkVolumeChanged, this, &MixerWidget::onSinkVolumeChanged);
    connect(m_mixer, &Mixer::sourceVolumeChanged, this, &MixerWidget::onSourceVolumeChanged);
    connect(m_mixer, &Mixer::sinkMuteChanged, this, &MixerWidget::onSinkMuteChanged);
    connect(m_mixer, &Mixer::sourceMuteChanged, this, &MixerWidget::onSourceMuteChanged);

    connect(m_sinkUp, &QPushButton::clicked, this, &MixerWidget::onSinkUp);
    connect(m_sinkDown, &QPushButton::clicked, this, &MixerWidget::onSinkDown);
    connect(m_sinkMute, &QPushButton::clicked, this, &MixerWidget::onSinkMute);
    connect(m_sourceUp, &QPushButton::clicked, this, &MixerWidget::onSourceUp);
    connect(m_sourceDown, &QPushButton::clicked, this, &MixerWidget::onSourceDown);
    connect(m_sourceMute, &QPushButton::clicked, this, &MixerWidget::onSourceMute);

    // Initial update
    updateVolumes();
}

MixerWidget::~MixerWidget() {
    // Cleanup
}

void MixerWidget::show() {
    QWidget::show();
    updateVolumes();
}

void MixerWidget::onSinkVolumeChanged(int volume) {
    QSignalBlocker b(m_sinkSlider);
    m_sinkBar->setValue(volume);
    m_sinkSlider->setValue(volume);
    m_sinkLabel->setText(QString("%1%").arg(volume));
}

void MixerWidget::onSourceVolumeChanged(int volume) {
    QSignalBlocker b(m_sourceSlider);
    m_sourceBar->setValue(volume);
    m_sourceSlider->setValue(volume);
    m_sourceLabel->setText(QString("%1%").arg(volume));
}

void MixerWidget::onSinkMuteChanged(bool muted) {
    m_sinkMute->setText(muted ? "Unmute" : "Mute");
    m_sinkMute->setProperty("type", muted ? "danger" : "ghost");
    m_sinkMute->style()->unpolish(m_sinkMute);
    m_sinkMute->style()->polish(m_sinkMute);
}

void MixerWidget::onSourceMuteChanged(bool muted) {
    m_sourceMute->setText(muted ? "Unmute" : "Mute");
    m_sourceMute->setProperty("type", muted ? "danger" : "ghost");
    m_sourceMute->style()->unpolish(m_sourceMute);
    m_sourceMute->style()->polish(m_sourceMute);
}

void MixerWidget::onSinkUp() {
    m_mixer->adjustSinkVolume(5);
}

void MixerWidget::onSinkDown() {
    m_mixer->adjustSinkVolume(-5);
}

void MixerWidget::onSinkMute() {
    m_mixer->toggleSinkMute();
}

void MixerWidget::onSourceUp() {
    m_mixer->adjustSourceVolume(5);
}

void MixerWidget::onSourceDown() {
    m_mixer->adjustSourceVolume(-5);
}

void MixerWidget::onSourceMute() {
    m_mixer->toggleSourceMute();
}

void MixerWidget::updateVolumes() {
    onSinkVolumeChanged(m_mixer->getSinkVolume());
    onSourceVolumeChanged(m_mixer->getSourceVolume());
}
