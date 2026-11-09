#include "theme_tokens.h"
#include <QApplication>
#include <QStyleFactory>
#include <QFontDatabase>

// ---- Fonts ----
static QFont s_fontPrimary;
static QFont s_fontMono;
static QFont s_fontLabel;
static QFont s_caption;

static void initFonts() {
    // Primary: attempt Inter → Fira Sans → Cantarell → sans-serif fallback
    s_fontPrimary = QFont("Inter", 13);
    if (!s_fontPrimary.family().contains("Inter", Qt::CaseInsensitive) &&
        !QFontDatabase::families().contains("Inter")) {
        s_fontPrimary = QFont("Fira Sans", 13);
    }
    if (!QFontDatabase::families().contains(s_fontPrimary.family())) {
        s_fontPrimary = QFont("Cantarell", 13);
    }
    s_fontPrimary.setWeight(QFont::Weight::Medium);

    // Label: slightly heavier
    s_fontLabel = s_fontPrimary;
    s_fontLabel.setWeight(QFont::Weight::Bold);
    s_fontLabel.setPointSize(s_fontPrimary.pointSize());

    // Mono for values/numbers
    s_fontMono = QFont("Fira Code", 12);
    if (!QFontDatabase::families().contains("Fira Code")) {
        s_fontMono = QFont("Droid Sans Mono", 12);
    }
    if (!QFontDatabase::families().contains(s_fontMono.family())) {
        s_fontMono = QFont("DejaVu Sans Mono", 12);
    }

    // Caption
    s_caption = QFont(s_fontPrimary.family(), 10);
    s_caption.setWeight(QFont::Weight::Normal);
}

QFont UiTheme::fontPrimary() {
    static bool once = [](){ initFonts(); return true; }();
    Q_UNUSED(once);
    return s_fontPrimary;
}

QFont UiTheme::fontMono() {
    static bool once = [](){ initFonts(); return true; }();
    Q_UNUSED(once);
    return s_fontMono;
}

QFont UiTheme::fontLabel() {
    static bool once = [](){ initFonts(); return true; }();
    Q_UNUSED(once);
    return s_fontLabel;
}

QFont UiTheme::fontCaption() {
    static bool once = [](){ initFonts(); return true; }();
    Q_UNUSED(once);
    return s_caption;
}

// ---- QPainter helpers ----
void UiTheme::drawRaisedPanel(QPainter* p, const QRect& rect) {
    // Fill with raised panel color
    p->fillRect(rect, bgPanelRaised());
}

void UiTheme::drawSubtleBorder(QPainter* p, const QRect& rect) {
    QPen pen(borderSubtle());
    pen.setWidth(1);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawRect(rect.adjusted(0, 0, -1, -1));
}

QColor UiTheme::accentWithAlpha(int alpha) {
    auto c = accentDefault();
    c.setAlpha(qBound(0, alpha, 255));
    return c;
}

QColor UiTheme::stateWithAlpha(int state, int alpha) {
    QColor c;
    switch (state) {
        case 0: c = stateOk(); break;
        case 1: c = stateWarn(); break;
        case 2: c = stateError(); break;
        case 3: c = stateIdle(); break;
        case 4: c = stateInfo(); break;
        default: c = textMuted(); break;
    }
    c.setAlpha(qBound(0, alpha, 255));
    return c;
}

// ---- Global QSS ----
QString UiTheme::globalQss() {
    return R"(
/* ── General widget overrides ─────────────────────────── */
QWidget {
    background-color: #151A1D;
    color: #E8EEF1;
    font-family: Cantarell, Inter, sans-serif;
    font-size: 13px;
}

/* Menu bar & menus */
QMenuBar {
    background-color: #0E1113;
    color: #A7B4BA;
    border-bottom: 1px solid #263136;
    padding: 2px 0;
    font-size: 12px;
}
QMenuBar::item {
    padding: 4px 10px;
    border-radius: 4px;
    margin: 2px 2px;
}
QMenuBar::item:selected {
    background-color: #232C31;
    color: #E8EEF1;
}
QMenu {
    background-color: #1C2327;
    border: 1px solid #263136;
    border-radius: 6px;
    padding: 4px;
    color: #E8EEF1;
    font-size: 12px;
}
QMenu::item {
    padding: 6px 24px 6px 16px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: #232C31;
}
QMenu::separator {
    height: 1px;
    background: #263136;
    margin: 4px 8px;
}

/* Push buttons */
QPushButton {
    background-color: #1C2327;
    border: 1px solid #263136;
    border-radius: 6px;
    padding: 6px 16px;
    color: #E8EEF1;
    font-weight: bold;
    font-size: 12px;
}
QPushButton:hover {
    background-color: #232C31;
    border-color: #35444B;
}
QPushButton:pressed {
    background-color: #0A0D0F;
}
QPushButton:disabled {
    background-color: #0E1113;
    color: #6B7B82;
    border-color: #263136;
}

/* Accent button variant (set via setStyleSheet on individual buttons) */
QPushButton[accent="true"] {
    background-color: #0F766E;
    border-color: #14B8A6;
    color: #E8EEF1;
}
QPushButton[accent="true"]:hover {
    background-color: #14B8A6;
    border-color: #2DD4BF;
}
QPushButton[accent="true"]:pressed {
    background-color: #0D5C53;
}

/* Tool buttons */
QToolButton {
    background-color: transparent;
    border: none;
    color: #A7B4BA;
    padding: 6px 10px;
    border-radius: 4px;
}
QToolButton:hover {
    background-color: #232C31;
    color: #E8EEF1;
}

/* Group boxes */
QGroupBox {
    color: #A7B4BA;
    font-weight: bold;
    font-size: 12px;
    border: 1px solid #263136;
    border-radius: 6px;
    margin-top: 8px;
    padding-top: 16px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 10px;
    padding: 0 4px;
    color: #A7B4BA;
}

/* Checkboxes */
QCheckBox {
    color: #E8EEF1;
    font-size: 12px;
    spacing: 6px;
}
QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid #35444B;
    border-radius: 4px;
    background-color: #0E1113;
}
QCheckBox::indicator:checked {
    background-color: #14B8A6;
    border-color: #2DD4BF;
}

