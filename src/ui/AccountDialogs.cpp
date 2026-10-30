// =====================================================================
//  AccountDialogs.cpp
// =====================================================================
#include "AccountDialogs.h"

#include <QApplication>
#include <QHBoxLayout>

#include "auth/PasswordHasher.h"   // wipe()

QVBoxLayout *startAccountDialog(QDialog *dialog, const QString &title, const QString &text)
{
    dialog->setWindowTitle(title);
    dialog->setModal(true);
    dialog->setMinimumWidth(460);

    QLabel *heading = new QLabel(title, dialog);
    heading->setObjectName("dialogTitle");
    QLabel *help = new QLabel(text, dialog);
    help->setObjectName("mutedText");
    help->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(10);
    layout->addWidget(heading);
    layout->addWidget(help);
    layout->addSpacing(4);
    return layout;
}

// Cancel and a primary button on the right.
QHBoxLayout *buttonRow(QDialog *dialog, QPushButton *primary)
{
    QPushButton *cancel = new QPushButton("Cancel", dialog);
    cancel->setObjectName("secondaryButton");
    primary->setObjectName("primaryButton");
    primary->setDefault(true);
    QObject::connect(cancel, &QPushButton::clicked, dialog, &QDialog::reject);

    QHBoxLayout *row = new QHBoxLayout();
    row->addStretch();
    row->addWidget(cancel);
    row->addWidget(primary);
    return row;
}

QLabel *createErrorLabel(QWidget *parent)
{
    QLabel *label = new QLabel(parent);
    label->setObjectName("formError");
    label->setWordWrap(true);
    label->hide();
    return label;
}

// ---- Edit profile ---------------------------------------------------------------

EditProfileDialog::EditProfileDialog(AuthService &sharedAuth, QWidget *parent)
    : QDialog(parent), auth(sharedAuth)
{
    QVBoxLayout *layout = startAccountDialog(this, "Edit profile",
                                             "Your name appears in greetings. Your email address is what "
                                             "you sign in with.");

    nameEdit = new QLineEdit(QString::fromStdString(auth.session().displayName()), this);
    nameEdit->setObjectName("profileNameEdit");
    nameEdit->setAccessibleName("Name");
    emailEdit = new QLineEdit(QString::fromStdString(auth.session().email()), this);
    emailEdit->setObjectName("profileEmailEdit");
    emailEdit->setAccessibleName("Email address");
    passwordField = new PasswordField("Current password", false, this);
    passwordField->setObjectName("profilePassword");
    QLabel *passwordNote = new QLabel("Needed only when you change your email address.", this);
    passwordNote->setObjectName("mutedText");
    errorLabel = createErrorLabel(this);
    saveButton = new QPushButton("Save profile", this);

    layout->addWidget(new QLabel("Name", this));
    layout->addWidget(nameEdit);
    layout->addWidget(new QLabel("Email", this));
    layout->addWidget(emailEdit);
    layout->addWidget(new QLabel("Current password", this));
    layout->addWidget(passwordField);
    layout->addWidget(passwordNote);
    layout->addWidget(errorLabel);
    layout->addLayout(buttonRow(this, saveButton));

    connect(saveButton, &QPushButton::clicked, this, &EditProfileDialog::onSaveClicked);
}

void EditProfileDialog::onSaveClicked()
{
    std::string password = passwordField->takePassword();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool saved = auth.updateProfile(nameEdit->text().toStdString(), emailEdit->text().toStdString(), password);
    QApplication::restoreOverrideCursor();
    if (!saved) {
        errorLabel->setText(auth.lastError());
        errorLabel->show();
        adjustSize();
        return;
    }
    accept();
}

// ---- Change password ------------------------------------------------------------

