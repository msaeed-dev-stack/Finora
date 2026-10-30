// =====================================================================
//  Totals.cpp
// =====================================================================
#include "Totals.h"

#include <algorithm>
#include <climits>

#include "TransactionFilter.h"   // toLowerText

bool addChecked(long long &total, long long amount)
{
    if (total < 0 || amount < 0) {
        return false;   // totals in Finora are never negative
    }
    // LLONG_MAX - total is the room that is left. If the amount does not
    // fit into that room, total + amount would overflow.
    if (amount > LLONG_MAX - total) {
        return false;
    }
    total = total + amount;
    return true;
}

// ---- Months ----------------------------------------------------------

// Reads the digits text[start] .. text[start + length - 1] as a number.
// Returns -1 if one of them is not a digit.
int readDigits(const std::string &text, int start, int length)
{
    int number = 0;
    for (int i = start; i < start + length; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return -1;
        }
        number = number * 10 + (text[i] - '0');
    }
    return number;
}

bool isValidMonth(const std::string &month)
{
    if (month.length() != 7 || month[4] != '-') {
        return false;
    }
    int year = readDigits(month, 0, 4);
    int monthNumber = readDigits(month, 5, 2);
    return year >= 1900 && year <= 2100 && monthNumber >= 1 && monthNumber <= 12;
}

std::string monthOfDate(const std::string &isoDate)
{
    if (!isValidIsoDate(isoDate)) {
        return "";
    }
    return isoDate.substr(0, 7);   // "2026-10-28" -> "2026-10"
}

std::string shiftMonth(const std::string &month, int count)
{
    if (!isValidMonth(month)) {
        return "";
    }
    int year = readDigits(month, 0, 4);
    int monthNumber = readDigits(month, 5, 2);

    // Count months from the start of year 0: January 2026 is month
    // 2026 * 12 + 0. Adding `count` and dividing by 12 again handles
    // every change of year, also backwards.
    int monthIndex = year * 12 + (monthNumber - 1) + count;
    int newYear = monthIndex / 12;
    int newMonth = monthIndex % 12 + 1;
    if (newYear < 1900 || newYear > 2100) {
        return "";
    }

    std::string monthText = std::to_string(newMonth);
    if (newMonth < 10) {
        monthText = "0" + monthText;
    }
    return std::to_string(newYear) + "-" + monthText;
}

// ---- Totals for each currency ----------------------------------------

std::vector<CurrencySummary> summarizeByCurrency(const std::vector<Transaction> &transactions,
                                                 const std::string &month)
{
    // One empty summary per supported currency.
    std::vector<CurrencySummary> summaries;
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        CurrencySummary summary;
        summary.currencyCode = SUPPORTED_CURRENCIES[i].code;
        summaries.push_back(summary);
    }

    int count = static_cast<int>(transactions.size());
    for (int i = 0; i < count; i++) {
        const Transaction &transaction = transactions[i];

        // Find the summary of this transaction's own currency.
        int index = -1;
        for (int c = 0; c < SUPPORTED_CURRENCY_COUNT; c++) {
            if (summaries[c].currencyCode == transaction.currencyCode) {
                index = c;
                break;
            }
        }
        if (index < 0) {
            continue;   // an unknown currency is never added to another one
        }

        CurrencySummary &summary = summaries[index];   // a reference: changes go into the vector
        summary.transactionCount++;
        bool inMonth = (monthOfDate(transaction.date) == month);

        bool added = true;
        if (transaction.type == TYPE_INCOME) {
            added = addChecked(summary.totalIncomeMinor, transaction.amountMinor);
            if (added && inMonth) {
                added = addChecked(summary.monthIncomeMinor, transaction.amountMinor);
            }
        } else {
            added = addChecked(summary.totalExpenseMinor, transaction.amountMinor);
            if (added && inMonth) {
                added = addChecked(summary.monthExpenseMinor, transaction.amountMinor);
            }
        }
        if (!added) {
            summary.overflow = true;
        }
    }
    return summaries;
}

// Both totals are 0 or more, so subtracting one from the other can never overflow.
long long balanceOf(const CurrencySummary &summary)
{
    return summary.totalIncomeMinor - summary.totalExpenseMinor;
}

long long monthNetOf(const CurrencySummary &summary)
{
    return summary.monthIncomeMinor - summary.monthExpenseMinor;
}

// ---- Money flow per month ---------------------------------------------

