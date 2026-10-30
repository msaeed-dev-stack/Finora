// =====================================================================
//  AccountDialogs.h
//  The small forms of the Settings page's account section: edit the
//  profile, change the password, and make new recovery codes.
// =====================================================================
#ifndef FINORA_ACCOUNTDIALOGS_H
#define FINORA_ACCOUNTDIALOGS_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "auth/AuthService.h"
#include "components/PasswordField.h"
#include "components/RecoveryCodesPanel.h"

// Name and email. The current password is needed only for a new email.
class EditProfileDialog : public QDialog
{
    Q_OBJECT

public:
    EditProfileDialog(AuthService &sharedAuth, QWidget *parent = nullptr);

private slots:
    void onSaveClicked();

private:
    AuthService &auth;
    QLineEdit *nameEdit = nullptr;
    QLineEdit *emailEdit = nullptr;
    PasswordField *passwordField = nullptr;
    QLabel *errorLabel = nullptr;
    QPushButton *saveButton = nullptr;
};

class ChangePasswordDialog : public QDialog
{
    Q_OBJECT

public:
    ChangePasswordDialog(AuthService &sharedAuth, QWidget *parent = nullptr);

private slots:
    void onSaveClicked();

private:
    AuthService &auth;
    PasswordField *currentField = nullptr;
    PasswordField *newField = nullptr;
    PasswordField *confirmField = nullptr;
    QLabel *errorLabel = nullptr;
    QPushButton *saveButton = nullptr;
};

// Two steps in one window: first the password, then the new codes.
class NewRecoveryCodesDialog : public QDialog
{
    Q_OBJECT

public:
    NewRecoveryCodesDialog(AuthService &sharedAuth, QWidget *parent = nullptr);

private slots:
    void onCreateClicked();

private:
    AuthService &auth;
    QWidget *passwordStep = nullptr;
    QWidget *codesStep = nullptr;
    PasswordField *passwordField = nullptr;
    QLabel *errorLabel = nullptr;
    QPushButton *createButton = nullptr;
    RecoveryCodesPanel *codesPanel = nullptr;
    QPushButton *doneButton = nullptr;
};

// The heading, explanation and button row every account dialog shares.
QVBoxLayout *startAccountDialog(QDialog *dialog, const QString &title, const QString &text);

#endif // FINORA_ACCOUNTDIALOGS_H
