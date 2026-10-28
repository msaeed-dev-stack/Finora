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
#include "database/InstanceLock.h"
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

// Tells the user that Finora is already open. This copy then closes, so
// two copies never write to the same database at the same time.
void showAlreadyRunning()
{
    QMessageBox::information(nullptr, "Finora is already open",
                             "Finora is already running on this computer.\n\n"
                             "Look for its window on the taskbar. Only one copy "
                             "of Finora can be open at a time, so that your "
                             "records are always saved safely.");
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

    QString databasePath = DatabaseManager::defaultDatabasePath();

    // Only one copy of Finora may use the database. The lock is taken
    // before the database is opened, and `instanceLock` is declared first,
    // so C++ destroys it last: the lock is released only after the
    // database has been closed (RAII).
    InstanceLock instanceLock(InstanceLock::lockPathFor(databasePath));
    if (!instanceLock.tryLock()) {
        if (instanceLock.heldByAnotherCopy()) {
            showAlreadyRunning();
        } else {
            showDatabaseError(databasePath, instanceLock.lastError());
        }
        return 1;
    }

    // Open the database before any page is shown. `database` is declared
    // before `window`, so C++ destroys it after the window, and its
    // destructor closes the connection when the program ends (RAII).
    DatabaseManager database;
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
