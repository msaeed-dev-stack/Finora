// =====================================================================
//  ReportExporter.h
//  Reports and files made from the signed-in person's own records:
//    - a CSV file of every transaction    (Finora Plus and larger, Phase 4C)
//    - a PDF summary of one month          (Finora Pro and larger, Phase 4C)
//    - the report of a month or a year, on screen and as a CSV file
//                                          (Finora Plus and larger, Milestone 6)
//  Each function checks the plan itself, before it reads any record, so
//  no screen can make one of these reports or files without the right plan.
// =====================================================================
#ifndef FINORA_REPORTEXPORTER_H
#define FINORA_REPORTEXPORTER_H

#include <QString>

#include <vector>

#include "core/Budget.h"
#include "core/Reports.h"
#include "core/Transaction.h"
#include "database/BudgetStore.h"
#include "database/DatabaseManager.h"

class ReportExporter
{
public:
    ReportExporter(DatabaseManager &sharedDatabase);

    // Writes every transaction of the signed-in person to `path` as CSV.
    // `rowCount` tells how many transactions were written.
    bool exportTransactionsCsv(const QString &path, int &rowCount);

    // Writes a one-page summary of `month` ("2026-10") to `path` as a PDF
    // file, made with Qt's own QPdfWriter. `ownerName` is printed on it.
    bool exportMonthlyPdf(const QString &month, const QString &ownerName, const QString &path);

    // The page the PDF is printed from, as simple HTML. Public for the tests.
    QString monthlySummaryHtml(const QString &month, const QString &ownerName,
                               const std::vector<Transaction> &transactions,
                               const std::vector<Budget> &budgets) const;

    // ---- Reports (Milestone 6, Finora Plus) ----

    // Builds the report that `request` asks for (a month or a year, one
    // currency) from the signed-in person's own transactions. The plan is
    // checked first; transactions are only read, never changed.
    bool buildReport(const ReportRequest &request, PeriodReport &report);

    // Writes the same report to `path` as a CSV file.
    bool exportReportCsv(const ReportRequest &request, const QString &path);

    QString lastError() const;

    // True when the last buildReport() or exportReportCsv() was refused
    // because the plan does not include reports, so the screen can explain
    // the plans instead.
    bool planBlocked() const;

private:
    bool requirePlan(Feature feature);
    bool fail(const QString &message);

    DatabaseManager &database;   // created in main(); this class only uses it
    BudgetStore budgetStore;     // the budgets of the same database (composition)
    QString errorMessage;
    bool blockedByPlan = false;  // see planBlocked()
};

#endif // FINORA_REPORTEXPORTER_H
