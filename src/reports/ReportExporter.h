// =====================================================================
//  ReportExporter.h
//  Files made from the signed-in person's own records (Phase 4C):
//    - a CSV file of every transaction    (Finora Plus and larger)
//    - a PDF summary of one month          (Finora Pro and larger)
//  Each function checks the plan itself, before it reads any record, so
//  no screen can make one of these files without the right plan.
// =====================================================================
#ifndef FINORA_REPORTEXPORTER_H
#define FINORA_REPORTEXPORTER_H

#include <QString>

#include <vector>

#include "core/Budget.h"
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

    QString lastError() const;

private:
    bool fail(const QString &message);

    DatabaseManager &database;   // created in main(); this class only uses it
    BudgetStore budgetStore;     // the budgets of the same database (composition)
    QString errorMessage;
};

#endif // FINORA_REPORTEXPORTER_H
