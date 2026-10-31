// =====================================================================
//  ReportExporter.cpp
// =====================================================================
#include "ReportExporter.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QSaveFile>
#include <QTextDocument>

#include "core/CsvExport.h"
#include "core/Money.h"
#include "core/Totals.h"

// Text from the person's records is escaped before it goes into HTML, so
// a category such as "<b>Food</b>" is printed as it is and can never
// change the page.
QString htmlText(const std::string &text)
{
    return QString::fromStdString(text).toHtmlEscaped();
}

// "2026-10" as "October 2026".
QString reportMonthTitle(const QString &month)
{
    QDate firstDay = QDate::fromString(month + "-01", "yyyy-MM-dd");
    if (!firstDay.isValid()) {
        return month;
    }
    return firstDay.toString("MMMM yyyy");
}

// The start of a report table, with its column titles.
QString tableStart(const QStringList &titles)
{
    QString html = "<table width=\"100%\" cellpadding=\"5\" cellspacing=\"0\" border=\"1\" "
                   "style=\"border-collapse: collapse; border-color: #D9DCE5;\">"
                   "<tr style=\"background-color: #EEEAFE;\">";
    for (int i = 0; i < titles.size(); i++) {
        QString align = "right";
        if (i == 0) {
            align = "left";
        }
        html += "<th align=\"" + align + "\">" + titles[i] + "</th>";
    }
    return html + "</tr>";
}

// One table row: the first cell on the left, the others on the right.
QString tableRow(const QStringList &cells)
{
    QString html = "<tr>";
    for (int i = 0; i < cells.size(); i++) {
        QString align = "right";
        if (i == 0) {
            align = "left";
        }
        html += "<td align=\"" + align + "\">" + cells[i] + "</td>";
    }
    return html + "</tr>";
}

ReportExporter::ReportExporter(DatabaseManager &sharedDatabase)
    : database(sharedDatabase), budgetStore(sharedDatabase)
{
}

bool ReportExporter::exportTransactionsCsv(const QString &path, int &rowCount)
{
    errorMessage.clear();
    rowCount = 0;

    // The plan first: Finora Plus or larger. Nothing is read before this.
    if (!database.requireFeature(FEATURE_CSV_EXPORT)) {
        return fail(database.lastError());
    }
    std::vector<Transaction> transactions;
    if (!database.loadTransactions(transactions)) {   // only the signed-in person's records
        return fail(database.lastError());
    }
    std::string text = transactionsCsv(transactions);

    // QSaveFile writes into a temporary file and replaces the real file
    // only when everything was written, so a failure never leaves half a
    // file behind.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return fail("Could not write " + QDir::toNativeSeparators(path) + ": " + file.errorString());
    }
    file.write("\xEF\xBB\xBF");   // the UTF-8 mark, so Excel reads letters such as é or € correctly
    file.write(text.c_str(), static_cast<qint64>(text.size()));
    if (!file.commit()) {
        return fail("Could not write " + QDir::toNativeSeparators(path) + ": " + file.errorString());
    }
    rowCount = static_cast<int>(transactions.size());
    database.recordAuditEvent("csv_exported", QString::number(rowCount) + " transactions");
    return true;
}

