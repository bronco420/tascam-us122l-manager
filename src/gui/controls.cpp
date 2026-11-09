#include "controls.h"
#include "theme_tokens.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <cmath>

// ──────────────────────────────────────────────
// ArcKnob
// ──────────────────────────────────────────────
ArcKnob::ArcKnob(QWidget* parent)
    : QWidget(parent), m_fontLabel(UiTheme::fontPrimary()), m_fontValue(UiTheme::fontMono()) {
    setMouseTracking(true);
    setMinimumSize(80, 92);
}

void ArcKnob::setValue(int v) {
    v = qBound(m_min, v, m_max);
    if (v == m_value) return;
    m_value = v;
    emit valueChanged(m_value);
    update();
}

void ArcKnob::setLabel(const QString& label) { m_label = label; }
void ArcKnob::setUnits(const QString& units) { m_units = units; }

int ArcKnob::valueFromAngle(double angle) const {
    // angle: 0 = top (−π/2), sweeps clockwise to 270° arc
    double normalized = (angle + M_PI / 2.0) / (3.0 * M_PI / 2.0);
    normalized = qBound(0.0, normalized, 1.0);
    return m_min + int(normalized * (m_max - m_min));
}

double ArcKnob::angleFromValue(int v) const {
    double t = (v - m_min) / double(m_max - m_min);
    return t * (3.0 * M_PI / 2.0) - M_PI / 2.0;
}

void ArcKnob::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() != Qt::LeftButton) return;
    m_dragging = true;
    m_lastY = ev->y();
    // Click vertically to set value: top → max, bottom → min
    int v = positionToValue(ev->pos().y());
    setValue(v);
}

void ArcKnob::mouseMoveEvent(QMouseEvent* ev) {
    if (!m_dragging) return;
    int dy = m_lastY - ev->y();  // drag up = increase
    int range = m_max - m_min;
    int step = qMax(1, range / 20);
    setValue(m_value + (dy > 0 ? step : -step));
    m_lastY = ev->y();
}

void ArcKnob::mouseReleaseEvent(QMouseEvent* ev) {
    Q_UNUSED(ev);
    if (m_dragging) {
        m_dragging = false;
        emit released();
    }
}

void ArcKnob::leaveEvent(QEvent*) {
    if (m_dragging) {
        m_dragging = false;
        emit released();
    }
}

int ArcKnob::positionToValue(int y) const {
    int h = height();
    double t = 1.0 - (double(y) / h);  // top=1, bottom=0
    return m_min + int(t * (m_max - m_min));
}

