#ifndef DIAGNOSTICS_WIDGET_H
#define DIAGNOSTICS_WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QGroupBox>

class Diagnostics;

class DiagnosticsWidget : public QWidget {
    Q_OBJECT

public:
    explicit DiagnosticsWidget(Diagnostics* diagnostics, QWidget* parent = nullptr);
    ~DiagnosticsWidget();

    void show();
    void refresh();

private:
    Diagnostics* m_diagnostics;

    QTextEdit* m_textEdit;
};

#endif // DIAGNOSTICS_WIDGET_H