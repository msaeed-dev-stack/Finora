// =====================================================================
//  Theme.h
//  Finora's design system: the colours of the light and the dark theme,
//  the bundled fonts, the style sheet, coloured icons and the
//  "reduce motion" setting.
// =====================================================================
#ifndef FINORA_THEME_H
#define FINORA_THEME_H

#include <QApplication>
#include <QColor>
#include <QPixmap>
#include <QString>
#include <QWidget>

// The two looks Finora offers.
enum ThemeMode {
    THEME_LIGHT,
    THEME_DARK
};

// Every colour Finora uses, named after its job ("design tokens").
// Both themes have the same names; only the values differ. Widgets ask
// for themeColors().income instead of writing "#0F766E" themselves, so
// one switch changes the whole program.
struct ThemeColors {
    QColor canvas;           // the window background behind the cards
    QColor surface;          // cards, the sidebar, dialogs, text boxes
    QColor surfaceMuted;     // hover fills, table stripes, empty progress tracks
    QColor border;           // card edges and dividers
    QColor borderStrong;     // text box and button edges
    QColor text;             // main text
    QColor textMuted;        // secondary text, labels
    QColor primary;          // Finora violet: buttons and selected states (white text on it)
    QColor primaryHover;
    QColor primaryPressed;
    QColor primaryText;      // violet text and icons on a surface
    QColor primarySoft;      // pale violet backgrounds
    QColor income;           // teal: money coming in
    QColor incomeSoft;
    QColor expense;          // rose: money going out
    QColor expenseText;      // rose text on expenseSoft
    QColor expenseSoft;
    QColor warning;          // amber: a budget at 80% or more
    QColor warningText;      // amber text on warningSoft
    QColor warningSoft;
    QColor focus;            // the keyboard focus ring
    QColor chartGrid;        // the light lines behind chart bars
    QColor tooltip;          // the chart's value bubble
    QColor tooltipText;
};

// The colours of each theme, and of the theme in use right now.
ThemeColors lightThemeColors();
ThemeColors darkThemeColors();
const ThemeColors &themeColors();
ThemeMode currentThemeMode();

// Registers the bundled Inter and Manrope fonts with Qt. Returns true if
// all font files loaded; if one is missing, Qt uses a similar font.
bool loadBundledFonts();

// Gives the whole program one theme:
//   1. Qt's "Fusion" style, so Windows 10 and 11 look the same,
//   2. a colour palette for the parts that Fusion draws itself,
//   3. the Inter font,
//   4. the style sheet resources/styles/finora.qss with the theme's colours.
// It can be called again at any time to switch themes. Returns true if
// the style sheet loaded, false if it was missing.
bool applyTheme(QApplication &app, ThemeMode mode);

// The Milestone 1 name, kept so older code and tests still work.
bool applyLightTheme(QApplication &app);

// Reads a Qt style sheet (.qss) file and returns its text.
// Returns an empty string if the file cannot be opened.
QString loadStyleSheet(const QString &filePath);

// Replaces every {{name}} in the style sheet template with the colour of
// that name, for example {{surface}} with #FFFFFF.
QString fillStyleTemplate(const QString &templateText, const ThemeColors &colors, ThemeMode mode);

// "Reduce motion": when it is on, Finora shows changes at once instead of
// fading or growing them. Off unless the user turns it on in Settings.
void setReducedMotion(bool reduced);
bool isReducedMotion();

// Loads :/icons/<name>.svg, paints it in `color` and returns a sharp
// picture of size x size pixels. The icon files draw with "currentColor",
// which is replaced here, so one file serves every colour and theme.
QPixmap tintedIcon(const QString &iconName, const QColor &color, int size);

// Gives a widget a "tone" (success, warning, danger, info or neutral).
// The style sheet colours chips and bars by this property, for example
// QLabel#statusChip[tone="warning"].
void setTone(QWidget *widget, const QString &tone);

#endif // FINORA_THEME_H
