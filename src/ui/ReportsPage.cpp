// =====================================================================
//  ReportsPage.cpp
// =====================================================================
#include "ReportsPage.h"

#include <QDate>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollArea>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "PageHeader.h"
#include "Sidebar.h"   // PAGE_SUBSCRIPTION
#include "UiHelpers.h"
#include "components/Toast.h"
#include "core/Budget.h"   // percentUsed
#include "core/Plans.h"

// "PKR 1,500.00 more than September 2026", for an income or expense total.
QString comparisonText(const std::string &code, long long now, long long before, const QString &previousName)
{
    if (previousName.isEmpty()) {
        return "";
    }
    if (now == before) {
        return "The same as " + previousName;
    }
    // Both totals are 0 or more, so neither difference can overflow.
    if (now > before) {
        return moneyText(code, now - before) + " more than " + previousName;
    }
    return moneyText(code, before - now) + " less than " + previousName;
}

// "3 transactions. PKR 1,500.00 more than September 2026."
QString totalDetail(int count, const QString &comparison)
{
    QString text = countText(count, "transaction", "transactions") + ".";
    if (!comparison.isEmpty()) {
        text += " " + comparison + ".";
    }
    return text;
}

// Makes a table exactly as tall as its header and rows.
void fitTableHeight(QTableWidget *table)
{
    int height = table->horizontalHeader()->sizeHint().height() + 4;
    for (int row = 0; row < table->rowCount(); row++) {
        height += table->rowHeight(row);
    }
    table->setFixedHeight(height);
}

// A table cell aligned to the right, for amounts and numbers.
QTableWidgetItem *numberCell(const QString &text)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

ReportsPage::ReportsPage(DatabaseManager &sharedDatabase, QWidget *parent)
    : Page(parent), database(sharedDatabase), exporter(sharedDatabase)
{
    setObjectName("reportsPage");

    QWidget *content = new QWidget(this);
    content->setObjectName("pageContent");

    // Everything below the choices. It is hidden while reports are locked.
    reportArea = new QWidget(content);
    periodTitle = new QLabel(reportArea);
    periodTitle->setObjectName("cardTitle");
    QLabel *separateNote = new QLabel("Each currency is reported on its own: PKR, USD and EUR are never added "
                                      "together.", reportArea);
    separateNote->setObjectName("mutedText");
    separateNote->setWordWrap(true);
    otherCurrencies = new QLabel(reportArea);
    otherCurrencies->setObjectName("mutedText");

    QHBoxLayout *summaryRow = new QHBoxLayout();
    summaryRow->setSpacing(16);
    summaryRow->addWidget(createSummaryCard("Income", incomeValue, incomeDetail), 1);
    summaryRow->addWidget(createSummaryCard("Expenses", expenseValue, expenseDetail), 1);
    summaryRow->addWidget(createSummaryCard("Net (income minus expenses)", netValue, netDetail), 1);

    QVBoxLayout *areaLayout = new QVBoxLayout(reportArea);
    areaLayout->setContentsMargins(0, 0, 0, 0);
    areaLayout->setSpacing(12);
    areaLayout->addWidget(periodTitle);
    areaLayout->addWidget(separateNote);
    areaLayout->addWidget(otherCurrencies);
    areaLayout->addSpacing(4);
    areaLayout->addLayout(summaryRow);
    areaLayout->addSpacing(4);
    // The two tables use the whole width, so long amounts always fit.
    areaLayout->addWidget(createTableCard("Categories", categoryTitle, categoryTable, categoryEmpty));
    areaLayout->addWidget(createTableCard("Month by month", monthsTitle, monthsTable, monthsEmpty));

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(20);
    contentLayout->addWidget(createHeader());
    contentLayout->addWidget(createChoices());
    contentLayout->addWidget(createPlanBanner());
    contentLayout->addWidget(reportArea);
    contentLayout->addStretch();

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setObjectName("pageScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    refresh();
}

QWidget *ReportsPage::createHeader()
{
    QWidget *header = new QWidget(this);

    exportButton = new QPushButton("Export report", header);
    exportButton->setObjectName("secondaryButton");
    exportButton->setAccessibleName("Export the report as a CSV file");
    exportButton->setCursor(Qt::PointingHandCursor);
    connect(exportButton, &QPushButton::clicked, this, &ReportsPage::onExportClicked);

    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(new PageHeader("Reports", "Income, expenses and categories for a month or a year", header));
    layout->addStretch();
    layout->addWidget(exportButton);
    return header;
}

// The choices: month or year, which one, the currency and the categories' type.
QFrame *ReportsPage::createChoices()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    kindBox = new QComboBox(card);
    kindBox->setObjectName("reportKindBox");
    kindBox->setAccessibleName("Report for a month or a year");
    kindBox->addItem("Month", REPORT_MONTH);
    kindBox->addItem("Year", REPORT_YEAR);

    switcher = new MonthSwitcher(card);

    // This year first, then the nine years before it, and next year for
    // transactions that were entered ahead of time.
    yearBox = new QComboBox(card);
    yearBox->setObjectName("reportYearBox");
    yearBox->setAccessibleName("Year");
    int thisYear = QDate::currentDate().year();
    for (int year = thisYear + 1; year >= thisYear - 9; year--) {
        yearBox->addItem(QString::number(year), QString::number(year));
    }
    yearBox->setCurrentIndex(yearBox->findData(QString::number(thisYear)));
    yearBox->hide();

    currencyBox = new QComboBox(card);
    currencyBox->setObjectName("reportCurrencyBox");
    currencyBox->setAccessibleName("Currency");
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        QString code = QString::fromUtf8(SUPPORTED_CURRENCIES[i].code);
        currencyBox->addItem(code + "  " + QString::fromUtf8(SUPPORTED_CURRENCIES[i].name), code);
    }

    typeBox = new QComboBox(card);
    typeBox->setObjectName("reportTypeBox");
    typeBox->setAccessibleName("Categories of");
    typeBox->addItem("Income and expenses", "");
    typeBox->addItem("Income only", QString::fromStdString(TYPE_INCOME));
    typeBox->addItem("Expenses only", QString::fromStdString(TYPE_EXPENSE));

    // Two rows, so the choices fit on small screens too: the period first,
    // then the currency and the categories.
    QHBoxLayout *periodRow = new QHBoxLayout();
    periodRow->setSpacing(10);
    periodRow->addWidget(new QLabel("Report for", card));
    periodRow->addWidget(kindBox);
    periodRow->addWidget(switcher);
    periodRow->addWidget(yearBox);
    periodRow->addStretch();

    QHBoxLayout *filterRow = new QHBoxLayout();
    filterRow->setSpacing(10);
    filterRow->addWidget(new QLabel("Currency", card));
    filterRow->addWidget(currencyBox);
    filterRow->addSpacing(12);
    filterRow->addWidget(new QLabel("Categories", card));
    filterRow->addWidget(typeBox);
    filterRow->addStretch();

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 14, 20, 14);
    layout->setSpacing(10);
    layout->addLayout(periodRow);
    layout->addLayout(filterRow);

    connect(kindBox, &QComboBox::currentIndexChanged, this, &ReportsPage::onChoiceChanged);
    connect(switcher, &MonthSwitcher::monthChanged, this, &ReportsPage::onChoiceChanged);
    connect(yearBox, &QComboBox::currentIndexChanged, this, &ReportsPage::onChoiceChanged);
    connect(currencyBox, &QComboBox::currentIndexChanged, this, &ReportsPage::onChoiceChanged);
    connect(typeBox, &QComboBox::currentIndexChanged, this, &ReportsPage::onChoiceChanged);
    return card;
}

