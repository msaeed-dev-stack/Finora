// =====================================================================
//  main.cpp
//  Finora - Desktop Personal Finance & Budget Management System
//
//  The program starts here. main() creates the Qt application, gives it
//  Finora's look, opens the database, shows the main window and then
//  waits for the user.
// =====================================================================
#include <QApplication>
#include <QDir>
#include <QMessageBox>

#include "database/DatabaseManager.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

// Explains why Finora cannot start. Finora then closes instead of running
// without saving, so the user never loses work without knowing it.
void showDatabaseError(const QString &databasePath, const QString &reason)
{
    QString fileText = QDir::toNativeSeparators(databasePath);
    if (fileText.isEmpty()) {
        fileText = "(Windows reported no data folder)";
    }

    QMessageBox::critical(nullptr, "Finora cannot open its data",
                          "Finora could not open or create its database.\n\n"
                          "File: " + fileText + "\n"
                          "Reason: " + reason + "\n\n"
                          "Finora will now close.");
}

int main(int argc, char *argv[])
{
    // QApplication runs the whole graphical program: it receives mouse
    // clicks and key presses from Windows and delivers them to widgets.
    QApplication app(argc, argv);
    app.setApplicationName("Finora");
    app.setApplicationVersion(FINORA_VERSION);   // the number comes from CMakeLists.txt

    // Style, font and colours. If the style sheet were missing, Finora
    // would still run, just with Qt's plain default look.
    applyLightTheme(app);

    // Open the database before any page is shown. `database` is declared
    // before `window`, so C++ destroys it after the window, and its
    // destructor closes the connection when the program ends (RAII).
    DatabaseManager database;
    QString databasePath = DatabaseManager::defaultDatabasePath();
    if (!database.open(databasePath)) {
        showDatabaseError(databasePath, database.lastError());
        return 1;
    }

    MainWindow window;
    window.show();

    // Start the event loop: wait for user actions until the window is
    // closed, then return the program's exit code to Windows.
    return app.exec();
}
