// =====================================================================
//  GoalStore.h
//  Saves, loads, changes and deletes savings goals and the money put
//  into them (Milestone 5).
// =====================================================================
#ifndef FINORA_GOALSTORE_H
#define FINORA_GOALSTORE_H

#include <QString>

#include <vector>

#include "DatabaseManager.h"
#include "core/Goal.h"

// The savings-goal part of the database. Like BudgetStore, it uses the
// connection that DatabaseManager opened (composition) and never opens
// its own.
//
// Savings goals are a Finora Plus feature. Every change (a new goal, an
// edit, money put in or taken out) checks the plan first, here, below
// every screen. Reading and deleting work on every plan, so after a
// downgrade the person still sees every goal and nothing is deleted.
// Every goal belongs to the signed-in person: the owner always comes
// from the Session, never from the screen.
class GoalStore
{
public:
    GoalStore(DatabaseManager &sharedDatabase);

    // Saves a valid goal. On success, goal.id is set to the new goal's
    // number. A second goal with the same name is refused.
    bool addGoal(Goal &goal);

    // Loads every goal of the signed-in person, sorted by name, with
    // savedMinor set to the sum of the goal's contributions.
    bool loadGoals(std::vector<Goal> &results);

    // Saves a new name, target and target date for an existing goal. The
    // currency can only change while nothing has been saved, because the
    // saved amount is in the old currency.
    bool updateGoal(const Goal &goal);

    // Deletes a goal and its contributions. Allowed on every plan.
    bool deleteGoal(int id);

    // Puts money into a goal (positive amount) or takes money out
    // (negative amount). Reading the saved amount, checking the rules and
    // saving happen inside one database transaction, so the saved amount
    // can never become negative or too large. On success, contribution.id
    // is set.
    bool addContribution(GoalContribution &contribution);

    // The contributions of one goal, newest first.
    bool loadContributions(int goalId, std::vector<GoalContribution> &results);

    QString lastError() const;

    // True when the last change was refused because the plan does not
    // include savings goals, so the screen can explain the plans instead.
    bool planBlocked() const;

private:
    bool checkPlan();
    bool checkNoDuplicate(const Goal &goal, int ownerId);
    bool savedAmount(int goalId, int ownerId, long long &savedMinor);
    bool fail(const QString &message);

    DatabaseManager &database;   // created in main(); this store only uses it
    QString errorMessage;
    bool blockedByPlan = false;  // see planBlocked()
};

#endif // FINORA_GOALSTORE_H