// Shown when the plan does not include reports.
QFrame *ReportsPage::createPlanBanner()
{
    planBanner = new QFrame(this);
    planBannerText = new QLabel(planBanner);
    planBannerText->setObjectName("infoBanner");
    planBannerText->setWordWrap(true);

    QPushButton *seePlans = new QPushButton("See plans", planBanner);
    seePlans->setObjectName("linkButton");
    seePlans->setCursor(Qt::PointingHandCursor);
    connect(seePlans, &QPushButton::clicked, this, &ReportsPage::onSeePlansClicked);

    QHBoxLayout *layout = new QHBoxLayout(planBanner);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(planBannerText, 1);
    layout->addWidget(seePlans);
    return planBanner;
}

QFrame *ReportsPage::createSummaryCard(const QString &title, QLabel *&value, QLabel *&detail)
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");
    QLabel *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("cardSubtitle");
    value = new QLabel(card);
    value->setObjectName("reportValue");
    detail = new QLabel(card);
    detail->setObjectName("mutedText");
    detail->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 14, 18, 16);
    layout->setSpacing(6);
    layout->addWidget(titleLabel);
    layout->addWidget(value);
    layout->addWidget(detail);
    return card;
}

QFrame *ReportsPage::createTableCard(const QString &title, QLabel *&titleLabel, QTableWidget *&table,
                                     QLabel *&emptyNote)
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");
    titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("cardTitle");

    table = new QTableWidget(card);
    table->setObjectName("historyTable");
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setFrameShape(QFrame::NoFrame);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // the page scrolls, not the table

    emptyNote = new QLabel(card);
    emptyNote->setObjectName("mutedText");
    emptyNote->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 14, 18, 16);
    layout->setSpacing(8);
    layout->addWidget(titleLabel);
    layout->addWidget(table, 1);
    layout->addWidget(emptyNote);
    return card;
}

// ---- Making and showing the report --------------------------------------

