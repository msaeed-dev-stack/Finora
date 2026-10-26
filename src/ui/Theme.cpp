// =====================================================================
//  Theme.cpp
// =====================================================================
#include "Theme.h"

#include <QDebug>
#include <QFile>
#include <QFont>
#include <QStyleHints>

bool applyLightTheme(QApplication &app)
{
    app.setStyle("Fusion");
    app.styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setFont(QFont("Segoe UI", 10));

    QString styleText = loadStyleSheet(":/styles/light.qss");
    if (styleText.isEmpty()) {
        return false;   // keep Qt's default look instead of crashing
    }

    app.setStyleSheet(styleText);
    return true;
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
