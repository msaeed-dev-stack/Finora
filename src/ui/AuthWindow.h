// =====================================================================
//  AuthWindow.h
//  The window before signing in: welcome, sign in, create an account,
//  the new account's recovery codes, and resetting a forgotten password.
// =====================================================================
#ifndef FINORA_AUTHWINDOW_H
#define FINORA_AUTHWINDOW_H

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "auth/AuthService.h"
#include "components/PasswordField.h"
#include "components/RecoveryCodesPanel.h"

class AuthWindow : public QWidget
{
    Q_OBJECT

public:
    // The pages of the window, in the order they are added.
    static const int PAGE_WELCOME = 0;
    static const int PAGE_SIGN_IN = 1;
    static const int PAGE_SIGN_UP = 2;
    static const int PAGE_CODES = 3;
    static const int PAGE_RESET = 4;

    AuthWindow(AuthService &sharedAuth, QWidget *parent = nullptr);

    // Welcome when this computer has no account yet, otherwise Sign in.
    void showStart();

    // The sign-in page with a message, for example "You have signed out."
    void showSignIn(const QString &message);

    int currentPage() const;

    // Colours the icons again after the theme changed.
    void refreshTheme();

signals:
    // Someone signed in. newAccount is true right after creating an account.
    void signedIn(bool newAccount);

private slots:
    void onSignInClicked();
    void onSignUpClicked();
    void onCodesContinueClicked();
    void onResetClicked();
    void goToSignIn();
    void goToSignUp();
    void goToReset();

private:
    QWidget *createBrandPanel();
    QWidget *createWelcomePage();
    QWidget *createSignInPage();
    QWidget *createSignUpPage();
    QWidget *createCodesPage();
    QWidget *createResetPage();
    QWidget *createPageFrame(const QString &title, const QString &text, QVBoxLayout *&body);
    QLabel *createFieldLabel(const QString &text, QWidget *field, QWidget *parent);
    QPushButton *createLinkButton(const QString &text, QWidget *parent);

    void showPage(int page);
    void showError(QLabel *errorLabel, const QString &message);
    void startBusy(QPushButton *button, const QString &busyText);
    void endBusy(QPushButton *button, const QString &normalText);

    AuthService &auth;   // created in main(); this window only uses it
    QStackedWidget *pages = nullptr;
    QLabel *logoLabel = nullptr;

    QLineEdit *signInEmail = nullptr;
    PasswordField *signInPassword = nullptr;
    QLabel *signInInfo = nullptr;
    QLabel *signInError = nullptr;
    QPushButton *signInButton = nullptr;

    QLineEdit *signUpName = nullptr;
    QLineEdit *signUpEmail = nullptr;
    PasswordField *signUpPassword = nullptr;
    PasswordField *signUpConfirm = nullptr;
    QLabel *signUpError = nullptr;
    QPushButton *signUpButton = nullptr;

    RecoveryCodesPanel *codesPanel = nullptr;
    QPushButton *codesContinueButton = nullptr;

    QLineEdit *resetEmail = nullptr;
    QLineEdit *resetCode = nullptr;
    PasswordField *resetPassword = nullptr;
    PasswordField *resetConfirm = nullptr;
    QLabel *resetError = nullptr;
    QPushButton *resetButton = nullptr;
};

#endif // FINORA_AUTHWINDOW_H
