#include "diagnostics_widget.h"
#include "core/diagnostics.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>

DiagnosticsWidget::DiagnosticsWidget(Diagnostics* diagnostics, QWidget* parent)
    : QWidget(parent), m_diagnostics(diagnostics) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Full diagnostics report
    QGroupBox* diagGroup = new QGroupBox("Full Diagnostics Report");
    QVBoxLayout* diagLayout = new QVBoxLayout(diagGroup);
    m_textEdit = new QTextEdit();
    m_textEdit->setReadOnly(true);
    m_textEdit->setMaximumHeight(600);
    diagLayout->addWidget(m_textEdit);

    mainLayout->addWidget(diagGroup);

    // Initial refresh
    refresh();
}

DiagnosticsWidget::~DiagnosticsWidget() {
    // Cleanup
}

void DiagnosticsWidget::show() {
    QWidget::show();
    refresh();
}

void DiagnosticsWidget::refresh() {
    m_textEdit->setText(m_diagnostics->generateReport());
}