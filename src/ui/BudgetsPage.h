// =====================================================================
//  BudgetsPage.h
//  The Budgets page: monthly spending limits per category, how much of
//  each is used, and buttons to create, edit and delete budgets.
// =====================================================================
#ifndef FINORA_BUDGETSPAGE_H
#define FINORA_BUDGETSPAGE_H

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

#include <vector>

#include "Page.h"
#include "components/MonthSwitcher.h"
#include "core/Budget.h"
#include "core/Transaction.h"
#include "database/BudgetStore.h"
#include "database/DatabaseManager.h"

class BudgetsPage : public Page
{
    Q_OBJECT

public:
    BudgetsPage(DatabaseManager &sharedDatabase, BudgetStore &sharedBudgets, QWidget *parent = nullptr);

    // Loads budgets and transactions again and draws the cards (Page).
    void refresh() override;

    // The actions behind the buttons. Each returns true on success; on a
    // database error it shows a warning. Public so the tests can call them.
    bool saveNewBudget(Budget &budget);
    bool saveEditedBudget(const Budget &budget);
    bool deleteBudgetById(int id);

    MonthSwitcher *monthSwitcher() const;

    // What the page currently shows, for the tests.
    const std::vector<BudgetProgress> &shownProgress() const;

private slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onMonthChanged();

private:
    QWidget *createHeader();
    QFrame *createEmptyState();
    QFrame *createBudgetCard(const BudgetProgress &progress);
    QFrame *createCurrencySummary(const std::string &currencyCode);
    void showBudgets();
    bool findBudget(int id, Budget &result) const;
    void showDatabaseError(const QString &action, const QString &reason);

    DatabaseManager &database;     // created in main(); this page only uses it
    BudgetStore &budgetStore;      // created in MainWindow; this page only uses it
    std::vector<Budget> budgets;
    std::vector<Transaction> transactions;
    std::vector<BudgetProgress> progressList;   // one entry per budget for the chosen month

    MonthSwitcher *switcher = nullptr;
    QWidget *summaryRow = nullptr;
    QHBoxLayout *summaryLayout = nullptr;
    QWidget *cardArea = nullptr;
    QGridLayout *cardGrid = nullptr;
    QFrame *emptyState = nullptr;
    QLabel *emptyIcon = nullptr;
};

#endif // FINORA_BUDGETSPAGE_H