bool ReportExporter::exportMonthlyPdf(const QString &month, const QString &ownerName, const QString &path)
{
    errorMessage.clear();

    // The plan first: Finora Pro or larger. Nothing is read before this.
    if (!database.requireFeature(FEATURE_PDF_REPORTS)) {
        return fail(database.lastError());
    }
    if (!isValidMonth(month.toStdString())) {
        return fail("The month " + month + " is not valid.");
    }
    std::vector<Transaction> transactions;
    std::vector<Budget> budgets;
    if (!database.loadTransactions(transactions)) {
        return fail(database.lastError());
    }
    if (!budgetStore.loadBudgets(budgets)) {
        return fail(budgetStore.lastError());
    }

    // An older file of the same name is removed first (the save dialog has
    // already asked), so the check below can only see the new file.
    if (QFile::exists(path) && !QFile::remove(path)) {
        return fail("Could not replace " + QDir::toNativeSeparators(path) + ". Is it open in another program?");
    }

    QTextDocument document;
    document.setHtml(monthlySummaryHtml(month, ownerName, transactions, budgets));
    {
        // QPdfWriter is part of Qt itself (Qt GUI), so no extra library is needed.
        QPdfWriter writer(path);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageMargins(QMarginsF(18, 18, 18, 18), QPageLayout::Millimeter);
        writer.setTitle("Finora monthly summary, " + reportMonthTitle(month));
        writer.setCreator("Finora");
        document.print(&writer);
    }   // the writer finishes the file here (RAII)

    // QPdfWriter reports no errors itself, so the file is checked instead.
    QFileInfo written(path);
    if (!written.exists() || written.size() == 0) {
        return fail("Could not write the PDF file " + QDir::toNativeSeparators(path) + ".");
    }
    database.recordAuditEvent("pdf_summary_exported", month);
    return true;
}

QString ReportExporter::monthlySummaryHtml(const QString &month, const QString &ownerName,
                                           const std::vector<Transaction> &transactions,
                                           const std::vector<Budget> &budgets) const
{
    std::string monthText = month.toStdString();
    QString html = "<html><body style=\"font-family: 'Inter', 'Segoe UI', sans-serif; color: #1F2433;\">";
    html += "<h1 style=\"color: #5B3DF5;\">Finora monthly summary</h1>";
    html += "<p>" + reportMonthTitle(month) + " &middot; prepared for " + ownerName.toHtmlEscaped() + "</p>";

    // ---- Income and expenses, one row per currency (never mixed) ----
    html += "<h2>Income and expenses</h2>";
    html += tableStart({"Currency", "Income", "Expenses", "Net"});
    std::vector<CurrencySummary> summaries = summarizeByCurrency(transactions, monthText);
    bool anyActivity = false;
    for (int i = 0; i < static_cast<int>(summaries.size()); i++) {
        const CurrencySummary &summary = summaries[i];
        if (summary.monthIncomeMinor == 0 && summary.monthExpenseMinor == 0) {
            continue;
        }
        anyActivity = true;
        const std::string &code = summary.currencyCode;
        if (summary.overflow) {
            html += tableRow({htmlText(code), "Too large to show", "", ""});
        } else {
            html += tableRow({htmlText(code), htmlText(formatMoney(code, summary.monthIncomeMinor)),
                              htmlText(formatMoney(code, summary.monthExpenseMinor)),
                              htmlText(formatSignedMoney(code, monthNetOf(summary)))});
        }
    }
    if (!anyActivity) {
        html += "<tr><td colspan=\"4\">No transactions in this month.</td></tr>";
    }
    html += "</table>";

    // ---- Spending by category, per currency ----
    html += "<h2>Spending by category</h2>";
    bool anyExpense = false;
    for (int i = 0; i < static_cast<int>(summaries.size()); i++) {
        const std::string &code = summaries[i].currencyCode;
        if (summaries[i].monthExpenseMinor == 0) {
            continue;
        }
        anyExpense = true;
        bool overflow = false;
        std::vector<CategoryTotal> totals = spendingByCategory(transactions, code, monthText, overflow);
        html += tableStart({"Category (" + htmlText(code) + ")", "Spent"});
        for (int j = 0; j < static_cast<int>(totals.size()); j++) {
            html += tableRow({htmlText(totals[j].category), htmlText(formatMoney(code, totals[j].amountMinor))});
        }
        html += "</table><br>";
    }
    if (!anyExpense) {
        html += "<p>No expenses in this month.</p>";
    }

    // ---- Budgets that are tracked (paused ones are not) ----
    html += "<h2>Budgets</h2>";
    std::vector<Budget> activeBudgets;
    for (int i = 0; i < static_cast<int>(budgets.size()); i++) {
        if (!budgets[i].paused) {
            activeBudgets.push_back(budgets[i]);
        }
    }
    std::vector<BudgetProgress> progress = allBudgetProgress(activeBudgets, transactions, monthText);
    if (progress.empty()) {
        html += "<p>No active budgets.</p>";
    } else {
        html += tableStart({"Budget", "Spent", "Limit", "Used", "Status"});
        for (int i = 0; i < static_cast<int>(progress.size()); i++) {
            const BudgetProgress &item = progress[i];
            const std::string &code = item.budget.currencyCode;
            html += tableRow({htmlText(item.budget.category + " (" + code + ")"),
                              htmlText(formatMoney(code, item.spentMinor)),
                              htmlText(formatMoney(code, item.budget.limitMinor)),
                              QString::number(item.percentUsed) + "%",
                              htmlText(budgetStatusName(item.status))});
        }
        html += "</table>";
    }

    // ---- Footer ----
    int monthCount = 0;
    for (int i = 0; i < static_cast<int>(transactions.size()); i++) {
        if (monthOfDate(transactions[i].date) == monthText) {
            monthCount++;
        }
    }
    html += "<p style=\"color: #6B7080;\">" + QString::number(monthCount) + " transaction(s) in "
            + reportMonthTitle(month) + ". Made by Finora on " + QDate::currentDate().toString("d MMMM yyyy")
            + ". Amounts are never converted between currencies.</p>";
    html += "</body></html>";
    return html;
}

