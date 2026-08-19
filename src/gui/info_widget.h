#ifndef INFO_WIDGET_H
#define INFO_WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QGroupBox>

class Diagnostics;

class InfoWidget : public QWidget {
    Q_OBJECT

public:
    explicit InfoWidget(Diagnostics* diagnostics, QWidget* parent = nullptr);
    ~InfoWidget();

    void show();
    void refresh();
    void showMidi();

private:
    Diagnostics* m_diagnostics;

    QTextEdit* m_cardText;
    QTextEdit* m_detailText;
};

#endif // INFO_WIDGET_H
