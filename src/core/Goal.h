// =====================================================================
//  Goal.h
//  Savings goals (Milestone 5): what a goal is, the rules for a goal and
//  for money put in or taken out, and how far a goal has come.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_GOAL_H
#define FINORA_GOAL_H

#include <string>

// One savings goal: "New laptop, PKR 250,000 by 31 March 2027".
struct Goal {
    int id = 0;                  // 0 until the database gives the goal its number
    std::string name;            // for example "New laptop"
    std::string currencyCode;    // "PKR", "USD" or "EUR"
    long long targetMinor = 0;   // in minor units: 25000000 means 250,000.00
    std::string targetDate;      // "2027-03-31", or "" when there is no date
    long long savedMinor = 0;    // the sum of all its contributions (filled in when loaded)
};

// Money put into a goal (a positive amount) or taken out of it (negative).
struct GoalContribution {
    int id = 0;
    int goalId = 0;
    long long amountMinor = 0;   // +50000 means 500.00 added; -20000 means 200.00 taken out
    std::string date;            // "2026-10-31"
    std::string note;            // optional, at most 100 characters
};

// How far a goal has come on a given day.
struct GoalProgress {
    Goal goal;
    int percent = 0;                  // whole percent, rounded down, at most 100
    long long remainingMinor = 0;     // what is still missing; 0 once reached
    bool reached = false;             // saved >= target
    bool overdue = false;             // the target date has passed and the goal is not reached
    int monthsLeft = -1;              // months until the target date, counting this one; -1 without a date
    long long monthlyNeededMinor = 0; // to save each month to reach the goal in time; 0 if not needed
};

const int GOAL_NAME_MAX_LENGTH = 40;
const int GOAL_NOTE_MAX_LENGTH = 100;

// Checks every field of a goal. Returns "" if it is valid, otherwise a
// message about the first problem.
std::string validateGoal(const Goal &goal);

// Checks one contribution for a goal that has `savedMinor` saved so far:
// the amount is not zero and within the limits, the date is valid, the
// note is not too long, a withdrawal is never more than what is saved,
// and the new total still fits (checked addition). Returns "" if valid.
std::string validateContribution(const GoalContribution &contribution, long long savedMinor);

// Whole months from the month of `today` to the month of `targetDate`,
// counting the current month: from 2026-10-31 to 2027-03-31 gives 6
// (October to March); a target in the current month gives 1. Both
// dates are "YYYY-MM-DD".
int monthsUntil(const std::string &today, const std::string &targetDate);

// The goal's progress on `today` ("YYYY-MM-DD"). The amount to save each
// month is rounded UP to the next minor unit, so that saving it every
// month really reaches the goal.
GoalProgress goalProgressFor(const Goal &goal, const std::string &today);

#endif // FINORA_GOAL_H
