// =====================================================================
//  Goal.cpp
// =====================================================================
#include "Goal.h"

#include "Budget.h"        // percentUsed
#include "Money.h"         // trimSpaces
#include "Totals.h"        // addChecked
#include "Transaction.h"   // isSupportedCurrency, isValidIsoDate, MAX_AMOUNT_MINOR

std::string validateGoal(const Goal &goal)
{
    std::string name = trimSpaces(goal.name);
    if (name.empty()) {
        return "Please give the goal a name.";
    }
    if (static_cast<int>(name.length()) > GOAL_NAME_MAX_LENGTH) {
        return "The name can have at most " + std::to_string(GOAL_NAME_MAX_LENGTH) + " characters.";
    }
    if (!isSupportedCurrency(goal.currencyCode)) {
        return "Please choose PKR, USD or EUR.";
    }
    if (goal.targetMinor <= 0) {
        return "The target must be more than zero.";
    }
    if (goal.targetMinor > MAX_AMOUNT_MINOR) {
        return "The target is too large.";
    }
    if (!goal.targetDate.empty() && !isValidIsoDate(goal.targetDate)) {
        return "The target date is not a valid date.";
    }
    return "";
}

std::string validateContribution(const GoalContribution &contribution, long long savedMinor)
{
    if (contribution.amountMinor == 0) {
        return "The amount must be more than zero.";
    }
    if (contribution.amountMinor > MAX_AMOUNT_MINOR || contribution.amountMinor < -MAX_AMOUNT_MINOR) {
        return "The amount is too large.";
    }
    if (!isValidIsoDate(contribution.date)) {
        return "Please choose a valid date.";
    }
    if (static_cast<int>(contribution.note.length()) > GOAL_NOTE_MAX_LENGTH) {
        return "The note can have at most " + std::to_string(GOAL_NOTE_MAX_LENGTH) + " characters.";
    }
    if (contribution.amountMinor < 0) {
        // Taking out more than is saved would make the saved amount negative.
        if (-contribution.amountMinor > savedMinor) {
            return "You can take out at most what is saved.";
        }
        return "";   // the total only gets smaller, so it cannot overflow
    }
    // Putting money in: the new total must still fit (checked addition,
    // which accepts only amounts of zero or more).
    long long newTotal = savedMinor;
    if (!addChecked(newTotal, contribution.amountMinor) || newTotal > MAX_AMOUNT_MINOR) {
        return "The saved amount would become too large.";
    }
    return "";
}

int monthsUntil(const std::string &today, const std::string &targetDate)
{
    // "2026-10-31": the year is characters 0-3, the month 5-6.
    int todayYear = std::stoi(today.substr(0, 4));
    int todayMonth = std::stoi(today.substr(5, 2));
    int targetYear = std::stoi(targetDate.substr(0, 4));
    int targetMonth = std::stoi(targetDate.substr(5, 2));
    // Count months from year 0, then subtract; +1 counts the current month too.
    return (targetYear * 12 + targetMonth) - (todayYear * 12 + todayMonth) + 1;
}

GoalProgress goalProgressFor(const Goal &goal, const std::string &today)
{
    GoalProgress progress;
    progress.goal = goal;

    progress.reached = goal.savedMinor >= goal.targetMinor;
    progress.percent = percentUsed(goal.savedMinor, goal.targetMinor);
    if (progress.percent > 100) {
        progress.percent = 100;
    }
    if (!progress.reached) {
        progress.remainingMinor = goal.targetMinor - goal.savedMinor;
    }

    if (goal.targetDate.empty() || progress.reached) {
        return progress;
    }
    // ISO dates compare like text, so "<" means "earlier".
    if (goal.targetDate < today) {
        progress.overdue = true;
        return progress;
    }
    progress.monthsLeft = monthsUntil(today, goal.targetDate);
    // Ceiling division: (a + b - 1) / b rounds a / b UP to a whole number,
    // so that saving this amount each month really reaches the goal.
    progress.monthlyNeededMinor = (progress.remainingMinor + progress.monthsLeft - 1) / progress.monthsLeft;
    return progress;
}
