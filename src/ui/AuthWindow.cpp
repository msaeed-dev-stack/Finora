// =====================================================================
//  AuthWindow.cpp
// =====================================================================
#include "AuthWindow.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QVBoxLayout>

#include "Theme.h"
#include "auth/PasswordHasher.h"   // wipe()

AuthWindow::AuthWindow(AuthService &sharedAuth, QWidget *parent)
    : QWidget(parent), auth(sharedAuth)
{
    setObjectName("authWindow");
    setWindowTitle("Finora");
    setWindowIcon(QIcon(":/icons/logo.svg"));
    setMinimumSize(900, 620);
    resize(1040, 680);

    pages = new QStackedWidget(this);
    pages->addWidget(createWelcomePage());   // PAGE_WELCOME
    pages->addWidget(createSignInPage());    // PAGE_SIGN_IN
    pages->addWidget(createSignUpPage());    // PAGE_SIGN_UP
    pages->addWidget(createCodesPage());     // PAGE_CODES
    pages->addWidget(createResetPage());     // PAGE_RESET

    // The form column stays at a comfortable reading width in the middle.
    QWidget *formArea = new QWidget(this);
    formArea->setObjectName("authFormArea");
    QHBoxLayout *formLayout = new QHBoxLayout(formArea);
    formLayout->setContentsMargins(48, 40, 48, 40);
    formLayout->addStretch(1);
    formLayout->addWidget(pages, 4);
    formLayout->addStretch(1);
    pages->setMaximumWidth(440);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(createBrandPanel());
    layout->addWidget(formArea, 1);

    showStart();
}

// ---- Building the pages ---------------------------------------------------

// The violet panel on the left: name, promise, and the honest note that
// these accounts exist only on this computer.
QWidget *AuthWindow::createBrandPanel()
{
    QFrame *panel = new QFrame(this);
    panel->setObjectName("authBrand");
    panel->setFixedWidth(380);

    logoLabel = new QLabel(panel);
    logoLabel->setPixmap(QIcon(":/icons/logo.svg").pixmap(QSize(56, 56)));
    logoLabel->setAccessibleName("Finora logo");

    QLabel *name = new QLabel("Finora", panel);
    name->setObjectName("authBrandTitle");
    QLabel *tagline = new QLabel("Your Money. Your Control.", panel);
    tagline->setObjectName("authBrandText");

    const int POINT_COUNT = 3;
    const char *POINTS[POINT_COUNT] = {
        "Every currency kept on its own, never mixed",
        "Budgets that warn you at 80% of the limit",
        "Passwords protected with Argon2id"
    };

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(40, 48, 40, 40);
    layout->setSpacing(10);
    layout->addWidget(logoLabel);
    layout->addSpacing(8);
    layout->addWidget(name);
    layout->addWidget(tagline);
    layout->addSpacing(28);
    for (int i = 0; i < POINT_COUNT; i++) {
        QLabel *point = new QLabel(QString::fromUtf8("✓  ") + POINTS[i], panel);
        point->setObjectName("authBrandText");
        point->setWordWrap(true);
        layout->addWidget(point);
    }
    layout->addStretch();

    QLabel *note = new QLabel("Local demonstration accounts: your account, password and records stay on this "
                              "computer. Finora has no online account and sends no email.", panel);
    note->setObjectName("authBrandNote");
    note->setWordWrap(true);
    layout->addWidget(note);
    return panel;
}

// A page with a big title, a line of text and an empty body layout.
QWidget *AuthWindow::createPageFrame(const QString &title, const QString &text, QVBoxLayout *&body)
{
    QWidget *page = new QWidget(this);
    QLabel *titleLabel = new QLabel(title, page);
    titleLabel->setObjectName("authTitle");
    titleLabel->setWordWrap(true);
    QLabel *textLabel = new QLabel(text, page);
    textLabel->setObjectName("authText");
    textLabel->setWordWrap(true);

    body = new QVBoxLayout();
    body->setSpacing(8);

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addStretch(1);
    layout->addWidget(titleLabel);
    layout->addWidget(textLabel);
    layout->addSpacing(16);
    layout->addLayout(body);
    layout->addStretch(2);
    return page;
}

QLabel *AuthWindow::createFieldLabel(const QString &text, QWidget *field, QWidget *parent)
{
    QLabel *label = new QLabel(text, parent);
    label->setObjectName("fieldLabel");
    label->setBuddy(field);   // a screen reader reads the label with the field
    return label;
}

