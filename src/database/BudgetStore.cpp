// =====================================================================
//  BudgetStore.cpp
// =====================================================================
#include "BudgetStore.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

BudgetStore::BudgetStore(DatabaseManager &sharedDatabase)
    : database(sharedDatabase)
{
}

bool BudgetStore::addBudget(Budget &budget)
{
    errorMessage.clear();
    limitReached = false;
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }

    std::string problem = validateBudget(budget);
    if (!problem.empty()) {
        return fail(QString::fromStdString(problem));
    }
    if (!checkNoDuplicate(budget)) {
        return false;
    }

    // The new budget belongs to the signed-in person (see DatabaseManager::currentOwner).
    QVariant owner;
    if (!database.currentOwner(owner)) {
        return fail(database.lastError());
    }

    // The plan decides how many budgets may exist (Phase 4C). This check
    // is here, below every screen, so no screen can skip it.
    int limit = database.budgetLimit();
    if (limit != UNLIMITED_BUDGETS) {
        int existing = 0;
        if (!countBudgets(owner, existing)) {
            return false;
        }
        if (existing >= limit) {
            limitReached = true;
            return fail(QString::fromStdString(budgetLimitMessage(database.currentPlan())));
        }
    }

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    bool prepared = query.prepare(
        "INSERT INTO budgets (user_id, category, currency_code, limit_minor) "
        "VALUES (:owner, :category, :currency, :limit)");
    if (!prepared) {
        return fail("Could not prepare to save the budget: " + query.lastError().text());
    }
    query.bindValue(":owner", owner);
    query.bindValue(":category", QString::fromStdString(budget.category));
    query.bindValue(":currency", QString::fromStdString(budget.currencyCode));
    query.bindValue(":limit", budget.limitMinor);

    if (!query.exec()) {
        return fail("Could not save the budget: " + query.lastError().text());
    }
    budget.id = query.lastInsertId().toInt();
    return true;
}

bool BudgetStore::loadBudgets(std::vector<Budget> &results)
{
    errorMessage.clear();
    results.clear();
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }

    QVariant owner;
    if (!database.currentOwner(owner)) {
        return fail(database.lastError());
    }

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("SELECT id, category, currency_code, limit_minor FROM budgets "
                  "WHERE user_id IS :owner ORDER BY lower(category), currency_code");
    query.bindValue(":owner", owner);
    if (!query.exec()) {
        return fail("Could not load the budgets: " + query.lastError().text());
    }

    while (query.next()) {
        Budget budget;
        budget.id = query.value(0).toInt();
        budget.category = query.value(1).toString().toStdString();
        budget.currencyCode = query.value(2).toString().toStdString();
        budget.limitMinor = query.value(3).toLongLong();
        results.push_back(budget);
    }

    // Budgets over the plan's limit are paused, not deleted (Phase 4C).
    // Every screen loads budgets here, so every screen sees the same rule.
    markPausedBudgets(results, database.budgetLimit());
    return true;
}

bool BudgetStore::updateBudget(const Budget &budget)
{
    errorMessage.clear();
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    if (budget.id <= 0) {
        return fail("A budget id must be a number above 0.");
    }

    std::string problem = validateBudget(budget);
    if (!problem.empty()) {
        return fail(QString::fromStdString(problem));
    }
    if (!checkNoDuplicate(budget)) {
        return false;
    }

    QVariant owner;
    if (!database.currentOwner(owner)) {
        return fail(database.lastError());
    }

    // "AND user_id IS :owner": another person's budget id is simply "not found".
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    bool prepared = query.prepare(
        "UPDATE budgets SET category = :category, currency_code = :currency, limit_minor = :limit, "
        "updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') WHERE id = :id AND user_id IS :owner");
    if (!prepared) {
        return fail("Could not prepare to update the budget: " + query.lastError().text());
    }
    query.bindValue(":category", QString::fromStdString(budget.category));
    query.bindValue(":currency", QString::fromStdString(budget.currencyCode));
    query.bindValue(":limit", budget.limitMinor);
    query.bindValue(":id", budget.id);
    query.bindValue(":owner", owner);

    if (!query.exec()) {
        return fail("Could not update the budget: " + query.lastError().text());
    }
    if (query.numRowsAffected() == 0) {
        return fail("No budget with id " + QString::number(budget.id) + " was found.");
    }
    return true;
}

bool BudgetStore::deleteBudget(int id)
{
    errorMessage.clear();
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    if (id <= 0) {
        return fail("A budget id must be a number above 0.");
    }

    QVariant owner;
    if (!database.currentOwner(owner)) {
        return fail(database.lastError());
    }

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    if (!query.prepare("DELETE FROM budgets WHERE id = :id AND user_id IS :owner")) {
        return fail("Could not prepare to delete the budget: " + query.lastError().text());
    }
    query.bindValue(":id", id);
    query.bindValue(":owner", owner);

    if (!query.exec()) {
        return fail("Could not delete the budget: " + query.lastError().text());
    }
    if (query.numRowsAffected() == 0) {
        return fail("No budget with id " + QString::number(id) + " was found.");
    }
    return true;
}

QString BudgetStore::lastError() const
{
    return errorMessage;
}

// Refuses a second budget for the same category (in any upper or lower
// case) and currency. The budget being edited (same id) does not count.
// The database has a UNIQUE index with the same rule as a second guard.
bool BudgetStore::planLimitReached() const
{
    return limitReached;
}

// How many budgets this owner has, active or paused.
bool BudgetStore::countBudgets(const QVariant &owner, int &count)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("SELECT COUNT(*) FROM budgets WHERE user_id IS :owner");
    query.bindValue(":owner", owner);
    if (!query.exec() || !query.next()) {
        return fail("Could not count the budgets: " + query.lastError().text());
    }
    count = query.value(0).toInt();
    return true;
}

bool BudgetStore::checkNoDuplicate(const Budget &budget)
{
    QVariant owner;
    if (!database.currentOwner(owner)) {
        return fail(database.lastError());
    }

    // Only this owner's budgets count: two people may both have a Food budget in PKR.
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    bool prepared = query.prepare(
        "SELECT COUNT(*) FROM budgets "
        "WHERE lower(category) = lower(:category) AND currency_code = :currency AND id <> :id "
        "AND user_id IS :owner");
    if (!prepared) {
        return fail("Could not prepare to check the budgets: " + query.lastError().text());
    }
    query.bindValue(":category", QString::fromStdString(budget.category));
    query.bindValue(":currency", QString::fromStdString(budget.currencyCode));
    query.bindValue(":id", budget.id);
    query.bindValue(":owner", owner);

    if (!query.exec() || !query.next()) {
        return fail("Could not check the budgets: " + query.lastError().text());
    }
    if (query.value(0).toInt() > 0) {
        return fail("There is already a " + QString::fromStdString(budget.category) + " budget in "
                    + QString::fromStdString(budget.currencyCode) + ". Edit that budget instead.");
    }
    return true;
}

bool BudgetStore::fail(const QString &message)
{
    errorMessage = message;
    return false;
}
