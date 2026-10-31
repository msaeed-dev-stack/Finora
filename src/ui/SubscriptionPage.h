// =====================================================================
//  SubscriptionPage.h
//  The Subscription page (Milestone 4, Phase 4C): the person's plan and
//  its status, how much of the plan is used, the four plans side by
//  side, a comparison of every feature, and the billing history.
//  Every payment in Finora is simulated, and the page says so at the top.
// =====================================================================
#ifndef FINORA_SUBSCRIPTIONPAGE_H
#define FINORA_SUBSCRIPTIONPAGE_H

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QTableWidget>
#include <QVBoxLayout>

#include <vector>

#include "Page.h"
#include "billing/SubscriptionService.h"
#include "core/Budget.h"
#include "core/Subscription.h"
#include "database/BudgetStore.h"
#include "database/DatabaseManager.h"
#include "database/SubscriptionStore.h"

class SubscriptionPage : public Page
{
    Q_OBJECT

public:
    // `sharedService` changes plans; it is nullptr when Finora runs without
    // accounts (the tests of Milestones 1 to 4A), and then the page only
    // shows the plans.
    SubscriptionPage(DatabaseManager &sharedDatabase, BudgetStore &sharedBudgets,
                     SubscriptionService *sharedService = nullptr, QWidget *parent = nullptr);

    // Loads the subscription, the budgets and the billing history again (Page).
    void refresh() override;

    // What the page shows, for the tests.
    bool isSignedIn() const;
    const Subscription &shownSubscription() const;
    int shownPaymentCount() const;
    int planColumns() const;

    // The actions behind the buttons, public so the tests can call them.
    void choosePlan(PlanId plan);

signals:
    // "Manage budgets" asks MainWindow to open the Budgets page.
    void openPageRequested(int pageNumber);

    // The plan, its status or the billing history changed; MainWindow
    // updates the plan under the person's name.
    void planChanged();

protected:
    // Four plan cards side by side on a wide window, two by two on a
    // narrow one. Qt calls this whenever the page changes its size.
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onManageBudgetsClicked();
    void onToggleComparisonClicked();
    void onPlanButtonClicked();
    void onPayCurrentPlanClicked();
    void onCancelClicked();
    void onKeepPlanClicked();
    void onTrialClicked();
    void onSimulateEndClicked();

private:
    QWidget *createHeader();
    QFrame *createCurrentPlanCard();
    QFrame *createUsageCard();
    QFrame *createPlanCard(PlanId plan);
    void addFeatureLine(QVBoxLayout *layout, QWidget *card, Feature feature);
    QFrame *createComparisonCard();
    QFrame *createHistoryCard();
    void showCurrentPlan();
    QString statusDetails() const;
    void showUsage();
    void showPlanCards();
    void showHistory();
    void showActions();
    QPushButton *addAction(const QString &text, const QString &style);
    void arrangePlanCards(int columns);
    void openCheckout(PlanId plan, PlanChange change);
    bool confirmAction(const QString &title, const QString &text, const QString &yesText, const QString &noText);
    void afterChange(const QString &message);
    void showServiceError();

    DatabaseManager &database;   // created in main(); this page only uses it
    BudgetStore &budgetStore;    // created in MainWindow; this page only uses it
    SubscriptionStore store;     // reads the signed-in person's own subscription
    SubscriptionService *service = nullptr;   // changes plans; nullptr without accounts

    bool signedIn = false;
    Subscription subscription;   // up to date: every deadline already applied
    std::vector<PaymentRecord> payments;
    std::vector<Budget> budgets;

    QLabel *currentName = nullptr;
    QLabel *currentStatus = nullptr;
    QLabel *currentPrice = nullptr;
    QLabel *currentDetails = nullptr;
    QLabel *lastPaymentLabel = nullptr;
    QHBoxLayout *actionsLayout = nullptr;   // the buttons of the current plan card
    QLabel *usageNumber = nullptr;
    QLabel *usageText = nullptr;
    QProgressBar *usageBar = nullptr;

    QWidget *planArea = nullptr;
    QGridLayout *planGrid = nullptr;
    QFrame *planCards[PLAN_COUNT] = {};        // one card per plan, in PlanId order
    QLabel *currentBadges[PLAN_COUNT] = {};    // "Current plan" on the person's own card
    QPushButton *planButtons[PLAN_COUNT] = {}; // the action of each plan card
    QPushButton *trialButton = nullptr;        // "Try Pro free for 14 days" on the Pro card
    int columnCount = 0;                       // plan cards per row now: 4 or 2

    QPushButton *comparisonButton = nullptr;
    QTableWidget *comparisonTable = nullptr;
    QTableWidget *historyTable = nullptr;
    QLabel *historyEmpty = nullptr;
};

#endif // FINORA_SUBSCRIPTIONPAGE_H
