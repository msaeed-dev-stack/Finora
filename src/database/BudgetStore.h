// =====================================================================
//  BudgetStore.h
//  Saves, loads, changes and deletes budgets in the SQLite database.
// =====================================================================
#ifndef FINORA_BUDGETSTORE_H
#define FINORA_BUDGETSTORE_H

#include <QString>

#include <vector>

#include "DatabaseManager.h"
#include "core/Budget.h"

// The budget part of the database. It does not open its own connection:
// it uses the one that DatabaseManager opened (composition: a
// BudgetStore HAS a DatabaseManager it works with, it does not own it).
class BudgetStore
{
public:
    BudgetStore(DatabaseManager &sharedDatabase);

    // Saves a valid budget. On success, budget.id is set to the new
    // budget's number. A second budget for the same category and
    // currency is refused with a clear message.
    bool addBudget(Budget &budget);

    // Loads every budget, sorted by category and then currency.
    bool loadBudgets(std::vector<Budget> &results);

    // Saves new values for an existing budget, found by budget.id.
    bool updateBudget(const Budget &budget);

    // Deletes the budget with the given id. Transactions are not touched.
    bool deleteBudget(int id);

    QString lastError() const;

private:
    bool checkNoDuplicate(const Budget &budget);
    bool fail(const QString &message);

    DatabaseManager &database;   // created in main(); this store only uses it
    QString errorMessage;
};

#endif // FINORA_BUDGETSTORE_H
