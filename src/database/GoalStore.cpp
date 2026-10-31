// =====================================================================
//  GoalStore.cpp
// =====================================================================
#include "GoalStore.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

// A goal without a date is stored as SQL NULL (an empty QVariant).
QVariant targetDateValue(const Goal &goal)
{
    if (goal.targetDate.empty()) {
        return QVariant();
    }
    return QString::fromStdString(goal.targetDate);
}

GoalStore::GoalStore(DatabaseManager &sharedDatabase)
    : database(sharedDatabase)
{
}

bool GoalStore::addGoal(Goal &goal)
{
    errorMessage.clear();
    blockedByPlan = false;
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    if (!checkPlan()) {
        return false;
    }
    std::string problem = validateGoal(goal);
    if (!problem.empty()) {
        return fail(QString::fromStdString(problem));
    }
    int ownerId = 0;
    if (!database.signedInUserId(ownerId)) {
        return fail("Please sign in first.");
    }
    if (!checkNoDuplicate(goal, ownerId)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    bool prepared = query.prepare(
        "INSERT INTO goals (user_id, name, currency_code, target_minor, target_date) "
        "VALUES (:owner, :name, :currency, :target, :date)");
    if (!prepared) {
        return fail("Could not prepare to save the goal: " + query.lastError().text());
    }
    query.bindValue(":owner", ownerId);
    query.bindValue(":name", QString::fromStdString(goal.name).trimmed());
    query.bindValue(":currency", QString::fromStdString(goal.currencyCode));
    query.bindValue(":target", goal.targetMinor);
    query.bindValue(":date", targetDateValue(goal));

    if (!query.exec()) {
        return fail("Could not save the goal: " + query.lastError().text());
    }
    goal.id = query.lastInsertId().toInt();
    goal.savedMinor = 0;
    return true;
}

bool GoalStore::loadGoals(std::vector<Goal> &results)
{
    errorMessage.clear();
    results.clear();
    blockedByPlan = false;
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    int ownerId = 0;
    if (!database.signedInUserId(ownerId)) {
        return fail("Please sign in first.");
    }

    // LEFT JOIN keeps goals that have no contributions yet; for them
    // SUM() is NULL, which COALESCE turns into 0.
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare(
        "SELECT g.id, g.name, g.currency_code, g.target_minor, COALESCE(g.target_date, ''), "
        "       COALESCE(SUM(c.amount_minor), 0) "
        "FROM goals g LEFT JOIN goal_contributions c ON c.goal_id = g.id "
        "WHERE g.user_id = :owner "
        "GROUP BY g.id ORDER BY lower(g.name)");
    query.bindValue(":owner", ownerId);
    if (!query.exec()) {
        return fail("Could not load the goals: " + query.lastError().text());
    }

    while (query.next()) {
        Goal goal;
        goal.id = query.value(0).toInt();
        goal.name = query.value(1).toString().toStdString();
        goal.currencyCode = query.value(2).toString().toStdString();
        goal.targetMinor = query.value(3).toLongLong();
        goal.targetDate = query.value(4).toString().toStdString();
        goal.savedMinor = query.value(5).toLongLong();
        results.push_back(goal);
    }
    return true;
}

bool GoalStore::updateGoal(const Goal &goal)
{
    errorMessage.clear();
    blockedByPlan = false;
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    if (!checkPlan()) {
        return false;
    }
    if (goal.id <= 0) {
        return fail("A goal id must be a number above 0.");
    }
    std::string problem = validateGoal(goal);
    if (!problem.empty()) {
        return fail(QString::fromStdString(problem));
    }
    int ownerId = 0;
    if (!database.signedInUserId(ownerId)) {
        return fail("Please sign in first.");
    }
    if (!checkNoDuplicate(goal, ownerId)) {
        return false;
    }

    // The saved amount is in the goal's currency, so the currency may
    // only change while nothing has been put in.
    QSqlQuery check(QSqlDatabase::database(database.sqlConnectionName(), false));
    check.prepare("SELECT currency_code, (SELECT COUNT(*) FROM goal_contributions WHERE goal_id = goals.id) "
                  "FROM goals WHERE id = :id AND user_id = :owner");
    check.bindValue(":id", goal.id);
    check.bindValue(":owner", ownerId);
    if (!check.exec()) {
        return fail("Could not read the goal: " + check.lastError().text());
    }
    if (!check.next()) {
        return fail("No goal with id " + QString::number(goal.id) + " was found.");
    }
    bool currencyChanged = check.value(0).toString().toStdString() != goal.currencyCode;
    if (currencyChanged && check.value(1).toInt() > 0) {
        return fail("The currency cannot change after money has been put into the goal. "
                    "Create a new goal for another currency instead.");
    }

    // "AND user_id = :owner": another person's goal id is simply "not found".
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    bool prepared = query.prepare(
        "UPDATE goals SET name = :name, currency_code = :currency, target_minor = :target, "
        "target_date = :date, updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') "
        "WHERE id = :id AND user_id = :owner");
    if (!prepared) {
        return fail("Could not prepare to update the goal: " + query.lastError().text());
    }
    query.bindValue(":name", QString::fromStdString(goal.name).trimmed());
    query.bindValue(":currency", QString::fromStdString(goal.currencyCode));
    query.bindValue(":target", goal.targetMinor);
    query.bindValue(":date", targetDateValue(goal));
    query.bindValue(":id", goal.id);
    query.bindValue(":owner", ownerId);

    if (!query.exec()) {
        return fail("Could not update the goal: " + query.lastError().text());
    }
    if (query.numRowsAffected() == 0) {
        return fail("No goal with id " + QString::number(goal.id) + " was found.");
    }
    return true;
}

bool GoalStore::deleteGoal(int id)
{
    errorMessage.clear();
    blockedByPlan = false;
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    if (id <= 0) {
        return fail("A goal id must be a number above 0.");
    }
    // No plan check: a person may always delete their own goal.
    int ownerId = 0;
    if (!database.signedInUserId(ownerId)) {
        return fail("Please sign in first.");
    }

    // The goal's contributions are deleted with it (ON DELETE CASCADE).
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    if (!query.prepare("DELETE FROM goals WHERE id = :id AND user_id = :owner")) {
        return fail("Could not prepare to delete the goal: " + query.lastError().text());
    }
    query.bindValue(":id", id);
    query.bindValue(":owner", ownerId);

    if (!query.exec()) {
        return fail("Could not delete the goal: " + query.lastError().text());
    }
    if (query.numRowsAffected() == 0) {
        return fail("No goal with id " + QString::number(id) + " was found.");
    }
    return true;
}

bool GoalStore::addContribution(GoalContribution &contribution)
{
    errorMessage.clear();
    blockedByPlan = false;
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    if (!checkPlan()) {
        return false;
    }
    if (contribution.goalId <= 0) {
        return fail("A goal id must be a number above 0.");
    }
    int ownerId = 0;
    if (!database.signedInUserId(ownerId)) {
        return fail("Please sign in first.");
    }

    // Reading the saved amount, checking and saving happen in ONE
    // transaction, so nothing can change the saved amount in between.
    QSqlDatabase db = QSqlDatabase::database(database.sqlConnectionName(), false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }

    long long savedMinor = 0;
    if (!savedAmount(contribution.goalId, ownerId, savedMinor)) {
        db.rollback();
        return false;   // savedAmount() has explained the problem
    }
    std::string problem = validateContribution(contribution, savedMinor);
    if (!problem.empty()) {
        db.rollback();
        return fail(QString::fromStdString(problem));
    }

    QSqlQuery query(db);
    bool prepared = query.prepare(
        "INSERT INTO goal_contributions (goal_id, amount_minor, contribution_date, note) "
        "VALUES (:goal, :amount, :date, :note)");
    if (!prepared) {
        QString reason = query.lastError().text();
        db.rollback();
        return fail("Could not prepare to save the money: " + reason);
    }
    query.bindValue(":goal", contribution.goalId);
    query.bindValue(":amount", contribution.amountMinor);
    query.bindValue(":date", QString::fromStdString(contribution.date));
    query.bindValue(":note", QString::fromStdString(contribution.note).trimmed());
    if (!query.exec()) {
        QString reason = query.lastError().text();
        db.rollback();
        return fail("Could not save the money: " + reason);
    }
    int newId = query.lastInsertId().toInt();

    if (!db.commit()) {
        QString reason = db.lastError().text();
        db.rollback();
        return fail("Could not save the money: " + reason);
    }
    contribution.id = newId;
    return true;
}

bool GoalStore::loadContributions(int goalId, std::vector<GoalContribution> &results)
{
    errorMessage.clear();
    results.clear();
    blockedByPlan = false;
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    int ownerId = 0;
    if (!database.signedInUserId(ownerId)) {
        return fail("Please sign in first.");
    }

    // The JOIN with goals keeps another person's contributions out.
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare(
        "SELECT c.id, c.goal_id, c.amount_minor, c.contribution_date, c.note "
        "FROM goal_contributions c JOIN goals g ON g.id = c.goal_id "
        "WHERE c.goal_id = :goal AND g.user_id = :owner "
        "ORDER BY c.contribution_date DESC, c.id DESC");
    query.bindValue(":goal", goalId);
    query.bindValue(":owner", ownerId);
    if (!query.exec()) {
        return fail("Could not load the history: " + query.lastError().text());
    }

    while (query.next()) {
        GoalContribution contribution;
        contribution.id = query.value(0).toInt();
        contribution.goalId = query.value(1).toInt();
        contribution.amountMinor = query.value(2).toLongLong();
        contribution.date = query.value(3).toString().toStdString();
        contribution.note = query.value(4).toString().toStdString();
        results.push_back(contribution);
    }
    return true;
}

QString GoalStore::lastError() const
{
    return errorMessage;
}

bool GoalStore::planBlocked() const
{
    return blockedByPlan;
}

// The one plan check of this class: requireFeature() reads the plan from
// the signed-in person's subscription and explains which plan is needed.
bool GoalStore::checkPlan()
{
    if (!database.requireFeature(FEATURE_SAVINGS_GOALS)) {
        blockedByPlan = true;
        return fail(database.lastError());
    }
    return true;
}

// Refuses a second goal with the same name (in any upper or lower case).
// The goal being edited (same id) does not count. The database has a
// UNIQUE index with the same rule as a second guard.
bool GoalStore::checkNoDuplicate(const Goal &goal, int ownerId)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    bool prepared = query.prepare(
        "SELECT COUNT(*) FROM goals WHERE lower(name) = lower(:name) AND id <> :id AND user_id = :owner");
    if (!prepared) {
        return fail("Could not prepare to check the goals: " + query.lastError().text());
    }
    query.bindValue(":name", QString::fromStdString(goal.name).trimmed());
    query.bindValue(":id", goal.id);
    query.bindValue(":owner", ownerId);

    if (!query.exec() || !query.next()) {
        return fail("Could not check the goals: " + query.lastError().text());
    }
    if (query.value(0).toInt() > 0) {
        return fail("There is already a goal called " + QString::fromStdString(goal.name).trimmed()
                    + ". Please choose another name.");
    }
    return true;
}

// The sum of a goal's contributions. Fails with "not found" if the goal
// does not exist or belongs to someone else.
bool GoalStore::savedAmount(int goalId, int ownerId, long long &savedMinor)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("SELECT (SELECT COALESCE(SUM(amount_minor), 0) FROM goal_contributions WHERE goal_id = goals.id) "
                  "FROM goals WHERE id = :id AND user_id = :owner");
    query.bindValue(":id", goalId);
    query.bindValue(":owner", ownerId);
    if (!query.exec()) {
        return fail("Could not read the goal: " + query.lastError().text());
    }
    if (!query.next()) {
        return fail("No goal with id " + QString::number(goalId) + " was found.");
    }
    savedMinor = query.value(0).toLongLong();
    return true;
}

bool GoalStore::fail(const QString &message)
{
    errorMessage = message;
    return false;
}
