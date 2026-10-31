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

#include "auth/AuthService.h"
#include "auth/PasswordHasher.h"
#include "billing/SimulatedPaymentGateway.h"
#include "billing/SubscriptionService.h"
#include "core/Session.h"
#include "database/DatabaseManager.h"
#include "database/InstanceLock.h"
#include "ui/AppController.h"
#include "ui/DatabaseStartup.h"
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
    // "Finora.exe --database <file>" uses another data file, for example a
    // demonstration copy, so the person's real data file is never touched.
    int optionAt = app.arguments().indexOf("--database");
    if (optionAt > 0 && optionAt + 1 < app.arguments().size()) {
        databasePath = QFileInfo(app.arguments().at(optionAt + 1)).absoluteFilePath();
    }

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

    // Who is signed in. It is declared before `database`, so C++ destroys
    // it after the database, which keeps a pointer to it (RAII order).
    Session session;

    // Open the database before any window is shown. Its destructor closes
    // the connection when the program ends (RAII). A file from an older
    // Finora is upgraded only if the user agrees, also after a crash in
    // the middle of an earlier upgrade (see ui/DatabaseStartup.h).
    DatabaseManager database;
    StartupResult startup = openDatabaseAtStartup(database, databasePath);
    if (startup == STARTUP_DECLINED) {
        return 0;   // the user chose "Quit without changes"
    }
    if (startup == STARTUP_FAILED) {
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

    // Subscriptions (Phase 4C). Every payment is SIMULATED: the gateway
    // moves no money, has no secret keys and never sees card details.
    // Started as "Finora.exe --billing-demo", the Subscription page also
    // offers a button that ends the current billing period now, to show
    // a renewal or an expiry live during a demonstration.
    bool billingDemo = app.arguments().contains("--billing-demo");
    SimulatedPaymentGateway paymentGateway;
    SubscriptionService subscriptions(database, paymentGateway, billingDemo);

    // The controller shows the sign-in window first, then the main window.
    AppController controller(database, auth);
    controller.setSubscriptionService(&subscriptions);
    controller.start();

    // Start the event loop: wait for user actions until the last window is
    // closed, then end the session and return the exit code to Windows.
    int exitCode = app.exec();
    auth.signOut("app_closed");
    return exitCode;
}
