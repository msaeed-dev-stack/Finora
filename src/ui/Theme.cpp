// =====================================================================
//  Theme.cpp
// =====================================================================
#include "Theme.h"

#include <QDebug>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>
#include <QSvgRenderer>

#include <vector>

// ---- The current theme ------------------------------------------------
// These variables exist once for the whole program ("static" keeps them
// private to this file). applyTheme() changes them; every widget reads
// them through themeColors() and isReducedMotion().
static ThemeMode activeMode = THEME_LIGHT;
static ThemeColors activeColors = lightThemeColors();
static bool reducedMotion = false;

ThemeColors lightThemeColors()
{
    ThemeColors c;
    c.canvas = QColor("#F5F3FC");
    c.surface = QColor("#FFFFFF");
    c.surfaceMuted = QColor("#F6F4FD");
    c.border = QColor("#E7E3F5");
    c.borderStrong = QColor("#D5CFEA");
    c.text = QColor("#17132E");
    c.textMuted = QColor("#625C7E");
    c.primary = QColor("#6D4AFF");
    c.primaryHover = QColor("#5D3BEF");
    c.primaryPressed = QColor("#4E2FD6");
    c.primaryText = QColor("#5634E8");
    c.primarySoft = QColor("#EFEAFF");
    c.income = QColor("#0F766E");
    c.incomeSoft = QColor("#E3F5F2");
    c.expense = QColor("#D61F45");
    c.expenseText = QColor("#C01B3E");
    c.expenseSoft = QColor("#FDE8EC");
    c.warning = QColor("#B86A00");
    c.warningText = QColor("#94510A");
    c.warningSoft = QColor("#FFF3DC");
    c.focus = QColor("#6D4AFF");
    c.chartGrid = QColor("#ECE9F6");
    c.tooltip = QColor("#17132E");
    c.tooltipText = QColor("#FFFFFF");
    return c;
}

ThemeColors darkThemeColors()
{
    ThemeColors c;
    c.canvas = QColor("#0F0C1D");
    c.surface = QColor("#181430");
    c.surfaceMuted = QColor("#211C3E");
    c.border = QColor("#2D2750");
    c.borderStrong = QColor("#3E3768");
    c.text = QColor("#F2EFFC");
    c.textMuted = QColor("#ABA5CB");
    c.primary = QColor("#6D4AFF");
    c.primaryHover = QColor("#5D3BEF");
    c.primaryPressed = QColor("#4E2FD6");
    c.primaryText = QColor("#A994FF");
    c.primarySoft = QColor("#2B2357");
    c.income = QColor("#2DD4BF");
    c.incomeSoft = QColor("#123B39");
    c.expense = QColor("#FB7185");
    c.expenseText = QColor("#FB7185");
    c.expenseSoft = QColor("#43172A");
    c.warning = QColor("#FBBF24");
    c.warningText = QColor("#FBBF24");
    c.warningSoft = QColor("#3D2E0A");
    c.focus = QColor("#A994FF");
    c.chartGrid = QColor("#2A2448");
    c.tooltip = QColor("#F2EFFC");
    c.tooltipText = QColor("#17132E");
    return c;
}

const ThemeColors &themeColors()
{
    return activeColors;
}

ThemeMode currentThemeMode()
{
    return activeMode;
}

bool loadBundledFonts()
{
    // Inter for text and numbers, Manrope for headings. Both use the SIL
    // Open Font License 1.1 (see resources/fonts/*-OFL.txt).
    const int FONT_COUNT = 7;
    const char *FONT_FILES[FONT_COUNT] = {
        ":/fonts/Inter-Regular.ttf", ":/fonts/Inter-Medium.ttf",
        ":/fonts/Inter-SemiBold.ttf", ":/fonts/Inter-Bold.ttf",
        ":/fonts/Manrope-SemiBold.ttf", ":/fonts/Manrope-Bold.ttf",
        ":/fonts/Manrope-ExtraBold.ttf"
    };

    bool allLoaded = true;
    for (int i = 0; i < FONT_COUNT; i++) {
        // addApplicationFont returns -1 if the file could not be used.
        if (QFontDatabase::addApplicationFont(FONT_FILES[i]) < 0) {
            qWarning() << "Finora: could not load font" << FONT_FILES[i];
            allLoaded = false;
        }
    }
    return allLoaded;
}

// The palette is used by the parts that Qt's Fusion style draws itself,
// such as check boxes, the calendar pop-up and message boxes.
QPalette buildPalette(const ThemeColors &c)
{
    QPalette palette;
    palette.setColor(QPalette::Window, c.surface);
    palette.setColor(QPalette::WindowText, c.text);
    palette.setColor(QPalette::Base, c.surface);
    palette.setColor(QPalette::AlternateBase, c.surfaceMuted);
    palette.setColor(QPalette::Text, c.text);
    palette.setColor(QPalette::PlaceholderText, c.textMuted);
    palette.setColor(QPalette::Button, c.surface);
    palette.setColor(QPalette::ButtonText, c.text);
    palette.setColor(QPalette::BrightText, c.expense);
    palette.setColor(QPalette::Highlight, c.primary);
    palette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    palette.setColor(QPalette::ToolTipBase, c.tooltip);
    palette.setColor(QPalette::ToolTipText, c.tooltipText);
    palette.setColor(QPalette::Link, c.primaryText);
    palette.setColor(QPalette::Light, c.surface);
    palette.setColor(QPalette::Midlight, c.surfaceMuted);
    palette.setColor(QPalette::Mid, c.border);
    palette.setColor(QPalette::Dark, c.borderStrong);
    palette.setColor(QPalette::Shadow, c.borderStrong);

    // Disabled widgets use the muted text colour.
    palette.setColor(QPalette::Disabled, QPalette::Text, c.textMuted);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, c.textMuted);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, c.textMuted);
    return palette;
}

