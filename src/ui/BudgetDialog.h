// =====================================================================
//  BudgetDialog.h
//  The form for creating a new budget or editing an existing one.
// =====================================================================
#ifndef FINORA_BUDGETDIALOG_H
#define FINORA_BUDGETDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <vector>

#include "core/Budget.h"

// Works like TransactionDialog: Save checks the form first, and if
// something is wrong a red message explains it and the form stays open.
class BudgetDialog : public QDialog
{
    Q_OBJECT

public:
    // Two constructors: one for a new budget, one that starts with an
    // existing budget. `existingBudgets` lets the form refuse a second
    // budget for the same category and currency before saving.
    BudgetDialog(const std::vector<Budget> &existingBudgets, QWidget *parent = nullptr);
    BudgetDialog(const Budget &budgetToEdit, const std::vector<Budget> &existingBudgets,
                 QWidget *parent = nullptr);

    // The budget built from the form. Use it after exec() returned Accepted.
    Budget budget() const;

private slots:
    void onLimitChanged();
    void onSaveClicked();

private:
    void createForm(const QString &title);
    bool readForm(Budget &entered, QString &problem) const;

    QComboBox *categoryBox = nullptr;
    QComboBox *currencyBox = nullptr;
    QLineEdit *limitEdit = nullptr;
    QLabel *limitPreview = nullptr;
    QLabel *errorLabel = nullptr;
    QPushButton *saveButton = nullptr;

    std::vector<Budget> otherBudgets;   // copies of the budgets that already exist
    int budgetId = 0;                   // 0 for a new budget, otherwise the id being edited
    Budget savedBudget;                 // filled when Save succeeds
};

#endif // FINORA_BUDGETDIALOG_H
