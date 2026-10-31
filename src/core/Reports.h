// =====================================================================
//  Reports.h
//  Reports (Milestone 6): the income, expenses and categories of ONE
//  currency in a month or a year, compared with the period before, and
//  the report as CSV text.
//  Plain C++ (no Qt), so the tests can check every rule directly.
//
//  The two rules of Totals.h hold here too: PKR, USD and EUR are never
//  added together, and every addition is checked.
// =====================================================================
#ifndef FINORA_REPORTS_H
#define FINORA_REPORTS_H

#include <string>
#include <vector>

#include "Totals.h"
#include "Transaction.h"

enum ReportPeriodKind {
    REPORT_MONTH,   // one month, written "2026-10"
    REPORT_YEAR     // one year, written "2026"
};

// What the person asks for.
struct ReportRequest {
    ReportPeriodKind kind = REPORT_MONTH;
    std::string period;          // "2026-10" for a month, "2026" for a year
    std::string currencyCode;    // "PKR", "USD" or "EUR": one currency at a time
    std::string type;            // which categories are listed: "" for both, TYPE_INCOME or TYPE_EXPENSE
};

// Income and expenses of one currency in one period.
struct PeriodTotals {
    long long incomeMinor = 0;
    long long expenseMinor = 0;
    int incomeCount = 0;         // how many income transactions
    int expenseCount = 0;        // how many expense transactions
    bool overflow = false;       // true if a total became too large to add up
};

// One category of one type in the period.
struct ReportCategory {
    std::string type;            // TYPE_INCOME or TYPE_EXPENSE
    std::string category;        // as first written; "Food" and "food" are one category
    long long amountMinor = 0;
    int count = 0;               // how many transactions
};

// Everything the Reports page shows.
struct PeriodReport {
    ReportRequest request;
    std::string firstDay;                     // "2026-10-01"
    std::string lastDay;                      // "2026-10-31"
    std::string previousPeriod;               // "2026-09" or "2025"; "" if there is none
    PeriodTotals totals;                      // the chosen period
    PeriodTotals previousTotals;              // the period before it, for the comparison
    std::vector<ReportCategory> categories;   // income first, then expenses; the largest first
    std::vector<MonthFlow> months;            // a month: the 6 months up to it; a year: its 12 months
    bool overflow = false;                    // true if any total became too large
};

// The first and last day of a period: the month "2026-02" gives
// "2026-02-01" and "2026-02-28"; the year "2026" gives "2026-01-01" and
// "2026-12-31". Returns false if the period is not a valid month or year
// between 1900 and 2100.
bool periodBounds(ReportPeriodKind kind, const std::string &period, std::string &firstDay, std::string &lastDay);

// The period before: "2026-01" gives "2025-12", "2026" gives "2025".
// Returns "" if there is none (before 1900) or the period is not valid.
std::string previousPeriodOf(ReportPeriodKind kind, const std::string &period);

// Income and expenses of one currency from firstDay to lastDay, both days
// included (dates are "YYYY-MM-DD", so they compare like text).
PeriodTotals totalsBetween(const std::vector<Transaction> &transactions, const std::string &currencyCode,
                           const std::string &firstDay, const std::string &lastDay);

// Income minus expenses; negative when more was spent than earned.
long long netOf(const PeriodTotals &totals);

// The categories of one currency between the two days, of one type (""
// for both). Sets `overflow` to true if a total became too large.
std::vector<ReportCategory> categoriesBetween(const std::vector<Transaction> &transactions,
                                              const std::string &currencyCode, const std::string &type,
                                              const std::string &firstDay, const std::string &lastDay,
                                              bool &overflow);

// Builds the whole report. Returns false, with an empty report, if the
// period, the currency or the type is not valid. Transactions are only
// read, never changed.
bool buildPeriodReport(const std::vector<Transaction> &transactions, const ReportRequest &request,
                       PeriodReport &report);

// The report as CSV text: what it covers, the totals next to the period
// before, the categories and the months. Category names go through
// csvField(), so commas, quotes, line breaks and formulas are safe.
// Every line ends with \r\n.
std::string reportCsv(const PeriodReport &report);

#endif // FINORA_REPORTS_H