void ArcKnob::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();
    int size = qMin(w, h);
    QRect arcRect(0, 0, size, size);

    // Background arc track
    double startAngle = -M_PI / 2.0;   // top
    double sweepAngle = 3.0 * M_PI / 2.0; // 270° clockwise

    int trackWidth = 6;
    QColor trackColor = QColor(0x1C, 0x23, 0x27);
    QColor activeColor = UiTheme::accentDefault();
    QColor inactiveColor = QColor(0x0A, 0x0D, 0x0F);

    // Draw full arc track
    p.setPen(QPen(trackColor, trackWidth, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(arcRect.adjusted(trackWidth/2, trackWidth/2, -trackWidth/2, -trackWidth/2),
              int(startAngle * 180 / M_PI * 16), int(sweepAngle * 180 / M_PI * -16));

    // Draw active portion arc
    double currentAngle = angleFromValue(m_value);
    if (m_value > m_min) {
        p.setPen(QPen(activeColor, trackWidth, Qt::SolidLine, Qt::RoundCap));
        int span = int((currentAngle - startAngle) * 180 / M_PI * -16);
        p.drawArc(arcRect.adjusted(trackWidth/2, trackWidth/2, -trackWidth/2, -trackWidth/2),
                  int(startAngle * 180 / M_PI * 16), span);
    }

    // Knob body (circle in center)
    int knobR = size / 2 - trackWidth - 4;
    QRectF knobCenter(w/2.0 - knobR, h/2.0 - knobR, knobR*2, knobR*2);
    p.setBrush(QColor(0x15, 0x1A, 0x1D));
    p.setPen(QPen(QColor(0x35, 0x44, 0x4B), 1));
    p.drawEllipse(knobCenter);

    // Indicator line from center to arc
    double indicatorAngle = angleFromValue(m_value);
    QPointF center(w/2.0, h/2.0);
    QPointF tip(center.x() + knobR * 0.7 * cos(indicatorAngle),
                center.y() + knobR * 0.7 * sin(indicatorAngle));
    p.setPen(QPen(UiTheme::accentGlow(), 2));
    p.drawLine(center, tip);

    // Label text below knob
    if (!m_label.isEmpty()) {
        p.setFont(m_fontLabel);
        p.setPen(UiTheme::textSecondary());
        p.drawText(rect().adjusted(0, size + 4, 0, 0),
                   Qt::AlignHCenter | Qt::AlignTop, m_label);
    }

    // Value text below label
    if (!m_units.isEmpty()) {
        QString display = QString("%1 %2")
                              .arg(m_value)
                              .arg(m_units);
        p.setFont(m_fontValue);
        p.setPen(UiTheme::textPrimary());
        p.drawText(rect().adjusted(0, size + 4 + 20, 0, 0),
                   Qt::AlignHCenter | Qt::AlignTop, display);
    }
}

// ──────────────────────────────────────────────
// Fader
// ──────────────────────────────────────────────
Fader::Fader(Orientation orient, QWidget* parent)
    : QWidget(parent), m_orient(orient) {
    setMouseTracking(true);
    setMinimumSize(orient == Vertical ? 48 : 120, orient == Vertical ? 160 : 48);
}

void Fader::setValue(int v) {
    v = qBound(m_min, v, m_max);
    if (v == m_value) return;
    m_value = v;
    emit valueChanged(m_value);
    update();
}

void Fader::setOrientation(Orientation o) {
    m_orient = o;
    updateGeometry();
    update();
}

void Fader::setLabel(const QString& label) { m_label = label; }
void Fader::setTickMarks(int count) { m_ticks = count; }

int Fader::positionToValue(double pos) const {
    double t = (m_max - m_min) > 0 ? pos / (m_max - m_min) : 0;
    return m_min + int(t * (m_max - m_min));
}

double Fader::valueToPosition(int v) const {
    if (m_max == m_min) return 0;
    return double(v - m_min) / (m_max - m_min);
}

void Fader::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() != Qt::LeftButton) return;
    m_dragging = true;
    int v = positionToValue(ev->pos().x() + ev->pos().y());
    setValue(v);
}

void Fader::mouseMoveEvent(QMouseEvent* ev) {
    if (!m_dragging) return;
    int px = m_orient == Vertical ? ev->pos().y() : ev->pos().x();
    int v = positionToValue(px);
    setValue(v);
}

void Fader::mouseReleaseEvent(QMouseEvent*) {
    if (m_dragging) {
        m_dragging = false;
        emit released();
    }
}

void Fader::leaveEvent(QEvent*) {
    if (m_dragging) {
        m_dragging = false;
        emit released();
    }
}

