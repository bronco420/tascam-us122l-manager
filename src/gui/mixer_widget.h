#ifndef MIXER_WIDGET_H
#define MIXER_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QVBoxLayout>
#include <QGroupBox>

class Mixer;

class MixerWidget : public QWidget {
    Q_OBJECT

public:
    explicit MixerWidget(Mixer* mixer, QWidget* parent = nullptr);
    ~MixerWidget();

    void show();

private slots:
    void onSinkVolumeChanged(int volume);
    void onSourceVolumeChanged(int volume);
    void onSinkMuteChanged(bool muted);
    void onSourceMuteChanged(bool muted);
    void onSinkUp();
    void onSinkDown();
    void onSinkMute();
    void onSourceUp();
    void onSourceDown();
    void onSourceMute();

private:
    void updateVolumes();

    Mixer* m_mixer;

    QLabel* m_sinkLabel;
    QProgressBar* m_sinkBar;
    QSlider* m_sinkSlider;
    QPushButton* m_sinkUp;
    QPushButton* m_sinkDown;
    QPushButton* m_sinkMute;

    QLabel* m_sourceLabel;
    QProgressBar* m_sourceBar;
    QSlider* m_sourceSlider;
    QPushButton* m_sourceUp;
    QPushButton* m_sourceDown;
    QPushButton* m_sourceMute;
};

#endif // MIXER_WIDGET_H