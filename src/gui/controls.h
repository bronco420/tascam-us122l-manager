#ifndef CONTROLS_H
#define CONTROLS_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QAbstractButton>
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QStyleOption>

// ──────────────────────────────────────────────
// ArcKnob — rotary knob with drag-to-adjust
// ──────────────────────────────────────────────
class ArcKnob : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minValue READ minValue WRITE setMinValue)
    Q_PROPERTY(int maxValue READ maxValue WRITE setMaxValue)

public:
    explicit ArcKnob(QWidget* parent = nullptr);

    int value() const { return m_value; }
    void setValue(int v);
    int minValue() const { return m_min; }
    void setMinValue(int v) { m_min = v; }
    int maxValue() const { return m_max; }
    void setMaxValue(int v) { m_max = v; }

    void setLabel(const QString& label);
    void setUnits(const QString& units);

signals:
    void valueChanged(int value);
    void released();  // emitted when user stops dragging

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    int valueFromAngle(double angle) const;
    double angleFromValue(int v) const;
    int positionToValue(int y) const;

    int m_value = 50;
    int m_min = 0;
    int m_max = 100;
    QString m_label;
    QString m_units;
    bool m_dragging = false;
    int m_lastY = 0;
    QFont m_fontLabel;
    QFont m_fontValue;
};

// ──────────────────────────────────────────────
// Fader — vertical/horizontal slider with track
// ──────────────────────────────────────────────
class Fader : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    enum Orientation { Vertical, Horizontal };

    explicit Fader(Orientation orient = Vertical, QWidget* parent = nullptr);

    int value() const { return m_value; }
    void setValue(int v);
    int minValue() const { return m_min; }
    void setMinValue(int v) { m_min = v; }
    int maxValue() const { return m_max; }
    void setMaxValue(int v) { m_max = v; }
    void setOrientation(Orientation o);

    void setLabel(const QString& label);
    void setTickMarks(int count);  // number of tick marks (0 = none)

signals:
    void valueChanged(int value);
    void released();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    int positionToValue(double pos) const;
    double valueToPosition(int v) const;

    int m_value = 50;
    int m_min = 0;
    int m_max = 100;
    Orientation m_orient = Vertical;
    QString m_label;
    int m_ticks = 0;
    bool m_dragging = false;
};

// ──────────────────────────────────────────────
// LedToggle — checkable button that shows a colored LED dot
// ──────────────────────────────────────────────
class LedToggle : public QAbstractButton {
    Q_OBJECT

public:
    explicit LedToggle(QWidget* parent = nullptr);

    void setLedColor(const QColor& color);
    void setState(bool on, bool updateText = true);

signals:
    void toggled(bool on);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QColor m_ledOnColor = QColor(0x34, 0xD3, 0x99);  // green
    QColor m_ledOffColor = QColor(0x64, 0x74, 0x8B);  // gray
};

// ──────────────────────────────────────────────
// StatusChip — colored badge showing status text
// ──────────────────────────────────────────────
class StatusChip : public QWidget {
    Q_OBJECT

public:
    enum State { Ok, Warn, Error, Idle, Info };

    explicit StatusChip(QWidget* parent = nullptr);

    void setText(const QString& text);
    void setState(State state);
    State state() const { return m_state; }

private:
    void updateColors();
    void paintEvent(QPaintEvent*) override;

    QString m_text;
    State m_state = Idle;
};

// ──────────────────────────────────────────────
// LcdDisplay — monospaced text display (like an LCD panel)
// ──────────────────────────────────────────────
class LcdDisplay : public QWidget {
    Q_OBJECT

public:
    explicit LcdDisplay(QWidget* parent = nullptr);

    void setText(const QString& text);
    void setAlignment(Qt::Alignment align);

signals:
    void textChanged(const QString& text);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QString m_text;
    Qt::Alignment m_align = Qt::AlignCenter;
};

// ──────────────────────────────────────────────
// SegmentedSelector — pill-shaped segmented control (e.g. preset selector)
// ──────────────────────────────────────────────
class SegmentedSelector : public QWidget {
    Q_OBJECT

public:
    explicit SegmentedSelector(QWidget* parent = nullptr);

    void addItem(const QString& label);
    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index);
    QString currentLabel() const;
    void clear();  // remove all items
    const QVector<QString>& items() const { return m_items; }

signals:
    void currentIndexChanged(int index);
    void itemClicked(int index);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

private:
    int hitIndex(const QPoint& pos) const;
    QVector<QString> m_items;
    int m_current = 0;
};

#endif // CONTROLS_H
