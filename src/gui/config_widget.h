#ifndef CONFIG_WIDGET_H
#define CONFIG_WIDGET_H

#include <QWidget>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>

class Config;

class ConfigWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConfigWidget(Config& config, QWidget* parent = nullptr);
    ~ConfigWidget();

    void show();

private slots:
    void onSaveClicked();

signals:
    void applyClicked();

private:
    Config& m_config;

    QComboBox* m_sampleRate;
    QComboBox* m_bufferSize;
    QSpinBox* m_periods;
    QPushButton* m_btnSave;
    QPushButton* m_btnApply;
};

#endif // CONFIG_WIDGET_H