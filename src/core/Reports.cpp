// =====================================================================
//  Reports.cpp
// =====================================================================
#include "Reports.h"

#include <algorithm>

#include "CsvExport.h"           // csvField, plainAmount
#include "TransactionFilter.h"   // toLowerText

// True for a year written as 4 digits between 1900 and 2100.
bool isValidReportYear(const std::string &text)
{
    if (text.length() != 4) {
        return false;
    }
    for (int i = 0; i < 4; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
    }
    int year = std::stoi(text);
    return year >= 1900 && year <= 2100;
}

bool periodBounds(ReportPeriodKind kind, const std::string &period, std::string &firstDay, std::string &lastDay)
{
    firstDay = "";
    lastDay = "";
    if (kind == REPORT_YEAR) {
        if (!isValidReportYear(period)) {
            return false;
        }
        firstDay = period + "-01-01";
        lastDay = period + "-12-31";
        return true;
    }
    if (!isValidMonth(period)) {
        return false;
    }
    // The last day depends on the month, and on leap years for February.
    int year = std::stoi(period.substr(0, 4));
    int month = std::stoi(period.substr(5, 2));
    firstDay = period + "-01";
    lastDay = period + "-" + std::to_string(daysInMonth(year, month));
    return true;
}

std::string previousPeriodOf(ReportPeriodKind kind, const std::string &period)
{
    if (kind == REPORT_YEAR) {
        if (!isValidReportYear(period) || period == "1900") {
            return "";
        }
        return std::to_string(std::stoi(period) - 1);
    }
    return shiftMonth(period, -1);   // "" for 1900-01 and for a month that is not valid
}

PeriodTotals totalsBetween(const std::vector<Transaction> &transactions, const std::string &currencyCode,
                           const std::string &firstDay, const std::string &lastDay)
{
    PeriodTotals totals;
    int count = static_cast<int>(transactions.size());
    for (int i = 0; i < count; i++) {
        const Transaction &transaction = transactions[i];
        if (transaction.currencyCode != currencyCode || transaction.date < firstDay
            || transaction.date > lastDay) {
            continue;   // another currency, or outside the period
        }
        if (transaction.type == TYPE_INCOME) {
            totals.incomeCount++;
            if (!addChecked(totals.incomeMinor, transaction.amountMinor)) {
                totals.overflow = true;
            }
        } else if (transaction.type == TYPE_EXPENSE) {
            totals.expenseCount++;
            if (!addChecked(totals.expenseMinor, transaction.amountMinor)) {
                totals.overflow = true;
            }
        }
    }
    return totals;
}

long long netOf(const PeriodTotals &totals)
{
    // Both totals are 0 or more, so the subtraction cannot overflow.
    return totals.incomeMinor - totals.expenseMinor;
}

// Used by std::sort: income before expenses, then the larger amount, then A to Z.
bool reportCategoryOrder(const ReportCategory &a, const ReportCategory &b)
{
    if (a.type != b.type) {
        return a.type == TYPE_INCOME;
    }
    if (a.amountMinor != b.amountMinor) {
        return a.amountMinor > b.amountMinor;
    }
    return a.category < b.category;
}

std::vector<ReportCategory> categoriesBetween(const std::vector<Transaction> &transactions,
                                              const std::string &currencyCode, const std::string &type,
                                              const std::string &firstDay, const std::string &lastDay,
                                              bool &overflow)
{
    std::vector<ReportCategory> lines;
    overflow = false;
    int count = static_cast<int>(transactions.size());
    for (int i = 0; i < count; i++) {
        const Transaction &transaction = transactions[i];
        if (transaction.currencyCode != currencyCode || transaction.date < firstDay
            || transaction.date > lastDay || (!type.empty() && transaction.type != type)) {
            continue;
        }
        // "Food" and "food" of the same type are one category.
        int index = -1;
        int listed = static_cast<int>(lines.size());
        for (int l = 0; l < listed; l++) {
            if (lines[l].type == transaction.type
                && toLowerText(lines[l].category) == toLowerText(transaction.category)) {
                index = l;
                break;
            }
        }
        if (index < 0) {
            ReportCategory line;
            line.type = transaction.type;
            line.category = transaction.category;
            lines.push_back(line);
            index = listed;
        }
        lines[index].count++;
        if (!addChecked(lines[index].amountMinor, transaction.amountMinor)) {
            overflow = true;
        }
    }
    std::sort(lines.begin(), lines.end(), reportCategoryOrder);
    return lines;
}

