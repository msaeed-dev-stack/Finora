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

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    bool prepared = query.prepare(
        "INSERT INTO budgets (category, currency_code, limit_minor) "
        "VALUES (:category, :currency, :limit)");
    if (!prepared) {
        return fail("Could not prepare to save the budget: " + query.lastError().text());
    }
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

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    if (!query.exec("SELECT id, category, currency_code, limit_minor FROM budgets "
                    "ORDER BY lower(category), currency_code")) {
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

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    bool prepared = query.prepare(
        "UPDATE budgets SET category = :category, currency_code = :currency, limit_minor = :limit, "
        "updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') WHERE id = :id");
    if (!prepared) {
        return fail("Could not prepare to update the budget: " + query.lastError().text());
    }
    query.bindValue(":category", QString::fromStdString(budget.category));
    query.bindValue(":currency", QString::fromStdString(budget.currencyCode));
    query.bindValue(":limit", budget.limitMinor);
    query.bindValue(":id", budget.id);

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

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    if (!query.prepare("DELETE FROM budgets WHERE id = :id")) {
        return fail("Could not prepare to delete the budget: " + query.lastError().text());
    }
    query.bindValue(":id", id);

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
bool BudgetStore::checkNoDuplicate(const Budget &budget)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    bool prepared = query.prepare(
        "SELECT COUNT(*) FROM budgets "
        "WHERE lower(category) = lower(:category) AND currency_code = :currency AND id <> :id");
    if (!prepared) {
        return fail("Could not prepare to check the budgets: " + query.lastError().text());
    }
    query.bindValue(":category", QString::fromStdString(budget.category));
    query.bindValue(":currency", QString::fromStdString(budget.currencyCode));
    query.bindValue(":id", budget.id);

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
