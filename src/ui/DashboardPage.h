// =====================================================================
//  DashboardPage.h
//  The first page: a greeting, one wallet card per currency, the money
//  flow chart, budget alerts, recent transactions and spending per
//  category. Every number comes from the SQLite database.
// =====================================================================
#ifndef FINORA_DASHBOARDPAGE_H
#define FINORA_DASHBOARDPAGE_H

#include <QButtonGroup>
#include <QFrame>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

#include <vector>

#include "Page.h"
#include "components/MoneyFlowChart.h"
#include "components/MonthSwitcher.h"
#include "components/WalletCard.h"
#include "core/Budget.h"
#include "core/Transaction.h"
#include "database/BudgetStore.h"
#include "database/DatabaseManager.h"

class DashboardPage : public Page
{
    Q_OBJECT

public:
    DashboardPage(DatabaseManager &sharedDatabase, BudgetStore &sharedBudgets, QWidget *parent = nullptr);

    // Loads everything from the database again and redraws (Page).
    void refresh() override;

    // Used by the tests and by the "+ Add Transaction" button.
    bool saveNewTransaction(Transaction &transaction);

    MonthSwitcher *monthSwitcher() const;
    MoneyFlowChart *chart() const;
    WalletCard *walletCard(const QString &currencyCode) const;
    QString chartCurrency() const;
    void setChartCurrency(const QString &currencyCode);
    int budgetAlertCount() const;

signals:
    // Asks MainWindow to show another page, for example after "View all".
    void openPageRequested(int pageNumber);

private slots:
    void onAddTransactionClicked();
    void onViewAllClicked();
    void onManageBudgetsClicked();
    void onChartOptionChanged();
    void onMonthChanged();

private:
    QWidget *createHeader();
    QWidget *createWalletRow();
    QFrame *createChartCard();
    QFrame *createAlertsCard();
    QFrame *createRecentCard();
    QFrame *createSpendingCard();
    QFrame *createCardFrame(const QString &title, QLabel *&subtitleLabel, QPushButton *linkButton,
                            QVBoxLayout *&bodyLayout);

    void showWallets();
    void showChart();
    void showBudgetAlerts();
    void showRecentTransactions();
    void showSpending();
    void updateLegend();

    DatabaseManager &database;   // created in main(); this page only uses it
    BudgetStore &budgetStore;    // created in MainWindow; this page only uses it
    std::vector<Transaction> transactions;
    std::vector<Budget> budgets;
    int alertCount = 0;

    QLabel *greetingLabel = nullptr;
    MonthSwitcher *switcher = nullptr;
    std::vector<WalletCard *> walletCards;

    MoneyFlowChart *flowChart = nullptr;
    QButtonGroup *currencyGroup = nullptr;   // PKR | USD | EUR
    QButtonGroup *rangeGroup = nullptr;      // 6 months | 12 months
    QLabel *chartSubtitle = nullptr;
    QLabel *chartSummary = nullptr;
    QLabel *legendLabel = nullptr;

    QLabel *alertsSubtitle = nullptr;
    QVBoxLayout *alertsLayout = nullptr;

    QTableWidget *recentTable = nullptr;
    QLabel *recentEmpty = nullptr;

    QLabel *spendingSubtitle = nullptr;
    QVBoxLayout *spendingLayout = nullptr;
};

#endif // FINORA_DASHBOARDPAGE_H
