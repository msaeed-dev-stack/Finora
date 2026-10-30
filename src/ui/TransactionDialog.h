// =====================================================================
//  TransactionDialog.h
//  The form for adding a new transaction or editing an existing one.
// =====================================================================
#ifndef FINORA_TRANSACTIONDIALOG_H
#define FINORA_TRANSACTIONDIALOG_H

#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "core/Transaction.h"

// A pop-up form. The page opens it with exec(), which waits until the
// user clicks Save or Cancel. Save checks the form first: if something
// is wrong, a red message explains it and the dialog stays open, so the
// page only ever receives a valid transaction.
class TransactionDialog : public QDialog
{
    Q_OBJECT   // required because this class has its own slots

public:
    // Two constructors (constructor overloading):
    // one for a new transaction, one that starts with an existing one.
    TransactionDialog(QWidget *parent = nullptr);
    TransactionDialog(const Transaction &existing, QWidget *parent = nullptr);

    // The transaction built from the form. Use it after exec() returned
    // QDialog::Accepted.
    Transaction transaction() const;

private slots:
    void onTypeChanged();
    void onAmountChanged();
    void onSaveClicked();

private:
    void createForm(const QString &title);
    void fillForm(const Transaction &existing);
    void fillCategories(const std::string &type);
    bool readForm(Transaction &entered, QString &problem) const;

    QRadioButton *incomeButton = nullptr;
    QRadioButton *expenseButton = nullptr;
    QComboBox *categoryBox = nullptr;
    QLineEdit *amountEdit = nullptr;
    QComboBox *currencyBox = nullptr;
    QDateEdit *dateEdit = nullptr;
    QLineEdit *descriptionEdit = nullptr;
    QLabel *amountPreview = nullptr;   // shows how the typed amount will be saved
    QLabel *errorLabel = nullptr;      // red message when the form is not valid
    QPushButton *saveButton = nullptr;

    int transactionId = 0;     // 0 for a new transaction, otherwise the id being edited
    // Filled when Save succeeds. (Not called "result": QDialog already has
    // a function result() that tells whether the dialog was accepted.)
    Transaction savedTransaction;
};

#endif // FINORA_TRANSACTIONDIALOG_H
