// =====================================================================
//  Theme.h
//  Finora's visual theme: style, colours and font for every widget.
// =====================================================================
#ifndef FINORA_THEME_H
#define FINORA_THEME_H

#include <QApplication>
#include <QString>

// Gives the whole program Finora's light look:
//   1. Qt's "Fusion" style, so Windows 10 and Windows 11 look the same,
//   2. light colours even when Windows itself is in dark mode,
//   3. the Segoe UI font,
//   4. colours, spacing and corners from resources/styles/light.qss.
// Returns true if the style sheet loaded, false if it was missing.
bool applyLightTheme(QApplication &app);

// Reads a Qt style sheet (.qss) file and returns its text.
// Returns an empty string if the file cannot be opened.
QString loadStyleSheet(const QString &filePath);

#endif // FINORA_THEME_H
