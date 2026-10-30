// =====================================================================
//  AppController.h
//  Decides which window is shown: the sign-in window while nobody is
//  signed in, the main window while somebody is. It also signs a person
//  out after a time without activity.
// =====================================================================
#ifndef FINORA_APPCONTROLLER_H
#define FINORA_APPCONTROLLER_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#include "AuthWindow.h"
#include "MainWindow.h"
#include "auth/AuthService.h"
#include "core/AccountRules.h"
#include "database/DatabaseManager.h"

class AppController : public QObject
{
    Q_OBJECT

public:
    // idleLimitSeconds: normally SESSION_IDLE_MINUTES; the tests use a few seconds.
    AppController(DatabaseManager &sharedDatabase, AuthService &sharedAuth,
                  int idleLimitSeconds = SESSION_IDLE_MINUTES * 60, QObject *parent = nullptr);
    ~AppController() override;

    // Applies the theme of this installation and shows the sign-in window.
    void start();

    AuthWindow *authWindow() const;
    MainWindow *mainWindow() const;   // nullptr while nobody is signed in

protected:
    // Every key press and mouse action in any Finora window passes here
    // first (an event filter). It only notes that the person is active.
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onSignedIn(bool newAccount);
    void onSignOutRequested();
    void checkSession();

private:
    QString offerRecordsWithoutOwner();
    void closeMainWindow(const QString &message);

    DatabaseManager &database;   // created in main(); used, not owned
    AuthService &auth;           // created in main(); used, not owned
    AuthWindow *signInWindow = nullptr;
    MainWindow *appWindow = nullptr;
    QTimer *sessionTimer = nullptr;
    QElapsedTimer lastActivity;    // time since the last key press or mouse action
    QElapsedTimer lastKeepAlive;   // time since the session was last marked active in the database
    int idleLimit = 0;             // seconds
};

#endif // FINORA_APPCONTROLLER_H