void Fader::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();
    bool vert = m_orient == Vertical;
    int trackLen = vert ? h : w;
    int trackW = 6;

    // Track background
    QRectF trackRect(vert ? (w - trackW) / 2.0 : 4,
                     vert ? 4 : (h - trackW) / 2.0,
                     trackW, trackLen - 8);

    // Background fill
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x0A, 0x0D, 0x0F));
    p.drawRoundedRect(trackRect, trackW / 2, trackW / 2);

    // Active portion (from bottom for vertical, from left for horizontal)
    double activeRatio = valueToPosition(m_value);
    if (activeRatio > 0.001) {
        QRectF activeRect = trackRect;
        if (vert) {
            activeRect.setHeight(trackLen * activeRatio);
            activeRect.setY(h - activeRect.height() - 4);
        } else {
            activeRect.setWidth(trackLen * activeRatio);
        }
        p.setBrush(UiTheme::accentDefault());
        p.drawRoundedRect(activeRect, trackW / 2, trackW / 2);
    }

    // Thumb
    int thumbSize = 16;
    double thumbPos;
    if (vert) {
        thumbPos = h - activeRatio * (h - 8) - 4 - thumbSize/2;
    } else {
        thumbPos = activeRatio * (w - 8) + 4 + thumbSize/2;
    }

    QRectF thumbRect(vert ? (w - thumbSize) / 2.0 : thumbPos - thumbSize/2,
                     vert ? thumbPos - thumbSize/2 : (h - thumbSize) / 2.0,
                     thumbSize, thumbSize);
    p.setBrush(QColor(0x1C, 0x23, 0x27));
    p.setPen(QPen(UiTheme::accentDefault(), 1.5));
    p.drawRoundedRect(thumbRect, thumbSize/2, thumbSize/2);

    // Tick marks
    if (m_ticks > 0) {
        p.setPen(QColor(0x35, 0x44, 0x4B));
        for (int i = 0; i <= m_ticks; ++i) {
            double t = double(i) / m_ticks;
            int tickX, tickY;
            if (vert) {
                tickX = w - trackW/2;
                tickY = h - t * (h - 8) - 4;
            } else {
                tickX = 4 + t * (w - 8);
                tickY = h - trackW/2;
            }
            p.drawLine(tickX, tickY, tickX + (vert ? -6 : 0), tickY + (vert ? 0 : 6));
        }
    }

    // Label
    if (!m_label.isEmpty()) {
        p.setFont(UiTheme::fontPrimary());
        p.setPen(UiTheme::textSecondary());
        QRect labelRect = rect();
        if (vert) {
            p.translate(0, h);
            p.rotate(-90);
            p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignBottom, m_label);
        } else {
            p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, m_label);
        }
    }
}

// ──────────────────────────────────────────────
// LedToggle
// ──────────────────────────────────────────────
LedToggle::LedToggle(QWidget* parent)
    : QAbstractButton(parent) {
    setCheckable(true);
    setChecked(false);
    setMinimumSize(36, 24);
}

void LedToggle::setLedColor(const QColor& color) { m_ledOnColor = color; }

void LedToggle::setState(bool on, bool updateText) {
    if (on == isChecked()) return;
    blockSignals(true);
    setChecked(on);
    blockSignals(false);
    if (updateText) update();
}

void LedToggle::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();
    bool on = isChecked();

    // Background
    QColor bg = on ? QColor(0x0F, 0x76, 0x6E) : QColor(0x1C, 0x23, 0x27);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(0, 0, w, h, h/2, h/2);

    // LED dot
    int ledR = qMin(w, h) / 2 - 4;
    QColor ledColor = on ? m_ledOnColor : m_ledOffColor;
    p.setBrush(ledColor);
    if (on) {
        // Glow effect
        p.setPen(Qt::NoPen);
        QPainterPath glow;
        glow.addEllipse(w/2, h/2, ledR + 4, ledR + 4);
        p.fillPath(glow, QColor(0x34, 0xD3, 0x99, 40));

        QPainterPath core;
        core.addEllipse(w/2, h/2, ledR, ledR);
        p.fillPath(core, m_ledOnColor);
    } else {
        p.drawEllipse(w/2, h/2, ledR * 2, ledR * 2);
    }

    // Label text below dot
    if (!text().isEmpty()) {
        p.setFont(UiTheme::fontCaption());
        p.setPen(on ? UiTheme::accentGlow() : UiTheme::textMuted());
        p.drawText(rect(), Qt::AlignHCenter | Qt::AlignTop, text());
    }
}

// ──────────────────────────────────────────────
// StatusChip
// ──────────────────────────────────────────────
StatusChip::StatusChip(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(20);
}

void StatusChip::setText(const QString& text) { m_text = text; update(); }
void StatusChip::setState(State state) { m_state = state; updateColors(); update(); }

void StatusChip::updateColors() {
    QColor bg, fg;
    switch (m_state) {
        case Ok:   bg = QColor(0x34, 0xD3, 0x99, 20);  fg = QColor(0x34, 0xD3, 0x99); break;
        case Warn: bg = QColor(0xFB, 0xBF, 0x24, 20);   fg = QColor(0xFB, 0xBF, 0x24); break;
        case Error:bg = QColor(0xF8, 0x71, 0x71, 20);    fg = QColor(0xF8, 0x71, 0x71); break;
        case Idle: bg = QColor(0x64, 0x74, 0x8B, 15);   fg = QColor(0x64, 0x74, 0x8B); break;
        case Info: bg = QColor(0x60, 0xA5, 0xFA, 20);    fg = QColor(0x60, 0xA5, 0xFA); break;
        default:   bg = QColor(0x1C, 0x23, 0x27, 20);   fg = QColor(0x6B, 0x7B, 0x82); break;
    }
    setStyleSheet(QString("StatusChip { background-color: %1; color: %2; border-radius: 10px; padding: 2px 8px; font-size: 11px; font-weight: bold; }")
                      .arg(bg.name())
                      .arg(fg.name()));
}

