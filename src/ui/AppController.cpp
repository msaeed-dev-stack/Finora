// =====================================================================
//  AppController.cpp
// =====================================================================
#include "AppController.h"

#include <QApplication>
#include <QEvent>
#include <QMessageBox>
#include <QPushButton>

#include "Theme.h"
#include "UiHelpers.h"
#include "components/Toast.h"

AppController::AppController(DatabaseManager &sharedDatabase, AuthService &sharedAuth, int idleLimitSeconds,
                             QObject *parent)
    : QObject(parent), database(sharedDatabase), auth(sharedAuth), idleLimit(idleLimitSeconds)
{
    signInWindow = new AuthWindow(auth);
    connect(signInWindow, &AuthWindow::signedIn, this, &AppController::onSignedIn);

    // Once a second: is the person still there? (Checking costs almost nothing.)
    sessionTimer = new QTimer(this);
    sessionTimer->setInterval(1000);
    connect(sessionTimer, &QTimer::timeout, this, &AppController::checkSession);
    sessionTimer->start();
    lastActivity.start();
    lastKeepAlive.start();

    qApp->installEventFilter(this);
}

AppController::~AppController()
{
    qApp->removeEventFilter(this);
    delete appWindow;      // top-level windows have no parent, so they are deleted here
    delete signInWindow;
}

void AppController::start()
{
    // Nobody is signed in yet, so these are the installation's settings.
    setReducedMotion(database.setting("reduced_motion", "0") == "1");
    ThemeMode mode = THEME_LIGHT;
    if (database.setting("theme", "light") == "dark") {
        mode = THEME_DARK;
    }
    applyTheme(*qApp, mode);
    signInWindow->refreshTheme();
    signInWindow->showStart();
    signInWindow->show();
}

AuthWindow *AppController::authWindow() const
{
    return signInWindow;
}

MainWindow *AppController::mainWindow() const
{
    return appWindow;
}

bool AppController::eventFilter(QObject *watched, QEvent *event)
{
    QEvent::Type type = event->type();
    if (type == QEvent::KeyPress || type == QEvent::MouseButtonPress || type == QEvent::MouseMove
        || type == QEvent::Wheel) {
        lastActivity.restart();
    }
    return QObject::eventFilter(watched, event);   // never stops the event
}

void AppController::onSignedIn(bool newAccount)
{
    signInWindow->hide();

    appWindow = new MainWindow(database, &auth);
    connect(appWindow, &MainWindow::signOutRequested, this, &AppController::onSignOutRequested);
    appWindow->show();

    // Records made before accounts existed are moved only if the person
    // agrees. The question appears over the main window (its parent).
    QString movedText;
    if (newAccount) {
        movedText = offerRecordsWithoutOwner();
        if (!movedText.isEmpty()) {
            appWindow->showPage(PAGE_DASHBOARD);   // load the moved records
        }
    }

    QString name = QString::fromStdString(auth.session().firstName());
    QString welcome = "Welcome back, " + name + "!";
    if (newAccount) {
        welcome = "Welcome to Finora, " + name + "!";
    }
    Toast::showMessage(appWindow, welcome + movedText);
    lastActivity.restart();
    lastKeepAlive.restart();
}

void AppController::onSignOutRequested()
{
    auth.signOut("signed_out");
    closeMainWindow("You have signed out.");
}

void AppController::checkSession()
{
    if (appWindow == nullptr || !auth.session().isSignedIn()) {
        return;
    }

    long long idleSeconds = lastActivity.elapsed() / 1000;
    if (idleSeconds >= idleLimit) {
        // An open dialog is closed first (as if Cancel was clicked), so no
        // window is deleted while it is still waiting for the person.
        QWidget *modal = QApplication::activeModalWidget();
        if (modal != nullptr) {
            modal->close();
            return;   // sign out at the next check, one second later
        }
        auth.endSessionIfIdle(idleSeconds, idleLimit);
        QString period = QString::number(idleLimit / 60) + " minutes";
        if (idleLimit < 60) {
            period = QString::number(idleLimit) + " seconds";   // only the tests use limits this short
        }
        if (idleLimit == 1) {
            period = "1 second";
        } else if (idleLimit == 60) {
            period = "1 minute";
        }
        closeMainWindow("For your security, Finora signed you out after " + period + " without activity.");
        return;
    }

    // While the person is active, the session is marked active once a minute.
    if (idleSeconds < 60 && lastKeepAlive.elapsed() >= 60000) {
        lastKeepAlive.restart();
        if (!auth.keepSessionAlive()) {
            closeMainWindow(auth.lastError());
        }
    }
}

// Asks whether the records made before accounts existed should move into
// the new account. Returns a sentence for the welcome message.
QString AppController::offerRecordsWithoutOwner()
{
    int transactions = 0;
    int budgets = 0;
    if (!auth.countRecordsWithoutOwner(transactions, budgets) || (transactions == 0 && budgets == 0)) {
        return QString();
    }

    QString name = QString::fromStdString(auth.session().firstName());
    QMessageBox box(appWindow);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle("Records from before accounts");
    box.setText("This computer has " + countText(transactions, "transaction", "transactions") + " and "
                + countText(budgets, "budget", "budgets") + " that were recorded before accounts existed.");
    box.setInformativeText("Move them into your account, " + name + "? After that only you will see them.\n\n"
                           "\"Not now\" leaves them as they are; you can move them later in Settings.");
    QPushButton *moveButton = box.addButton("Move them into my account", QMessageBox::AcceptRole);
    QPushButton *notNowButton = box.addButton("Not now", QMessageBox::RejectRole);
    box.setDefaultButton(notNowButton);   // Enter never moves records by accident
    box.exec();
    if (box.clickedButton() != moveButton) {
        return QString();
    }

    int movedTransactions = 0;
    int movedBudgets = 0;
    int keptBudgets = 0;
    if (!auth.claimRecordsWithoutOwner(movedTransactions, movedBudgets, keptBudgets)) {
        QMessageBox::warning(appWindow, "Finora", "The records could not be moved, so nothing was changed.\n\n"
                                                  "Reason: " + auth.lastError());
        return QString();
    }
    return " " + countText(movedTransactions, "transaction", "transactions") + " and "
           + countText(movedBudgets, "budget", "budgets") + " moved into your account.";
}

void AppController::closeMainWindow(const QString &message)
{
    // Nobody is signed in now, so this saves the installation's theme:
    // the sign-in window keeps the look the person last used.
    QString theme = "light";
    if (currentThemeMode() == THEME_DARK) {
        theme = "dark";
    }
    database.saveSetting("theme", theme);

    if (appWindow != nullptr) {
        appWindow->hide();
        appWindow->deleteLater();   // deleted once Qt has finished the current event
        appWindow = nullptr;
    }
    signInWindow->refreshTheme();
    signInWindow->showSignIn(message);
    signInWindow->show();
}
