// =====================================================================
//  Budget.cpp
// =====================================================================
#include "Budget.h"

#include "Totals.h"              // addChecked, monthOfDate
#include "TransactionFilter.h"   // toLowerText

std::string validateBudget(const Budget &budget)
{
    if (isBlank(budget.category)) {
        return "Please choose a category.";
    }
    if (budget.category.length() > 40) {
        return "A category name can be at most 40 characters long.";
    }
    if (!isSupportedCurrency(budget.currencyCode)) {
        return "Please choose PKR, USD or EUR.";
    }
    if (budget.limitMinor <= 0) {
        return "The monthly limit must be greater than zero.";
    }
    if (budget.limitMinor > MAX_AMOUNT_MINOR) {
        return "The monthly limit is too large.";
    }
    return "";
}

BudgetStatus budgetStatusFor(long long spentMinor, long long limitMinor)
{
    if (spentMinor > limitMinor) {
        return BUDGET_OVER;
    }
    if (spentMinor == limitMinor) {
        return BUDGET_REACHED;
    }
    // From here spent < limit <= MAX_AMOUNT_MINOR (about 10^12), so
    // 5 x spent stays far below the long long limit (about 9.2 x 10^18).
    if (5 * spentMinor >= 4 * limitMinor) {
        return BUDGET_WARNING;
    }
    return BUDGET_ON_TRACK;
}

int percentUsed(long long spentMinor, long long limitMinor)
{
    if (limitMinor <= 0 || spentMinor <= 0) {
        return 0;
    }
    // Check the size first: 10 x limit is at most about 10^13, so this
    // multiplication is safe, and below it spent x 100 is safe too.
    if (spentMinor >= 10 * limitMinor) {
        return 999;
    }
    return static_cast<int>(spentMinor * 100 / limitMinor);   // integer division rounds down
}

std::string budgetStatusName(BudgetStatus status)
{
    switch (status) {
    case BUDGET_ON_TRACK:
        return "On track";
    case BUDGET_WARNING:
        return "Nearing limit";
    case BUDGET_REACHED:
        return "Limit reached";
    case BUDGET_OVER:
        return "Over budget";
    }
    return "";
}

BudgetProgress budgetProgressFor(const Budget &budget,
                                 const std::vector<Transaction> &transactions,
                                 const std::string &month)
{
    BudgetProgress progress;
    progress.budget = budget;
    progress.month = month;

    std::string budgetCategory = toLowerText(budget.category);
    int count = static_cast<int>(transactions.size());
    for (int i = 0; i < count; i++) {
        const Transaction &transaction = transactions[i];
        if (transaction.type != TYPE_EXPENSE) {
            continue;   // only spending counts towards a budget
        }
        if (transaction.currencyCode != budget.currencyCode) {
            continue;   // a USD expense never uses up a PKR budget
        }
        if (toLowerText(transaction.category) != budgetCategory) {
            continue;
        }
        if (monthOfDate(transaction.date) != month) {
            continue;
        }
        if (!addChecked(progress.spentMinor, transaction.amountMinor)) {
            progress.overflow = true;
            break;
        }
    }

    // limit <= MAX_AMOUNT_MINOR and spent >= 0, so this cannot overflow.
    progress.remainingMinor = budget.limitMinor - progress.spentMinor;
    progress.percentUsed = percentUsed(progress.spentMinor, budget.limitMinor);
    progress.status = budgetStatusFor(progress.spentMinor, budget.limitMinor);

    // A total too large for a long long is certainly far above the limit.
    if (progress.overflow) {
        progress.percentUsed = 999;
        progress.status = BUDGET_OVER;
    }
    return progress;
}

std::vector<BudgetProgress> allBudgetProgress(const std::vector<Budget> &budgets,
                                              const std::vector<Transaction> &transactions,
                                              const std::string &month)
{
    std::vector<BudgetProgress> results;
    int count = static_cast<int>(budgets.size());
    for (int i = 0; i < count; i++) {
        results.push_back(budgetProgressFor(budgets[i], transactions, month));
    }
    return results;
}
