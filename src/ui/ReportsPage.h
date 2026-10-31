// =====================================================================
//  ReportsPage.h
//  The Reports page (Milestone 6): the income, expenses and categories
//  of one currency for a chosen month or year, compared with the period
//  before and month by month, and the report as a CSV file.
//  Reports are part of Finora Plus; ReportExporter checks the plan.
// =====================================================================
#ifndef FINORA_REPORTSPAGE_H
#define FINORA_REPORTSPAGE_H

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

#include "Page.h"
#include "components/MonthSwitcher.h"
#include "core/Reports.h"
#include "database/DatabaseManager.h"
#include "reports/ReportExporter.h"

class ReportsPage : public Page
{
    Q_OBJECT

public:
    ReportsPage(DatabaseManager &sharedDatabase, QWidget *parent = nullptr);

    // Makes the report again for the chosen period, currency and type (Page).
    void refresh() override;

    // Writes the report on screen to `path` as a CSV file. Returns true on
    // success. Public so the tests can call it without a file dialog.
    bool exportReportTo(const QString &path);

    // What the page shows, for the tests.
    const PeriodReport &shownReport() const;
    bool isLocked() const;

signals:
    // "See plans" asks MainWindow to open the Subscription page.
    void openPageRequested(int pageNumber);

private slots:
    void onChoiceChanged();
    void onExportClicked();
    void onSeePlansClicked();

private:
    QWidget *createHeader();
    QFrame *createChoices();
    QFrame *createPlanBanner();
    QFrame *createSummaryCard(const QString &title, QLabel *&value, QLabel *&detail);
    QFrame *createTableCard(const QString &title, QLabel *&titleLabel, QTableWidget *&table, QLabel *&emptyNote);
    ReportRequest chosenRequest() const;
    QString periodName(const std::string &period) const;
    void showReport();
    void showLocked();
    void fillCategoryTable();
    void fillMonthsTable();

    DatabaseManager &database;   // created in main(); this page only uses it
    ReportExporter exporter;     // the report functions and their plan checks (composition)
    PeriodReport report;         // the report on screen
    bool locked = false;

    QPushButton *exportButton = nullptr;
    QComboBox *kindBox = nullptr;       // Month or Year
    MonthSwitcher *switcher = nullptr;  // the month, in the month view
    QComboBox *yearBox = nullptr;       // the year, in the year view
    QComboBox *currencyBox = nullptr;
    QComboBox *typeBox = nullptr;       // which categories are listed
    QFrame *planBanner = nullptr;
    QLabel *planBannerText = nullptr;
    QWidget *reportArea = nullptr;      // everything below the choices; hidden when locked
    QLabel *periodTitle = nullptr;
    QLabel *otherCurrencies = nullptr;
    QLabel *incomeValue = nullptr;
    QLabel *incomeDetail = nullptr;
    QLabel *expenseValue = nullptr;
    QLabel *expenseDetail = nullptr;
    QLabel *netValue = nullptr;
    QLabel *netDetail = nullptr;
    QLabel *categoryTitle = nullptr;
    QTableWidget *categoryTable = nullptr;
    QLabel *categoryEmpty = nullptr;
    QLabel *monthsTitle = nullptr;
    QTableWidget *monthsTable = nullptr;
    QLabel *monthsEmpty = nullptr;
};

#endif // FINORA_REPORTSPAGE_H
