#ifndef THEME_TOKENS_H
#define THEME_TOKENS_H

#include <QColor>
#include <QString>
#include <QFont>
#include <QPainter>
#include <QWidget>

// Central theme token store for the TASCAM US-122L Manager GUI.
// All colors, fonts, border radii, and spacing are defined here so that
// every widget shares a single source of truth.
class UiTheme {
public:
    // ---- Palette (hex → QColor) ----
    static QColor bgBase()          { return QColor(0x0E, 0x11, 0x13); }
    static QColor bgPanel()         { return QColor(0x15, 0x1A, 0x1D); }
    static QColor bgPanelRaised()   { return QColor(0x1C, 0x23, 0x27); }
    static QColor bgSunken()        { return QColor(0x0A, 0x0D, 0x0F); }
    static QColor bgHover()         { return QColor(0x23, 0x2C, 0x31); }
    static QColor bgActive()        { return QColor(0x26, 0x35, 0x3A); }

    static QColor borderSubtle()    { return QColor(0x26, 0x31, 0x36); }
    static QColor borderStrong()    { return QColor(0x35, 0x44, 0x4B); }
    static QColor borderAccent()    { return QColor(0x14, 0xB8, 0xA6); }

    static QColor accentDefault()   { return QColor(0x14, 0xB8, 0xA6); }
    static QColor accentGlow()      { return QColor(0x2D, 0xD4, 0xBF); }
    static QColor accentDim()       { return QColor(0x0F, 0x76, 0x6E); }

    static QColor textPrimary()     { return QColor(0xE8, 0xEE, 0xF1); }
    static QColor textSecondary()   { return QColor(0xA7, 0xB4, 0xBA); }
    static QColor textMuted()       { return QColor(0x6B, 0x7B, 0x82); }

    static QColor stateOk()         { return QColor(0x34, 0xD3, 0x99); }
    static QColor stateWarn()       { return QColor(0xFB, 0xBF, 0x24); }
    static QColor stateError()      { return QColor(0xF8, 0x71, 0x71); }
    static QColor stateIdle()       { return QColor(0x64, 0x74, 0x8B); }
    static QColor stateInfo()       { return QColor(0x60, 0xA5, 0xFA); }

    // ---- Typography ----
    static QFont fontPrimary();
    static QFont fontMono();
    static QFont fontLabel();
    static QFont fontCaption();

    // ---- Spacing & radii ----
    static int spacingSmall()       { return 4; }
    static int spacingMedium()      { return 8; }
    static int spacingLarge()       { return 16; }
    static int radiusSmall()        { return 4; }
    static int radiusMedium()       { return 6; }
    static int radiusLarge()        { return 8; }

    // ---- QPainter helpers ----
    static void drawRaisedPanel(QPainter* p, const QRect& rect);
    static void drawSubtleBorder(QPainter* p, const QRect& rect);
    static QColor accentWithAlpha(int alpha);
    static QColor stateWithAlpha(int state, int alpha);

    // ---- QSS generator (applied to QApplication) ----
    static QString globalQss();
};

#endif // THEME_TOKENS_H