bool buildPeriodReport(const std::vector<Transaction> &transactions, const ReportRequest &request,
                       PeriodReport &report)
{
    report = PeriodReport();
    bool typeValid = request.type.empty() || request.type == TYPE_INCOME || request.type == TYPE_EXPENSE;
    std::string firstDay;
    std::string lastDay;
    if (!isSupportedCurrency(request.currencyCode) || !typeValid
        || !periodBounds(request.kind, request.period, firstDay, lastDay)) {
        return false;
    }

    report.request = request;
    report.firstDay = firstDay;
    report.lastDay = lastDay;
    report.totals = totalsBetween(transactions, request.currencyCode, firstDay, lastDay);

    // The same currency in the period before, for the comparison.
    report.previousPeriod = previousPeriodOf(request.kind, request.period);
    std::string previousFirst;
    std::string previousLast;
    if (periodBounds(request.kind, report.previousPeriod, previousFirst, previousLast)) {
        report.previousTotals = totalsBetween(transactions, request.currencyCode, previousFirst, previousLast);
    }

    bool categoryOverflow = false;
    report.categories = categoriesBetween(transactions, request.currencyCode, request.type, firstDay, lastDay,
                                          categoryOverflow);

    // Month by month: the 6 months up to the chosen month, or the 12
    // months of the chosen year (monthlyFlow from Totals.h).
    if (request.kind == REPORT_YEAR) {
        report.months = monthlyFlow(transactions, request.currencyCode, request.period + "-12", 12);
    } else {
        report.months = monthlyFlow(transactions, request.currencyCode, request.period, 6);
    }

    report.overflow = report.totals.overflow || report.previousTotals.overflow || categoryOverflow;
    for (int i = 0; i < static_cast<int>(report.months.size()); i++) {
        if (report.months[i].overflow) {
            report.overflow = true;
        }
    }
    return true;
}

std::string reportCsv(const PeriodReport &report)
{
    const std::string end = "\r\n";
    std::string kindName = "Month";
    if (report.request.kind == REPORT_YEAR) {
        kindName = "Year";
    }

    std::string text = "Report,Period,Currency,From,To" + end;
    text += csvField(kindName) + "," + csvField(report.request.period) + "," + csvField(report.request.currencyCode)
            + "," + report.firstDay + "," + report.lastDay + end;
    text += end;

    // The totals next to the period before.
    text += "Summary,This period," + csvField("Previous period " + report.previousPeriod) + end;
    text += "Income," + plainAmount(report.totals.incomeMinor) + "," + plainAmount(report.previousTotals.incomeMinor)
            + end;
    text += "Expenses," + plainAmount(report.totals.expenseMinor) + ","
            + plainAmount(report.previousTotals.expenseMinor) + end;
    text += "Net," + plainAmount(netOf(report.totals)) + "," + plainAmount(netOf(report.previousTotals)) + end;
    text += "Transactions," + std::to_string(report.totals.incomeCount + report.totals.expenseCount) + ","
            + std::to_string(report.previousTotals.incomeCount + report.previousTotals.expenseCount) + end;
    text += end;

    // The categories. A category name comes from the person, so it always
    // goes through csvField().
    text += "Category,Type,Amount,Transactions" + end;
    for (int i = 0; i < static_cast<int>(report.categories.size()); i++) {
        const ReportCategory &line = report.categories[i];
        std::string typeName = "Expense";
        if (line.type == TYPE_INCOME) {
            typeName = "Income";
        }
        text += csvField(line.category) + "," + typeName + "," + plainAmount(line.amountMinor) + ","
                + std::to_string(line.count) + end;
    }
    text += end;

    // Month by month.
    text += "Month,Income,Expenses,Net" + end;
    for (int i = 0; i < static_cast<int>(report.months.size()); i++) {
        const MonthFlow &flow = report.months[i];
        text += flow.month + "," + plainAmount(flow.incomeMinor) + "," + plainAmount(flow.expenseMinor) + ","
                + plainAmount(flow.incomeMinor - flow.expenseMinor) + end;
    }
    return text;
}
