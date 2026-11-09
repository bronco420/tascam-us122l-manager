#ifndef DRAWER_H
#define DRAWER_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <QPropertyAnimation>
#include <QEasingCurve>

class Drawer : public QFrame {
    Q_OBJECT

public:
    explicit Drawer(QWidget* parent = nullptr);

    // Add a section with a title and a list of items
    void addSection(const QString& title, const QStringList& items);

    void show();
    void hide();
    bool isShown() const { return m_shown; }

signals:
    void itemSelected(const QString& action);

private slots:
    void onItemClicked(int index);
    void onCloseClicked();
    void onAnimationFinished();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    struct Section {
        QString title;
        QStringList items;
    };

    QVector<Section> m_sections;
    bool m_shown = false;
    QPropertyAnimation* m_animation;
    QWidget* m_scroller;  // scrollable area for items
    QVBoxLayout* m_contentLayout;

    static constexpr int DRAWER_WIDTH = 320;
    static constexpr int ANIM_DURATION = 250;
};

#endif // DRAWER_H