ReportRequest ReportsPage::chosenRequest() const
{
    ReportRequest request;
    request.kind = static_cast<ReportPeriodKind>(kindBox->currentData().toInt());
    request.period = switcher->month().toStdString();
    if (request.kind == REPORT_YEAR) {
        request.period = yearBox->currentData().toString().toStdString();
    }
    request.currencyCode = currencyBox->currentData().toString().toStdString();
    request.type = typeBox->currentData().toString().toStdString();
    return request;
}

// "October 2026" for a month, "2026" for a year.
QString ReportsPage::periodName(const std::string &period) const
{
    if (report.request.kind == REPORT_YEAR) {
        return QString::fromStdString(period);
    }
    return monthTitle(QString::fromStdString(period));
}

void ReportsPage::refresh()
{
    switcher->refreshIcons();
    bool monthView = static_cast<ReportPeriodKind>(kindBox->currentData().toInt()) == REPORT_MONTH;
    switcher->setVisible(monthView);
    yearBox->setVisible(!monthView);

    // ReportExporter checks the plan before it reads a single record.
    if (!exporter.buildReport(chosenRequest(), report)) {
        if (exporter.planBlocked()) {
            showLocked();
            return;
        }
        locked = false;
        planBanner->hide();
        reportArea->hide();
        QMessageBox::warning(this, "Finora", "Could not make the report.\n\nReason: " + exporter.lastError());
        return;
    }
    locked = false;
    showReport();
}

void ReportsPage::showLocked()
{
    locked = true;
    report = PeriodReport();
    QString plan = QString::fromStdString(planName(FEATURES[FEATURE_REPORTS].lowestPlan));
    planBannerText->setText(QString::fromStdString(lockedFeatureMessage(FEATURE_REPORTS, database.currentPlan()))
                            + " Reports show your income, expenses and categories for any month or year, "
                              "compared with the period before.");
    planBanner->show();
    reportArea->hide();
    showLockIfNeeded(exportButton, true, plan);
}

void ReportsPage::showReport()
{
    planBanner->hide();
    reportArea->show();
    showLockIfNeeded(exportButton, false, QString());

    std::string code = report.request.currencyCode;
    QString period = periodName(report.request.period);
    QString previous;
    if (!report.previousPeriod.empty()) {
        previous = periodName(report.previousPeriod);
    }
    QDate first = QDate::fromString(QString::fromStdString(report.firstDay), "yyyy-MM-dd");
    QDate last = QDate::fromString(QString::fromStdString(report.lastDay), "yyyy-MM-dd");
    periodTitle->setText(period + " · " + QString::fromStdString(code) + " · " + first.toString("d MMM yyyy")
                         + " to " + last.toString("d MMM yyyy"));

    // The other currencies of the same period are only named, never added.
    QStringList others;
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        ReportRequest other = report.request;
        other.currencyCode = SUPPORTED_CURRENCIES[i].code;
        PeriodReport otherReport;
        if (other.currencyCode != code && exporter.buildReport(other, otherReport)) {
            int count = otherReport.totals.incomeCount + otherReport.totals.expenseCount;
            if (count > 0) {
                others << QString::fromStdString(other.currencyCode) + " ("
                              + countText(count, "transaction", "transactions") + ")";
            }
        }
    }
    otherCurrencies->setVisible(!others.isEmpty());
    otherCurrencies->setText("Also in " + period + ": " + others.join(", ")
                             + ". Choose that currency to see its report.");

    const PeriodTotals &totals = report.totals;
    const PeriodTotals &before = report.previousTotals;
    if (report.overflow) {
        incomeValue->setText("Too large to show");
        expenseValue->setText("Too large to show");
        netValue->setText("-");
        incomeDetail->clear();
        expenseDetail->clear();
        netDetail->clear();
    } else {
        incomeValue->setText(moneyText(code, totals.incomeMinor));
        expenseValue->setText(moneyText(code, totals.expenseMinor));
        netValue->setText(moneyText(code, netOf(totals)));
        incomeDetail->setText(totalDetail(totals.incomeCount,
                                          comparisonText(code, totals.incomeMinor, before.incomeMinor, previous)));
        expenseDetail->setText(totalDetail(totals.expenseCount,
                                           comparisonText(code, totals.expenseMinor, before.expenseMinor, previous)));
        netDetail->clear();
        if (!previous.isEmpty()) {
            netDetail->setText(previous + ": " + moneyText(code, netOf(before)) + ".");
        }
    }
    fillCategoryTable();
    fillMonthsTable();
}