bool applyTheme(QApplication &app, ThemeMode mode)
{
    activeMode = mode;
    if (mode == THEME_DARK) {
        activeColors = darkThemeColors();
    } else {
        activeColors = lightThemeColors();
    }

    // Set the Fusion style only once; switching themes later keeps it.
    if (app.style()->name() != "fusion") {
        app.setStyle("Fusion");
    }
    // The colour scheme also tells Windows 11 to draw a dark or light title bar.
    if (mode == THEME_DARK) {
        app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    } else {
        app.styleHints()->setColorScheme(Qt::ColorScheme::Light);
    }
    app.setPalette(buildPalette(activeColors));

    QFont font("Inter", 10);
    font.setStyleHint(QFont::SansSerif);   // if Inter is missing, use another sans-serif font
    app.setFont(font);

    QString templateText = loadStyleSheet(":/styles/finora.qss");
    if (templateText.isEmpty()) {
        return false;   // keep Qt's default look instead of crashing
    }

    app.setStyleSheet(fillStyleTemplate(templateText, activeColors, mode));
    return true;
}

bool applyLightTheme(QApplication &app)
{
    return applyTheme(app, THEME_LIGHT);
}

QString loadStyleSheet(const QString &filePath)
{
    QFile file(filePath);

    // open() returns false when the file is missing or cannot be read.
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Finora: could not open style sheet" << filePath;
        return QString();
    }

    QString styleText = QString::fromUtf8(file.readAll());
    file.close();
    return styleText;
}

// One name in the style sheet template and the text that replaces it.
struct StyleToken {
    QString name;    // for example "surface", written {{surface}} in finora.qss
    QString value;   // for example "#FFFFFF"
};

QString fillStyleTemplate(const QString &templateText, const ThemeColors &colors, ThemeMode mode)
{
    QString chevronIcon = ":/icons/chevron_down_light.svg";
    if (mode == THEME_DARK) {
        chevronIcon = ":/icons/chevron_down_dark.svg";
    }

    std::vector<StyleToken> tokens = {
        {"canvas", colors.canvas.name()},
        {"surface", colors.surface.name()},
        {"surfaceMuted", colors.surfaceMuted.name()},
        {"border", colors.border.name()},
        {"borderStrong", colors.borderStrong.name()},
        {"text", colors.text.name()},
        {"textMuted", colors.textMuted.name()},
        {"primary", colors.primary.name()},
        {"primaryHover", colors.primaryHover.name()},
        {"primaryPressed", colors.primaryPressed.name()},
        {"primaryText", colors.primaryText.name()},
        {"primarySoft", colors.primarySoft.name()},
        {"income", colors.income.name()},
        {"incomeSoft", colors.incomeSoft.name()},
        {"expense", colors.expense.name()},
        {"expenseText", colors.expenseText.name()},
        {"expenseSoft", colors.expenseSoft.name()},
        {"warning", colors.warning.name()},
        {"warningText", colors.warningText.name()},
        {"warningSoft", colors.warningSoft.name()},
        {"focus", colors.focus.name()},
        {"tooltip", colors.tooltip.name()},
        {"tooltipText", colors.tooltipText.name()},
        {"chevronIcon", chevronIcon}
    };

    // The braces make every name unique: replacing {{primary}} can never
    // change part of {{primaryHover}}.
    QString result = templateText;
    int count = static_cast<int>(tokens.size());
    for (int i = 0; i < count; i++) {
        result.replace("{{" + tokens[i].name + "}}", tokens[i].value);
    }
    if (result.contains("{{")) {
        qWarning() << "Finora: the style sheet uses a colour name that does not exist";
    }
    return result;
}

void setReducedMotion(bool reduced)
{
    reducedMotion = reduced;
}

bool isReducedMotion()
{
    return reducedMotion;
}

QPixmap tintedIcon(const QString &iconName, const QColor &color, int size)
{
    QFile file(":/icons/" + iconName + ".svg");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Finora: missing icon" << iconName;
        return QPixmap();
    }
    QByteArray svgText = file.readAll();
    file.close();
    svgText.replace("currentColor", color.name().toUtf8());

    // Draw at the screen's pixel density, so the icon stays sharp on
    // high-resolution (for example 150% or 200%) displays.
    qreal pixelRatio = qApp->devicePixelRatio();
    int pixels = qRound(size * pixelRatio);
    QPixmap pixmap(pixels, pixels);
    pixmap.fill(Qt::transparent);

    QSvgRenderer renderer(svgText);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();

    pixmap.setDevicePixelRatio(pixelRatio);
    return pixmap;
}

void setTone(QWidget *widget, const QString &tone)
{
    widget->setProperty("tone", tone);
    // Qt reads [tone="..."] rules only when a widget is styled, so the
    // widget is styled again after the property changed.
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}