/* Radio buttons */
QRadioButton {
    color: #E8EEF1;
    font-size: 12px;
    spacing: 6px;
}
QRadioButton::indicator {
    width: 16px;
    height: 16px;
}
QRadioButton::indicator::unchecked {
    border: 1px solid #35444B;
    border-radius: 8px;
    background-color: #0E1113;
}
QRadioButton::indicator::checked {
    border: 2px solid #14B8A6;
    border-radius: 8px;
    background-color: #14B8A6;
}

/* Scroll bars */
QScrollBar:vertical {
    background-color: transparent;
    width: 8px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background-color: #263136;
    border-radius: 4px;
    min-height: 20px;
}
QScrollBar::handle:vertical:hover {
    background-color: #35444B;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    background-color: transparent;
    height: 8px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background-color: #263136;
    border-radius: 4px;
    min-width: 20px;
}
QScrollBar::handle:horizontal:hover {
    background-color: #35444B;
}

/* Combo boxes */
QComboBox {
    background-color: #0E1113;
    border: 1px solid #35444B;
    border-radius: 4px;
    padding: 4px 8px;
    color: #E8EEF1;
    font-size: 12px;
}
QComboBox::drop-down {
    border: none;
    width: 16px;
}
QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #A7B4BA;
    margin-right: 4px;
}
QComboBox QAbstractItemView {
    background-color: #1C2327;
    border: 1px solid #263136;
    border-radius: 4px;
    selection-background-color: #232C31;
    color: #E8EEF1;
    outline: none;
    padding: 4px;
}

/* Spin boxes */
QSpinBox, QDoubleSpinBox {
    background-color: #0E1113;
    border: 1px solid #35444B;
    border-radius: 4px;
    padding: 4px 8px;
    color: #E8EEF1;
    font-size: 12px;
}
QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    background-color: transparent;
    border: none;
    width: 16px;
}

/* Progress bars */
QProgressBar {
    background-color: #0E1113;
    border: 1px solid #263136;
    border-radius: 4px;
    text-align: center;
    color: #E8EEF1;
    font-size: 11px;
    height: 16px;
}
QProgressBar::chunk {
    background-color: #14B8A6;
    border-radius: 3px;
}

/* Text edit / text box */
QTextEdit, QLineEdit {
    background-color: #0E1113;
    border: 1px solid #35444B;
    border-radius: 4px;
    padding: 6px 8px;
    color: #E8EEF1;
    font-size: 12px;
}
QTextEdit:focus, QLineEdit:focus {
    border-color: #14B8A6;
}

/* Status bar */
QStatusBar {
    background-color: #0E1113;
    color: #6B7B82;
    font-size: 11px;
    border-top: 1px solid #263136;
}
QStatusBar QLabel {
    color: #6B7B82;
}

/* Tool tip */
QToolTip {
    background-color: #1C2327;
    color: #E8EEF1;
    border: 1px solid #35444B;
    border-radius: 4px;
    padding: 4px 8px;
}

/* Tab widget (if used) */
QTabWidget::pane {
    border: 1px solid #263136;
    border-radius: 6px;
    background-color: #151A1D;
}
QTabBar::tab {
    background-color: #0E1113;
    color: #6B7B82;
    padding: 8px 16px;
    border-top-left-radius: 4px;
    border-top-right-radius: 4px;
}
QTabBar::tab:selected {
    background-color: #1C2327;
    color: #E8EEF1;
    border-bottom: 2px solid #14B8A6;
}
)"  ;
}
