// =====================================================================
//  Budget.h
//  A monthly spending limit for one category, and the rules that say
//  how much of it has been used.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_BUDGET_H
#define FINORA_BUDGET_H

#include <string>
#include <vector>

#include "Transaction.h"

// One budget: "Food, PKR 20,000 per month". The limit repeats every
// month; what is spent is counted again for each month.
struct Budget {
    int id = 0;                  // 0 until the database gives the budget its number
    std::string category;        // an expense category, for example "Food"
    std::string currencyCode;    // "PKR", "USD" or "EUR"
    long long limitMinor = 0;    // monthly limit in minor units: 2000000 means 20,000.00
};

// How much of a budget is used. An enum gives each state a name, so the
// code says BUDGET_WARNING instead of a number such as 1.
enum BudgetStatus {
    BUDGET_ON_TRACK,   // less than 80% used
    BUDGET_WARNING,    // 80% or more used, but some money is left
    BUDGET_REACHED,    // exactly 100%: nothing left, but not over
    BUDGET_OVER        // more than the limit has been spent
};

// A budget together with what has been spent on it in one month.
struct BudgetProgress {
    Budget budget;
    std::string month;              // "2026-10"
    long long spentMinor = 0;       // expenses of this category and currency in the month
    long long remainingMinor = 0;   // limit minus spent; negative when over budget
    int percentUsed = 0;            // whole percent, rounded down, at most 999
    BudgetStatus status = BUDGET_ON_TRACK;
    bool overflow = false;          // the spent total was too large to add up
};

// Checks every field. Returns an empty string if the budget is valid,
// otherwise a message about the first problem found.
std::string validateBudget(const Budget &budget);

// The status for an amount spent against a limit (limit above 0).
// The 80% rule uses whole numbers only: 5 x spent >= 4 x limit means
// spent / limit >= 0.8, without dividing and without any rounding.
BudgetStatus budgetStatusFor(long long spentMinor, long long limitMinor);

// spent / limit as a whole percent, rounded down: 7999 of 10000 is 79.
// Returns 999 for 999% or more, so a huge amount never overflows.
int percentUsed(long long spentMinor, long long limitMinor);

// Words for each status, shown next to the colour so that the status
// never depends on colour alone: "On track", "Nearing limit",
// "Limit reached" or "Over budget".
std::string budgetStatusName(BudgetStatus status);

// Adds up the expenses of the budget's category and currency in `month`
// ("YYYY-MM"). The category is compared without caring about upper or
// lower case, so "food" counts towards a "Food" budget.
BudgetProgress budgetProgressFor(const Budget &budget,
                                 const std::vector<Transaction> &transactions,
                                 const std::string &month);

// budgetProgressFor() for every budget, in the same order.
std::vector<BudgetProgress> allBudgetProgress(const std::vector<Budget> &budgets,
                                              const std::vector<Transaction> &transactions,
                                              const std::string &month);

#endif // FINORA_BUDGET_H
