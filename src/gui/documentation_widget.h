#ifndef DOCUMENTATION_WIDGET_H
#define DOCUMENTATION_WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QGroupBox>

class DocumentationWidget : public QWidget {
    Q_OBJECT

public:
    explicit DocumentationWidget(QWidget* parent = nullptr);
    ~DocumentationWidget();

    void show();
    void refresh();

private:
    QTextEdit* m_textEdit;
};

#endif // DOCUMENTATION_WIDGET_H