ChangePasswordDialog::ChangePasswordDialog(AuthService &sharedAuth, QWidget *parent)
    : QDialog(parent), auth(sharedAuth)
{
    QVBoxLayout *layout = startAccountDialog(this, "Change password",
                                             "Choose a new password of at least 10 characters. Your recovery "
                                             "codes keep working.");

    currentField = new PasswordField("Current password", false, this);
    currentField->setObjectName("currentPassword");
    newField = new PasswordField("New password", true, this);
    newField->setObjectName("newPassword");
    confirmField = new PasswordField("Repeat the new password", false, this);
    confirmField->setObjectName("confirmPassword");
    errorLabel = createErrorLabel(this);
    saveButton = new QPushButton("Change password", this);

    layout->addWidget(new QLabel("Current password", this));
    layout->addWidget(currentField);
    layout->addWidget(new QLabel("New password", this));
    layout->addWidget(newField);
    layout->addWidget(new QLabel("Repeat the new password", this));
    layout->addWidget(confirmField);
    layout->addWidget(errorLabel);
    layout->addLayout(buttonRow(this, saveButton));

    connect(saveButton, &QPushButton::clicked, this, &ChangePasswordDialog::onSaveClicked);
}

void ChangePasswordDialog::onSaveClicked()
{
    if (newField->lineEdit()->text() != confirmField->lineEdit()->text()) {
        errorLabel->setText("The two new passwords are not the same.");
        errorLabel->show();
        adjustSize();
        return;
    }
    std::string current = currentField->takePassword();
    std::string fresh = newField->takePassword();
    std::string confirmCopy = confirmField->takePassword();
    PasswordHasher::wipe(confirmCopy);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool changed = auth.changePassword(current, fresh);   // wipes both
    QApplication::restoreOverrideCursor();
    if (!changed) {
        errorLabel->setText(auth.lastError());
        errorLabel->show();
        adjustSize();
        return;
    }
    accept();
}

// ---- New recovery codes ---------------------------------------------------------

NewRecoveryCodesDialog::NewRecoveryCodesDialog(AuthService &sharedAuth, QWidget *parent)
    : QDialog(parent), auth(sharedAuth)
{
    QVBoxLayout *layout = startAccountDialog(this, "New recovery codes",
                                             "New codes replace ALL your old codes at once, used or not. "
                                             "Enter your password to continue.");

    // Step 1: the password.
    passwordStep = new QWidget(this);
    passwordField = new PasswordField("Current password", false, passwordStep);
    passwordField->setObjectName("codesPassword");
    errorLabel = createErrorLabel(passwordStep);
    createButton = new QPushButton("Create new codes", passwordStep);
    QVBoxLayout *passwordLayout = new QVBoxLayout(passwordStep);
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->addWidget(passwordField);
    passwordLayout->addWidget(errorLabel);
    passwordLayout->addLayout(buttonRow(this, createButton));

    // Step 2: the codes, shown once.
    codesStep = new QWidget(this);
    codesPanel = new RecoveryCodesPanel(codesStep);
    doneButton = new QPushButton("Done", codesStep);
    doneButton->setObjectName("primaryButton");
    doneButton->setEnabled(false);
    QVBoxLayout *codesLayout = new QVBoxLayout(codesStep);
    codesLayout->setContentsMargins(0, 0, 0, 0);
    codesLayout->addWidget(codesPanel);
    codesLayout->addWidget(doneButton, 0, Qt::AlignRight);
    codesStep->hide();

    layout->addWidget(passwordStep);
    layout->addWidget(codesStep);

    connect(createButton, &QPushButton::clicked, this, &NewRecoveryCodesDialog::onCreateClicked);
    connect(codesPanel, &RecoveryCodesPanel::savedChanged, doneButton, &QPushButton::setEnabled);
    connect(doneButton, &QPushButton::clicked, this, &QDialog::accept);
}

void NewRecoveryCodesDialog::onCreateClicked()
{
    std::string password = passwordField->takePassword();
    std::vector<std::string> codes;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool made = auth.regenerateRecoveryCodes(password, codes);   // wipes the password
    QApplication::restoreOverrideCursor();
    if (!made) {
        errorLabel->setText(auth.lastError());
        errorLabel->show();
        adjustSize();
        return;
    }
    codesPanel->showCodes(codes);
    for (int i = 0; i < static_cast<int>(codes.size()); i++) {
        PasswordHasher::wipe(codes[i]);
    }
    passwordStep->hide();
    codesStep->show();
    adjustSize();
}