QPushButton *AuthWindow::createLinkButton(const QString &text, QWidget *parent)
{
    QPushButton *button = new QPushButton(text, parent);
    button->setObjectName("linkButton");
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

QWidget *AuthWindow::createWelcomePage()
{
    QVBoxLayout *body = nullptr;
    QWidget *page = createPageFrame("Welcome to Finora",
                                    "Create an account to keep your records private on this computer. "
                                    "Each person who uses Finora here gets their own account and sees only "
                                    "their own money.", body);

    QPushButton *create = new QPushButton("Create an account", page);
    create->setObjectName("primaryButton");
    create->setCursor(Qt::PointingHandCursor);
    QPushButton *signIn = new QPushButton("I already have an account", page);
    signIn->setObjectName("secondaryButton");
    signIn->setCursor(Qt::PointingHandCursor);

    body->addWidget(create);
    body->addWidget(signIn);
    connect(create, &QPushButton::clicked, this, &AuthWindow::goToSignUp);
    connect(signIn, &QPushButton::clicked, this, &AuthWindow::goToSignIn);
    return page;
}

QWidget *AuthWindow::createSignInPage()
{
    QVBoxLayout *body = nullptr;
    QWidget *page = createPageFrame("Sign in", "Welcome back. Sign in to see your money.", body);

    signInInfo = new QLabel(page);
    signInInfo->setObjectName("infoBanner");
    signInInfo->setWordWrap(true);
    signInInfo->hide();

    signInEmail = new QLineEdit(page);
    signInEmail->setObjectName("signInEmail");
    signInEmail->setPlaceholderText("name@example.com");
    signInEmail->setAccessibleName("Email address");
    signInPassword = new PasswordField("Password", false, page);
    signInPassword->setObjectName("signInPassword");

    signInError = new QLabel(page);
    signInError->setObjectName("formError");
    signInError->setWordWrap(true);
    signInError->hide();

    signInButton = new QPushButton("Sign in", page);
    signInButton->setObjectName("primaryButton");
    signInButton->setCursor(Qt::PointingHandCursor);
    QPushButton *forgot = createLinkButton("Forgot your password?", page);
    QPushButton *create = createLinkButton("Create an account", page);

    QHBoxLayout *links = new QHBoxLayout();
    links->addWidget(forgot);
    links->addStretch();
    links->addWidget(create);

    body->addWidget(signInInfo);
    body->addWidget(createFieldLabel("Email", signInEmail, page));
    body->addWidget(signInEmail);
    body->addSpacing(4);
    body->addWidget(createFieldLabel("Password", signInPassword->lineEdit(), page));
    body->addWidget(signInPassword);
    body->addWidget(signInError);
    body->addSpacing(6);
    body->addWidget(signInButton);
    body->addLayout(links);

    connect(signInButton, &QPushButton::clicked, this, &AuthWindow::onSignInClicked);
    connect(signInPassword, &PasswordField::returnPressed, this, &AuthWindow::onSignInClicked);
    connect(forgot, &QPushButton::clicked, this, &AuthWindow::goToReset);
    connect(create, &QPushButton::clicked, this, &AuthWindow::goToSignUp);
    return page;
}

QWidget *AuthWindow::createSignUpPage()
{
    QVBoxLayout *body = nullptr;
    QWidget *page = createPageFrame("Create your account",
                                    "Your password protects your records on this computer. Finora stores "
                                    "only a one-way Argon2id hash of it, never the password itself.", body);

    signUpName = new QLineEdit(page);
    signUpName->setObjectName("signUpName");
    signUpName->setPlaceholderText("For example Muhammad Saeed");
    signUpName->setAccessibleName("Your name");
    signUpEmail = new QLineEdit(page);
    signUpEmail->setObjectName("signUpEmail");
    signUpEmail->setPlaceholderText("name@example.com");
    signUpEmail->setAccessibleName("Email address");
    signUpPassword = new PasswordField("New password", true, page);
    signUpPassword->setObjectName("signUpPassword");
    signUpConfirm = new PasswordField("Repeat the password", false, page);
    signUpConfirm->setObjectName("signUpConfirm");

    signUpError = new QLabel(page);
    signUpError->setObjectName("formError");
    signUpError->setWordWrap(true);
    signUpError->hide();

    signUpButton = new QPushButton("Create account", page);
    signUpButton->setObjectName("primaryButton");
    signUpButton->setCursor(Qt::PointingHandCursor);
    QPushButton *back = createLinkButton("I already have an account", page);

    body->addWidget(createFieldLabel("Name", signUpName, page));
    body->addWidget(signUpName);
    body->addWidget(createFieldLabel("Email", signUpEmail, page));
    body->addWidget(signUpEmail);
    body->addWidget(createFieldLabel("Password", signUpPassword->lineEdit(), page));
    body->addWidget(signUpPassword);
    body->addWidget(createFieldLabel("Repeat the password", signUpConfirm->lineEdit(), page));
    body->addWidget(signUpConfirm);
    body->addWidget(signUpError);
    body->addSpacing(6);
    body->addWidget(signUpButton);
    body->addWidget(back, 0, Qt::AlignLeft);

    connect(signUpButton, &QPushButton::clicked, this, &AuthWindow::onSignUpClicked);
    connect(signUpConfirm, &PasswordField::returnPressed, this, &AuthWindow::onSignUpClicked);
    connect(back, &QPushButton::clicked, this, &AuthWindow::goToSignIn);
    return page;
}

QWidget *AuthWindow::createCodesPage()
{
    QVBoxLayout *body = nullptr;
    QWidget *page = createPageFrame("Save your recovery codes",
                                    "If you forget your password, one of these codes lets you set a new one. "
                                    "Each code works only once. Finora shows them only now: keep them somewhere "
                                    "safe, away from this computer.", body);

    codesPanel = new RecoveryCodesPanel(page);
    codesContinueButton = new QPushButton("Continue to Finora", page);
    codesContinueButton->setObjectName("primaryButton");
    codesContinueButton->setCursor(Qt::PointingHandCursor);
    codesContinueButton->setEnabled(false);   // only after "I have saved these codes"

    body->addWidget(codesPanel);
    body->addSpacing(6);
    body->addWidget(codesContinueButton);

    connect(codesPanel, &RecoveryCodesPanel::savedChanged, codesContinueButton, &QPushButton::setEnabled);
    connect(codesContinueButton, &QPushButton::clicked, this, &AuthWindow::onCodesContinueClicked);
    return page;
}

QWidget *AuthWindow::createResetPage()
{
    QVBoxLayout *body = nullptr;
    QWidget *page = createPageFrame("Reset your password",
                                    "Use one of the recovery codes you saved when you created your account. "
                                    "On this local version there is no reset email.", body);

    resetEmail = new QLineEdit(page);
    resetEmail->setObjectName("resetEmail");
    resetEmail->setPlaceholderText("name@example.com");
    resetEmail->setAccessibleName("Email address");
    resetCode = new QLineEdit(page);
    resetCode->setObjectName("resetCode");
    resetCode->setPlaceholderText("XXXX-XXXX-XXXX");
    resetCode->setAccessibleName("Recovery code");
    resetPassword = new PasswordField("New password", true, page);
    resetPassword->setObjectName("resetPassword");
    resetConfirm = new PasswordField("Repeat the new password", false, page);
    resetConfirm->setObjectName("resetConfirm");

    resetError = new QLabel(page);
    resetError->setObjectName("formError");
    resetError->setWordWrap(true);
    resetError->hide();

    resetButton = new QPushButton("Set new password", page);
    resetButton->setObjectName("primaryButton");
    resetButton->setCursor(Qt::PointingHandCursor);
    QPushButton *back = createLinkButton("Back to sign in", page);

    body->addWidget(createFieldLabel("Email", resetEmail, page));
    body->addWidget(resetEmail);
    body->addWidget(createFieldLabel("Recovery code", resetCode, page));
    body->addWidget(resetCode);
    body->addWidget(createFieldLabel("New password", resetPassword->lineEdit(), page));
    body->addWidget(resetPassword);
    body->addWidget(createFieldLabel("Repeat the new password", resetConfirm->lineEdit(), page));
    body->addWidget(resetConfirm);
    body->addWidget(resetError);
    body->addSpacing(6);
    body->addWidget(resetButton);
    body->addWidget(back, 0, Qt::AlignLeft);

    connect(resetButton, &QPushButton::clicked, this, &AuthWindow::onResetClicked);
    connect(back, &QPushButton::clicked, this, &AuthWindow::goToSignIn);
    return page;
}

// ---- Showing pages ------------------------------------------------------------

void AuthWindow::showStart()
{
    if (auth.hasAnyAccount()) {
        showSignIn(QString());
    } else {
        showPage(PAGE_WELCOME);
    }
}

void AuthWindow::showSignIn(const QString &message)
{
    signInInfo->setText(message);
    signInInfo->setVisible(!message.isEmpty());
    signInError->hide();
    showPage(PAGE_SIGN_IN);
}

int AuthWindow::currentPage() const
{
    return pages->currentIndex();
}

void AuthWindow::refreshTheme()
{
    signInPassword->refreshIcons();
    signUpPassword->refreshIcons();
    signUpConfirm->refreshIcons();
    resetPassword->refreshIcons();
    resetConfirm->refreshIcons();
}

void AuthWindow::showPage(int page)
{
    pages->setCurrentIndex(page);
    // The first field of the page gets the keyboard focus.
    if (page == PAGE_SIGN_IN) {
        signInEmail->setFocus();
    } else if (page == PAGE_SIGN_UP) {
        signUpName->setFocus();
    } else if (page == PAGE_RESET) {
        resetEmail->setFocus();
    }
}

void AuthWindow::goToSignIn()
{
    showSignIn(QString());
}

void AuthWindow::goToSignUp()
{
    signUpError->hide();
    showPage(PAGE_SIGN_UP);
}

void AuthWindow::goToReset()
{
    resetError->hide();
    resetEmail->setText(signInEmail->text());
    showPage(PAGE_RESET);
}

void AuthWindow::showError(QLabel *errorLabel, const QString &message)
{
    errorLabel->setText(message);
    errorLabel->show();
}

// Hashing takes about half a second: the button says what is happening
// and the mouse pointer shows that Finora is busy.
void AuthWindow::startBusy(QPushButton *button, const QString &busyText)
{
    button->setEnabled(false);
    button->setText(busyText);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);   // draw the change now
}

