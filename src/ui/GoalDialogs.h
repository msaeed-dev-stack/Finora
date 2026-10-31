// =====================================================================
//  GoalDialogs.h
//  The three small windows of the Goals page (Milestone 5): the form for
//  a goal, the form for money put in or taken out, and a goal's history.
// =====================================================================
#ifndef FINORA_GOALDIALOGS_H
#define FINORA_GOALDIALOGS_H

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

#include <vector>

#include "core/Goal.h"
#include "database/GoalStore.h"

// The form for a new goal or for editing one. Works like BudgetDialog:
// Save checks the form first, and if something is wrong a red message
// explains it and the form stays open. GoalStore checks everything again.
class GoalDialog : public QDialog
{
    Q_OBJECT

public:
    // A new goal.
    GoalDialog(QWidget *parent = nullptr);
    // An existing goal. Once money is saved in it, its currency is fixed.
    GoalDialog(const Goal &goalToEdit, QWidget *parent = nullptr);

    // The goal built from the form. Use it after exec() returned Accepted.
    Goal goal() const;

private slots:
    void onTargetChanged();
    void onHasDateToggled();
    void onSaveClicked();

private:
    void createForm(const QString &title);
    bool readForm(Goal &entered, QString &problem) const;

    QLineEdit *nameEdit = nullptr;
    QComboBox *currencyBox = nullptr;
    QLineEdit *targetEdit = nullptr;
    QLabel *targetPreview = nullptr;
    QCheckBox *hasDateBox = nullptr;
    QDateEdit *dateEdit = nullptr;
    QLabel *errorLabel = nullptr;

    int goalId = 0;          // 0 for a new goal, otherwise the id being edited
    long long saved = 0;     // what the goal being edited has saved
    Goal savedGoal;          // filled when Save succeeds
};

// The form for money put into a goal or taken out of it.
class ContributionDialog : public QDialog
{
    Q_OBJECT

public:
    // `takeOut` is false for "Add money" and true for "Take out".
    ContributionDialog(const Goal &goal, bool takeOut, QWidget *parent = nullptr);

    // The contribution built from the form: a positive amount for money
    // put in, a negative amount for money taken out.
    GoalContribution contribution() const;

private slots:
    void onAmountChanged();
    void onSaveClicked();

private:
    bool readForm(GoalContribution &entered, QString &problem) const;

    Goal goal;               // a copy of the goal, for its name, currency and saved amount
    bool takingOut = false;
    QLineEdit *amountEdit = nullptr;
    QLabel *amountPreview = nullptr;
    QDateEdit *dateEdit = nullptr;
    QLineEdit *noteEdit = nullptr;
    QLabel *errorLabel = nullptr;
    GoalContribution savedContribution;   // filled when Save succeeds
};

// Every contribution of one goal, newest first. Read-only.
class GoalHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    GoalHistoryDialog(GoalStore &goalStore, const Goal &goal, QWidget *parent = nullptr);

    // How many contributions the table shows (for the tests).
    int shownCount() const;

private:
    QTableWidget *table = nullptr;
    int rowCount = 0;
};

#endif // FINORA_GOALDIALOGS_H