void StatusChip::paintEvent(QPaintEvent*) {
    // Handled by QSS, but we draw the text manually for control
    QPainter p(this);
    p.setPen(p.pen().color());  // already set by QSS color
    p.setFont(UiTheme::fontCaption());
    p.drawText(rect(), Qt::AlignVCenter | Qt::AlignHCenter, m_text);
}

// ──────────────────────────────────────────────
// LcdDisplay
// ──────────────────────────────────────────────
LcdDisplay::LcdDisplay(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(120, 32);
    setStyleSheet("LcdDisplay { "
        "background-color: #0A0D0F; "
        "border: 1px solid #263136; "
        "border-radius: 4px; "
        "color: #14B8A6; "
        "font-family: 'Fira Code', 'DejaVu Sans Mono', monospace; "
        "font-size: 14px; "
        "padding: 4px 8px; }");
}

void LcdDisplay::setText(const QString& text) {
    m_text = text;
    emit textChanged(m_text);
    update();
}

void LcdDisplay::setAlignment(Qt::Alignment align) { m_align = align; update(); }

void LcdDisplay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setPen(QColor(0x14, 0xB8, 0xA6));
    p.setFont(UiTheme::fontMono());
    p.drawText(rect(), int(m_align), m_text);
}

// ──────────────────────────────────────────────
// SegmentedSelector
// ──────────────────────────────────────────────
SegmentedSelector::SegmentedSelector(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(30);
}

void SegmentedSelector::addItem(const QString& label) { m_items << label; update(); }

void SegmentedSelector::setCurrentIndex(int index) {
    if (index < 0 || index >= m_items.size()) return;
    m_current = index;
    emit currentIndexChanged(m_current);
    update();
}

QString SegmentedSelector::currentLabel() const {
    return m_current >= 0 && m_current < m_items.size() ? m_items[m_current] : QString();
}

void SegmentedSelector::clear() {
    m_items.clear();
    m_current = 0;
    update();
}

int SegmentedSelector::hitIndex(const QPoint& pos) const {
    if (m_items.isEmpty()) return -1;
    int w = width();
    int segW = w / m_items.size();
    int idx = pos.x() / segW;
    return qBound(0, idx, m_items.size() - 1);
}

void SegmentedSelector::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() != Qt::LeftButton) return;
    int idx = hitIndex(ev->pos());
    if (idx >= 0 && idx != m_current) {
        m_current = idx;
        emit currentIndexChanged(m_current);
        emit itemClicked(m_current);
        update();
    }
}

void SegmentedSelector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();
    if (m_items.isEmpty()) return;

    int segW = w / m_items.size();

    for (int i = 0; i < m_items.size(); ++i) {
        QRect segRect(i * segW, 0, segW, h);

        // Background
        if (i == m_current) {
            p.setBrush(QColor(0x14, 0xB8, 0xA6));
            p.setPen(Qt::NoPen);
        } else {
            p.setBrush(QColor(0x1C, 0x23, 0x27));
            p.setPen(QPen(QColor(0x26, 0x31, 0x36), 1));
        }
        // Draw rounded rect for first/last, plain rects for middle
        if (i == 0) {
            p.drawRoundedRect(segRect.adjusted(0, 0, -1, -1), h/2, h/2);
        } else if (i == m_items.size() - 1) {
            p.drawRoundedRect(segRect.adjusted(1, 0, 0, -1), h/2, h/2);
        } else {
            p.drawRect(segRect);
        }

        // Text
        if (i == m_current) {
            p.setPen(QColor(0x0E, 0x11, 0x13));
        } else {
            p.setPen(UiTheme::textSecondary());
        }
        p.setFont(UiTheme::fontCaption());
        p.drawText(segRect, Qt::AlignVCenter | Qt::AlignHCenter, m_items[i]);
    }
}