// ---- Reports (Milestone 6) ------------------------------------------------

bool ReportExporter::buildReport(const ReportRequest &request, PeriodReport &report)
{
    errorMessage.clear();
    blockedByPlan = false;
    report = PeriodReport();

    // The plan first: Finora Plus or larger. Nothing is read before this.
    if (!requirePlan(FEATURE_REPORTS)) {
        return false;
    }
    std::vector<Transaction> transactions;
    if (!database.loadTransactions(transactions)) {   // only the signed-in person's records
        return fail(database.lastError());
    }
    if (!buildPeriodReport(transactions, request, report)) {
        return fail("This report cannot be made: choose a valid month or year, PKR, USD or EUR, and a type.");
    }
    return true;
}

bool ReportExporter::exportReportCsv(const ReportRequest &request, const QString &path)
{
    errorMessage.clear();
    blockedByPlan = false;

    // Reports and CSV files are both part of Finora Plus.
    if (!requirePlan(FEATURE_REPORTS) || !requirePlan(FEATURE_CSV_EXPORT)) {
        return false;
    }
    PeriodReport report;
    if (!buildReport(request, report)) {
        return false;   // buildReport() has explained the problem
    }
    if (report.overflow) {
        return fail("Some totals of this report are too large to add up, so it was not exported.");
    }
    std::string text = reportCsv(report);

    // As for the transactions: the real file is replaced only when
    // everything was written, and it starts with the UTF-8 mark.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return fail("Could not write " + QDir::toNativeSeparators(path) + ": " + file.errorString());
    }
    file.write("\xEF\xBB\xBF");
    file.write(text.c_str(), static_cast<qint64>(text.size()));
    if (!file.commit()) {
        return fail("Could not write " + QDir::toNativeSeparators(path) + ": " + file.errorString());
    }
    database.recordAuditEvent("report_exported", QString::fromStdString(request.period) + ", "
                                                     + QString::fromStdString(request.currencyCode));
    return true;
}

bool ReportExporter::planBlocked() const
{
    return blockedByPlan;
}

// The plan check of the report functions: requireFeature() reads the plan
// from the signed-in person's own subscription and explains what is needed.
bool ReportExporter::requirePlan(Feature feature)
{
    if (!database.requireFeature(feature)) {
        blockedByPlan = true;
        return fail(database.lastError());
    }
    return true;
}

QString ReportExporter::lastError() const
{
    return errorMessage;
}

bool ReportExporter::fail(const QString &message)
{
    errorMessage = message;
    return false;
}