void AuthWindow::endBusy(QPushButton *button, const QString &normalText)
{
    QApplication::restoreOverrideCursor();
    button->setText(normalText);
    button->setEnabled(true);
}

// ---- Actions --------------------------------------------------------------------

void AuthWindow::onSignInClicked()
{
    signInError->hide();
    startBusy(signInButton, "Signing in...");
    std::string password = signInPassword->takePassword();
    bool ok = auth.signIn(signInEmail->text().toStdString(), password);   // wipes the password
    endBusy(signInButton, "Sign in");

    if (!ok) {
        showError(signInError, auth.lastError());
        signInPassword->lineEdit()->setFocus();
        return;
    }
    signInInfo->hide();
    emit signedIn(false);
}

void AuthWindow::onSignUpClicked()
{
    signUpError->hide();
    if (signUpPassword->lineEdit()->text() != signUpConfirm->lineEdit()->text()) {
        showError(signUpError, "The two passwords are not the same.");
        return;
    }

    startBusy(signUpButton, "Creating your account...");
    std::string password = signUpPassword->takePassword();
    std::string confirmCopy = signUpConfirm->takePassword();
    PasswordHasher::wipe(confirmCopy);   // already compared; not needed any more
    std::vector<std::string> codes;
    bool ok = auth.signUp(signUpName->text().toStdString(), signUpEmail->text().toStdString(), password, codes);
    endBusy(signUpButton, "Create account");

    if (!ok) {
        showError(signUpError, auth.lastError());
        return;
    }

    // The codes exist only on this page; Finora keeps their hashes only.
    codesPanel->showCodes(codes);
    for (int i = 0; i < static_cast<int>(codes.size()); i++) {
        PasswordHasher::wipe(codes[i]);
    }
    codesContinueButton->setEnabled(false);
    signUpName->clear();
    signUpEmail->clear();
    showPage(PAGE_CODES);
}

void AuthWindow::onCodesContinueClicked()
{
    codesPanel->clearCodes();
    emit signedIn(true);
}

void AuthWindow::onResetClicked()
{
    resetError->hide();
    if (resetPassword->lineEdit()->text() != resetConfirm->lineEdit()->text()) {
        showError(resetError, "The two passwords are not the same.");
        return;
    }

    startBusy(resetButton, "Checking the code...");
    std::string code = resetCode->text().toStdString();
    resetCode->clear();
    std::string newPassword = resetPassword->takePassword();
    std::string confirmCopy = resetConfirm->takePassword();
    PasswordHasher::wipe(confirmCopy);
    int codesLeft = 0;
    bool ok = auth.resetPasswordWithRecoveryCode(resetEmail->text().toStdString(), code, newPassword, codesLeft);
    endBusy(resetButton, "Set new password");

    if (!ok) {
        showError(resetError, auth.lastError());
        return;
    }
    signInEmail->setText(resetEmail->text());
    showSignIn("Your password was changed. You have " + QString::number(codesLeft)
               + " recovery codes left. Please sign in with your new password.");
}