std::vector<MonthFlow> monthlyFlow(const std::vector<Transaction> &transactions,
                                   const std::string &currencyCode,
                                   const std::string &lastMonth, int monthCount)
{
    std::vector<MonthFlow> flows;
    if (!isValidMonth(lastMonth) || monthCount < 1) {
        return flows;
    }

    // The oldest month first: for 6 months ending 2026-10 that is 2026-05.
    for (int back = monthCount - 1; back >= 0; back--) {
        MonthFlow flow;
        flow.month = shiftMonth(lastMonth, -back);
        flows.push_back(flow);
    }

    int count = static_cast<int>(transactions.size());
    int flowCount = static_cast<int>(flows.size());
    for (int i = 0; i < count; i++) {
        const Transaction &transaction = transactions[i];
        if (transaction.currencyCode != currencyCode) {
            continue;
        }
        std::string month = monthOfDate(transaction.date);

        for (int m = 0; m < flowCount; m++) {
            if (flows[m].month != month) {
                continue;
            }
            bool added = true;
            if (transaction.type == TYPE_INCOME) {
                added = addChecked(flows[m].incomeMinor, transaction.amountMinor);
            } else {
                added = addChecked(flows[m].expenseMinor, transaction.amountMinor);
            }
            if (!added) {
                flows[m].overflow = true;
            }
            break;   // a transaction belongs to exactly one month
        }
    }
    return flows;
}

// ---- Spending per category ---------------------------------------------

// Used by std::sort below: true if `a` should come before `b`.
bool largerAmountFirst(const CategoryTotal &a, const CategoryTotal &b)
{
    if (a.amountMinor != b.amountMinor) {
        return a.amountMinor > b.amountMinor;
    }
    return a.category < b.category;   // equal amounts: A to Z
}

std::vector<CategoryTotal> spendingByCategory(const std::vector<Transaction> &transactions,
                                              const std::string &currencyCode,
                                              const std::string &month, bool &overflow)
{
    std::vector<CategoryTotal> totals;
    overflow = false;

    int count = static_cast<int>(transactions.size());
    for (int i = 0; i < count; i++) {
        const Transaction &transaction = transactions[i];
        if (transaction.type != TYPE_EXPENSE || transaction.currencyCode != currencyCode
            || monthOfDate(transaction.date) != month) {
            continue;
        }

        // "Food" and "food" are the same category.
        int index = -1;
        int listed = static_cast<int>(totals.size());
        for (int t = 0; t < listed; t++) {
            if (toLowerText(totals[t].category) == toLowerText(transaction.category)) {
                index = t;
                break;
            }
        }
        if (index < 0) {
            CategoryTotal newTotal;
            newTotal.category = transaction.category;
            totals.push_back(newTotal);
            index = listed;
        }

        if (!addChecked(totals[index].amountMinor, transaction.amountMinor)) {
            overflow = true;
        }
    }

    std::sort(totals.begin(), totals.end(), largerAmountFirst);
    return totals;
}

// ---- Chart helpers ---------------------------------------------------

long long chartScaleMaximum(long long largestMinor)
{
    const int STEP_COUNT = 4;          // the chart shows 4 equal steps
    const long long SMALLEST_STEP = 100;   // never smaller than 1.00

    // The smallest step that still reaches the largest value (rounded up).
    long long neededStep = (largestMinor + STEP_COUNT - 1) / STEP_COUNT;
    if (neededStep < SMALLEST_STEP) {
        neededStep = SMALLEST_STEP;
    }
    if (neededStep > 1000000000000000LL) {
        return neededStep * STEP_COUNT;   // far beyond real use: no rounding
    }

    // `power` becomes the largest power of ten that is not above neededStep.
    long long power = 1;
    while (neededStep / power >= 10) {
        power = power * 10;
    }

    // Round steps: 1, 1.5, 2, 2.5, 3, 4, 5, 6, 8 or 10 times `power`
    // (written in tenths, so 15 means 1.5).
    const int ROUND_STEP_COUNT = 10;
    const int ROUND_STEPS_IN_TENTHS[ROUND_STEP_COUNT] = {10, 15, 20, 25, 30, 40, 50, 60, 80, 100};
    for (int i = 0; i < ROUND_STEP_COUNT; i++) {
        long long step = ROUND_STEPS_IN_TENTHS[i] * (power / 10);
        if (step >= neededStep) {
            return step * STEP_COUNT;
        }
    }
    return power * 10 * STEP_COUNT;   // not reached: 100 tenths always fits
}

// "12k" or "1.2k": `value` divided by `unit`, with one decimal only when
// it is small and the decimal is not zero.
std::string compactWithUnit(long long value, long long unit, const std::string &suffix)
{
    long long tenths = value * 10 / unit;   // 1,234 / 1,000 -> 12 tenths
    if (tenths >= 100 || tenths % 10 == 0) {
        return std::to_string(tenths / 10) + suffix;
    }
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) + suffix;
}

std::string formatCompactAmount(long long amountMinor)
{
    std::string sign = "";
    if (amountMinor < 0) {
        sign = "-";
        amountMinor = -amountMinor;
    }
    long long whole = amountMinor / 100;   // the chart labels show whole units only

    if (whole < 1000) {
        return sign + std::to_string(whole);
    }
    if (whole < 1000000) {
        return sign + compactWithUnit(whole, 1000, "k");
    }
    if (whole < 1000000000) {
        return sign + compactWithUnit(whole, 1000000, "M");
    }
    return sign + compactWithUnit(whole, 1000000000, "B");
}
