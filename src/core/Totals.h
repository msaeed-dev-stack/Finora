// =====================================================================
//  Totals.h
//  Adding up transactions for the dashboard: totals per currency, money
//  flow per month and spending per category.
//  Plain C++ (no Qt), so the tests can check every rule directly.
//
//  Two rules hold everywhere in this file:
//    1. Different currencies are NEVER added together: every total here
//       belongs to one currency. (Since Milestone 5 the Currency page can
//       show an ESTIMATE of all balances in one currency, with the rates
//       the person entered; see ExchangeRates.h. It never changes these totals.)
//    2. Every addition is checked. If a total would become too large for
//       a long long, the total is marked with overflow = true instead of
//       silently turning into a wrong number.
// =====================================================================
#ifndef FINORA_TOTALS_H
#define FINORA_TOTALS_H

#include <string>
#include <vector>

#include "Transaction.h"

// ---- Checked addition ------------------------------------------------

// Adds `amount` to `total`. Both must be 0 or more. If the result would
// be larger than the biggest long long, nothing is added and the
// function returns false; otherwise it returns true.
bool addChecked(long long &total, long long amount);

// ---- Months ----------------------------------------------------------
// A month is written "YYYY-MM", for example "2026-10".

// True if text is a month between 1900-01 and 2100-12 written as YYYY-MM.
bool isValidMonth(const std::string &month);

// The month of a date: "2026-10-28" gives "2026-10". Returns "" if the
// date is not valid.
std::string monthOfDate(const std::string &isoDate);

// Moves a month forwards (count > 0) or backwards (count < 0):
// ("2026-01", -1) gives "2025-12". Returns "" if the month is not valid
// or the result is outside 1900 to 2100.
std::string shiftMonth(const std::string &month, int count);

// ---- Totals for one currency ---------------------------------------

struct CurrencySummary {
    std::string currencyCode;          // "PKR", "USD" or "EUR"
    long long totalIncomeMinor = 0;    // every income ever recorded
    long long totalExpenseMinor = 0;   // every expense ever recorded
    long long monthIncomeMinor = 0;    // income in the chosen month
    long long monthExpenseMinor = 0;   // expenses in the chosen month
    int transactionCount = 0;          // transactions in this currency, all time
    bool overflow = false;             // true if a total became too large to add up
};

// One summary per supported currency, in the order of SUPPORTED_CURRENCIES.
// A transaction in any other currency is left out, never added to another one.
std::vector<CurrencySummary> summarizeByCurrency(const std::vector<Transaction> &transactions,
                                                 const std::string &month);

// Income minus expenses, all time. Negative when more was spent than earned.
long long balanceOf(const CurrencySummary &summary);

// Income minus expenses in the chosen month.
long long monthNetOf(const CurrencySummary &summary);

// ---- Money flow per month (for the chart) ----------------------------

struct MonthFlow {
    std::string month;          // "2026-10"
    long long incomeMinor = 0;
    long long expenseMinor = 0;
    bool overflow = false;
};

// Income and expenses of one currency for `monthCount` months that end
// with `lastMonth`, oldest month first. Months without transactions
// are included with zero, so the chart has no gaps.
std::vector<MonthFlow> monthlyFlow(const std::vector<Transaction> &transactions,
                                   const std::string &currencyCode,
                                   const std::string &lastMonth, int monthCount);

// ---- Spending per category -------------------------------------------

struct CategoryTotal {
    std::string category;
    long long amountMinor = 0;
};

// Expenses of one currency in one month, one entry per category, the
// largest first. Sets `overflow` to true if a total became too large.
std::vector<CategoryTotal> spendingByCategory(const std::vector<Transaction> &transactions,
                                              const std::string &currencyCode,
                                              const std::string &month, bool &overflow);

// ---- Chart helpers ---------------------------------------------------

// The value at the top of the chart's scale: a round number that is at
// least `largestMinor` and divides into four equal steps with round
// labels. 120,000.00 gives 120,000.00 (steps of 30,000); 87.00 gives 100.00.
long long chartScaleMaximum(long long largestMinor);

// A short label for a chart scale, from minor units: 12000000 (120,000.00)
// gives "120k", 125000 gives "1.2k", 4500000000 gives "45M".
std::string formatCompactAmount(long long amountMinor);

#endif // FINORA_TOTALS_H
