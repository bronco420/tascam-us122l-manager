#ifndef PRESET_WIDGET_H
#define PRESET_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>

class Preset;

class PresetWidget : public QWidget {
    Q_OBJECT

public:
    explicit PresetWidget(Preset* preset, QWidget* parent = nullptr);
    ~PresetWidget();

    void show();

private:
    Preset* m_preset;
    QGroupBox* m_presetStudio;
    QGroupBox* m_presetStandard;
    QGroupBox* m_presetLive;
    QGroupBox* m_presetHiRes;
};

#endif // PRESET_WIDGET_H