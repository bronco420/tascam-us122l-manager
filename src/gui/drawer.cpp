#include "drawer.h"
#include "theme_tokens.h"
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QScrollBar>
#include <QScrollArea>

Drawer::Drawer(QWidget* parent)
    : QFrame(parent) {
    setFixedSize(DRAWER_WIDTH, parent ? parent->height() : 600);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
   setAttribute(Qt::WA_TranslucentBackground);

    // Outer layout
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header bar
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(16, 12, 16, 12);
    headerLayout->setSpacing(8);

    auto* closeBtn = new QPushButton("✕", this);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet("QPushButton { "
        "background-color: #1C2327; "
        "border: 1px solid #263136; "
        "border-radius: 4px; "
        "color: #A7B4BA; "
        "font-size: 12px; } "
        "QPushButton:hover { background-color: #232C31; color: #E8EEF1; }");
    connect(closeBtn, &QPushButton::clicked, this, &Drawer::onCloseClicked);
    headerLayout->addWidget(closeBtn);

    auto* titleLabel = new QLabel("Strumenti", this);
    titleLabel->setFont(UiTheme::fontLabel());
    titleLabel->setStyleSheet("color: #E8EEF1; font-size: 14px;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    mainLayout->addLayout(headerLayout);

    // Divider
    auto* divider = new QWidget();
    divider->setFixedHeight(1);
    divider->setStyleSheet("background-color: #263136;");
    mainLayout->addWidget(divider);

    // Scrollable content area
    m_scroller = new QWidget(this);
    m_contentLayout = new QVBoxLayout(m_scroller);
    m_contentLayout->setContentsMargins(12, 8, 12, 12);
    m_contentLayout->setSpacing(4);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(m_scroller);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }"
                              "QScrollBar:vertical { background-color: transparent; width: 6px; margin: 0; }"
                              "QScrollBar::handle:vertical { background-color: #263136; border-radius: 3px; min-height: 16px; }"
                              "QScrollBar::handle:vertical:hover { background-color: #35444B; }");
    mainLayout->addWidget(scrollArea);

    // Animation for sliding in/out
    m_animation = new QPropertyAnimation(this, "pos", this);
    m_animation->setDuration(ANIM_DURATION);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_animation, &QPropertyAnimation::finished, this, &Drawer::onAnimationFinished);
}

void Drawer::addSection(const QString& title, const QStringList& items) {
    Section sec;
    sec.title = title;
    sec.items = items;
    m_sections << sec;

    // Title
    auto* sectionTitle = new QLabel(title, m_scroller);
    sectionTitle->setFont(UiTheme::fontLabel());
    sectionTitle->setStyleSheet("color: #A7B4BA; font-size: 10px; letter-spacing: 1.5px; padding: 8px 0 4px 4px;");
    m_contentLayout->addWidget(sectionTitle);

    // Items
    for (const auto& item : items) {
        auto* btn = new QPushButton(item, m_scroller);
        btn->setMinimumHeight(36);
        btn->setStyleSheet("QPushButton { "
            "background-color: #1C2327; "
            "border: 1px solid transparent; "
            "border-radius: 4px; "
            "color: #E8EEF1; "
            "text-align: left; "
            "padding-left: 12px; "
            "font-size: 12px; } "
            "QPushButton:hover { "
            "background-color: #232C31; "
            "border-color: #263136; }");
        int idx = m_sections.last().items.indexOf(item);
        connect(btn, &QPushButton::clicked, this, [this, idx]() { onItemClicked(idx); });
        m_contentLayout->addWidget(btn);
    }

    // Spacer at bottom
    m_contentLayout->addStretch();
}

void Drawer::show() {
    if (m_shown) return;
    m_shown = true;

    QWidget* parent = parentWidget();
    if (!parent) return;

    int startX = parent->width();  // start off-screen right
    move(startX, 0);
    setWindowOpacity(1.0);
    show();

    // Animate to positioned location (behind the main window edge)
    int targetX = parent->width() - width();
    m_animation->setStartValue(pos());
    m_animation->setEndValue(QPoint(targetX, 0));
    m_animation->start();
}

void Drawer::hide() {
    if (!m_shown) return;
    m_shown = false;

    int targetX = parentWidget() ? parentWidget()->width() : width();
    m_animation->setStartValue(pos());
    m_animation->setEndValue(QPoint(targetX, 0));
    m_animation->start();
}

void Drawer::onItemClicked(int index) {
    // Build action string: "section:index" or just the item text
    QString sectionName = m_sections.isEmpty() ? "" : m_sections.last().title;
    QString itemName = m_sections.isEmpty() || index < 0 || index >= m_sections.last().items.size()
                      ? QString() : m_sections.last().items[index];

    emit itemSelected(itemName);
    hide();
}

void Drawer::onCloseClicked() { hide(); }

void Drawer::onAnimationFinished() {
    if (!m_shown) {
        hide();  // hide() when animation done (fully off-screen)
    }
}

void Drawer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    // Semi-transparent scrim behind drawer
    p.fillRect(rect(), QColor(0x0E, 0x11, 0x13));
    // Right border line
    p.setPen(QColor(0x26, 0x31, 0x36));
    p.drawLine(0, 0, 0, height());
}
