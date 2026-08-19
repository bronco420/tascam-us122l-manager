#include "preset_widget.h"
#include "core/preset.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>

PresetWidget::PresetWidget(Preset* preset, QWidget* parent)
    : QWidget(parent), m_preset(preset) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    auto addPresetCard = [this](const QString& name, const QString& desc,
                                const QString& key, QGroupBox*& group) {
        group = new QGroupBox(name);
        QVBoxLayout* layout = new QVBoxLayout(group);

        QLabel* infoLabel = new QLabel(desc);
        infoLabel->setProperty("muted", true);
        layout->addWidget(infoLabel);

        QPushButton* btn = new QPushButton("Apply Preset");
        btn->setProperty("type", "primary");
        layout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, key, name]() {
            if (m_preset->applyPreset(key)) {
                QMessageBox::information(this, "Preset Applied",
                    QString("%1 preset applied successfully!").arg(name));
            } else {
                QMessageBox::critical(this, "Error",
                    QString("Failed to apply %1 preset.").arg(name));
            }
        });

        return group;
    };

    QGroupBox* studioGroup = nullptr;
    m_presetStudio = addPresetCard("Studio", "44.1 kHz · buffer 256 · 3 periodi", "studio", studioGroup);
    mainLayout->addWidget(studioGroup);

    QGroupBox* standardGroup = nullptr;
    m_presetStandard = addPresetCard("Standard", "48 kHz · buffer 128 · 2 periodi", "standard", standardGroup);
    mainLayout->addWidget(standardGroup);

    QGroupBox* liveGroup = nullptr;
    m_presetLive = addPresetCard("Live", "48 kHz · buffer 64 · 3 periodi", "live", liveGroup);
    mainLayout->addWidget(liveGroup);

    QGroupBox* hiResGroup = nullptr;
    m_presetHiRes = addPresetCard("Hi-Res", "96 kHz · buffer 256 · 3 periodi", "hi-res", hiResGroup);
    mainLayout->addWidget(hiResGroup);

    mainLayout->addStretch();
}

PresetWidget::~PresetWidget() {
    // Cleanup
}

void PresetWidget::show() {
    QWidget::show();
}
