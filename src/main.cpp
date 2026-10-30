// =====================================================================
//  main.cpp
//  Finora - Desktop Personal Finance & Budget Management System
//
//  The program starts here. main() creates the Qt application, gives it
//  Finora's look, opens (and if needed upgrades) the database, shows the
//  main window and then waits for the user.
// =====================================================================
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

#include "auth/AuthService.h"
#include "auth/PasswordHasher.h"
#include "core/Session.h"
#include "database/DatabaseManager.h"
#include "database/InstanceLock.h"
#include "ui/AppController.h"
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

// Asks before a database from an older Finora is upgraded. The user
// decides; "Quit" closes Finora and leaves the file exactly as it is.
bool askToUpgrade(const QString &databasePath, int storedVersion)
{
    QString backupFolder = QDir::toNativeSeparators(QFileInfo(databasePath).absolutePath() + "/backups");

    QMessageBox box;
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle("Upgrade Finora's data file");
    box.setText("This version of Finora stores your records in a new layout.");
    box.setInformativeText(
        "Your data file uses layout " + QString::number(storedVersion) + ". Finora will upgrade it to layout "
        + QString::number(CURRENT_SCHEMA_VERSION) + ", which adds budgets and prepares user accounts. "
        "Every record is kept.\n\n"
        "Before anything changes, a complete backup copy is saved in:\n" + backupFolder + "\n\n"
        "If any step fails, the upgrade is undone and your file stays exactly as it is.");
    QPushButton *upgradeButton = box.addButton("Upgrade and continue", QMessageBox::AcceptRole);
    box.addButton("Quit without changes", QMessageBox::RejectRole);
    box.setDefaultButton(upgradeButton);
    box.exec();

    return box.clickedButton() == upgradeButton;
}

int main(int argc, char *argv[])
{
    // QApplication runs the whole graphical program: it receives mouse
    // clicks and key presses from Windows and delivers them to widgets.
    QApplication app(argc, argv);
    app.setApplicationName("Finora");
    app.setApplicationVersion(FINORA_VERSION);   // the number comes from CMakeLists.txt

    // Fonts, style and colours. If the style sheet were missing, Finora
    // would still run, just with Qt's plain default look. The saved theme
    // (light or dark) is applied once the database is open.
    loadBundledFonts();
    applyTheme(app, THEME_LIGHT);

    // libsodium protects the passwords. Without it Finora must not run.
    if (!PasswordHasher::initializeLibrary()) {
        QMessageBox::critical(nullptr, "Finora cannot start",
                              "Finora could not start its security library (libsodium-26.dll), which "
                              "protects passwords.\n\nFinora will now close.");
        return 1;
    }

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

    // A file from an older Finora is upgraded only if the user agrees.
    int storedVersion = DatabaseManager::storedSchemaVersion(databasePath);
    if (storedVersion >= 1 && storedVersion < CURRENT_SCHEMA_VERSION) {
        if (!askToUpgrade(databasePath, storedVersion)) {
            return 0;   // nothing was changed
        }
    }

    // Who is signed in. It is declared before `database`, so C++ destroys
    // it after the database, which keeps a pointer to it (RAII order).
    Session session;

    // Open the database before any window is shown. Its destructor closes
    // the connection when the program ends (RAII).
    DatabaseManager database;
    if (!database.open(databasePath)) {
        showDatabaseError(databasePath, database.lastError());
        return 1;
    }
    // From now on every record belongs to the signed-in person, and none
    // can be reached while nobody is signed in.
    database.attachSession(session);

    // Passwords: Argon2id MODERATE (256 MiB, 3 passes). Recovery codes:
    // INTERACTIVE (64 MiB, 2 passes). See docs/MILESTONE4_PROGRESS.md.
    PasswordHasher passwordHasher(HASH_MODERATE);
    PasswordHasher recoveryCodeHasher(HASH_INTERACTIVE);
    AuthService auth(database, session, passwordHasher, recoveryCodeHasher);

    // The controller shows the sign-in window first, then the main window.
    AppController controller(database, auth);
    controller.start();

    // Start the event loop: wait for user actions until the last window is
    // closed, then end the session and return the exit code to Windows.
    int exitCode = app.exec();
    auth.signOut("app_closed");
    return exitCode;
}