void ReportsPage::fillCategoryTable()
{
    std::string code = report.request.currencyCode;
    categoryTable->clear();
    categoryTable->setColumnCount(5);
    categoryTable->setHorizontalHeaderLabels({"Category", "Type", "Amount", "Share", "Transactions"});
    categoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < 5; column++) {
        categoryTable->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }

    int count = static_cast<int>(report.categories.size());
    categoryTable->setRowCount(count);
    for (int i = 0; i < count; i++) {
        const ReportCategory &line = report.categories[i];
        bool income = line.type == TYPE_INCOME;
        // The share of the category in all income, or in all expenses, of the period.
        long long typeTotal = income ? report.totals.incomeMinor : report.totals.expenseMinor;
        categoryTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(line.category)));
        categoryTable->setItem(i, 1, new QTableWidgetItem(income ? "Income" : "Expense"));
        categoryTable->setItem(i, 2, numberCell(moneyText(code, line.amountMinor)));
        categoryTable->setItem(i, 3, numberCell(QString::number(percentUsed(line.amountMinor, typeTotal)) + "%"));
        categoryTable->setItem(i, 4, numberCell(QString::number(line.count)));
    }
    fitTableHeight(categoryTable);
    categoryTable->setVisible(count > 0);

    QString what = "income or expenses";
    if (report.request.type == TYPE_INCOME) {
        what = "income";
    } else if (report.request.type == TYPE_EXPENSE) {
        what = "expenses";
    }
    categoryEmpty->setVisible(count == 0);
    categoryEmpty->setText("No " + what + " in " + QString::fromStdString(code) + " in "
                           + periodName(report.request.period) + ".");
}

void ReportsPage::fillMonthsTable()
{
    std::string code = report.request.currencyCode;
    if (report.request.kind == REPORT_YEAR) {
        monthsTitle->setText("Month by month in " + periodName(report.request.period));
    } else {
        monthsTitle->setText("The 6 months up to " + periodName(report.request.period));
    }
    monthsTable->clear();
    monthsTable->setColumnCount(4);
    monthsTable->setHorizontalHeaderLabels({"Month", "Income", "Expenses", "Net"});
    for (int column = 0; column < 4; column++) {
        monthsTable->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Stretch);
    }

    int count = static_cast<int>(report.months.size());
    monthsTable->setRowCount(count);
    bool anyActivity = false;
    for (int i = 0; i < count; i++) {
        const MonthFlow &flow = report.months[i];
        QString name = shortMonthName(QString::fromStdString(flow.month)) + " "
                       + QString::fromStdString(flow.month.substr(0, 4));
        monthsTable->setItem(i, 0, new QTableWidgetItem(name));
        monthsTable->setItem(i, 1, numberCell(moneyText(code, flow.incomeMinor)));
        monthsTable->setItem(i, 2, numberCell(moneyText(code, flow.expenseMinor)));
        monthsTable->setItem(i, 3, numberCell(moneyText(code, flow.incomeMinor - flow.expenseMinor)));
        if (flow.incomeMinor > 0 || flow.expenseMinor > 0) {
            anyActivity = true;
        }
    }
    fitTableHeight(monthsTable);
    monthsEmpty->setVisible(!anyActivity);
    monthsEmpty->setText("No " + QString::fromStdString(code) + " transactions in these months.");
}

// ---- Actions ---------------------------------------------------------------

bool ReportsPage::exportReportTo(const QString &path)
{
    if (!exporter.exportReportCsv(chosenRequest(), path)) {
        if (exporter.planBlocked()) {
            showLocked();
        } else {
            QMessageBox::warning(this, "Finora", "Could not export the report.\n\nReason: " + exporter.lastError());
        }
        return false;
    }
    Toast::showMessage(this, "Report exported to " + QFileInfo(path).fileName() + ".");
    return true;
}

const PeriodReport &ReportsPage::shownReport() const
{
    return report;
}

bool ReportsPage::isLocked() const
{
    return locked;
}

void ReportsPage::onChoiceChanged()
{
    refresh();
}

void ReportsPage::onExportClicked()
{
    // Below Finora Plus, explain the plans instead of asking for a file.
    // This only saves the person a wasted dialog: ReportExporter checks
    // the plan again before it reads a single record.
    PlanId plan = database.currentPlan();
    if (!planIncludes(plan, FEATURE_REPORTS)) {
        if (askToSeePlans(this, "Reports", QString::fromStdString(lockedFeatureMessage(FEATURE_REPORTS, plan)))) {
            emit openPageRequested(PAGE_SUBSCRIPTION);
        }
        return;
    }
    ReportRequest request = chosenRequest();
    QString suggested = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Finora report "
                        + QString::fromStdString(request.period) + " " + QString::fromStdString(request.currencyCode)
                        + ".csv";
    QString path = QFileDialog::getSaveFileName(this, "Export report", suggested, "CSV files (*.csv)");
    if (!path.isEmpty()) {
        exportReportTo(path);
    }
}

void ReportsPage::onSeePlansClicked()
{
    emit openPageRequested(PAGE_SUBSCRIPTION);
}
