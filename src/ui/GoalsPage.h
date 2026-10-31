// =====================================================================
//  GoalsPage.h
//  The Goals page (Milestone 5): savings goals with their progress, and
//  buttons to add money, take money out, see the history, edit and delete.
// =====================================================================
#ifndef FINORA_GOALSPAGE_H
#define FINORA_GOALSPAGE_H

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include <vector>

#include "Page.h"
#include "core/Goal.h"
#include "database/DatabaseManager.h"
#include "database/GoalStore.h"

class GoalsPage : public Page
{
    Q_OBJECT

public:
    GoalsPage(DatabaseManager &sharedDatabase, QWidget *parent = nullptr);

    // Loads the goals again and draws the cards (Page).
    void refresh() override;

    // The actions behind the buttons. Each returns true on success;
    // otherwise it explains the problem. Public so the tests can call them.
    bool saveNewGoal(Goal &goal);
    bool saveEditedGoal(const Goal &goal);
    bool saveContribution(GoalContribution &contribution);
    bool deleteGoalById(int id);

    // What the page currently shows, for the tests.
    const std::vector<GoalProgress> &shownProgress() const;
    // True when the current plan does not include savings goals.
    bool isLocked() const;

signals:
    // "See plans" asks MainWindow to open the Subscription page.
    void openPageRequested(int pageNumber);

private slots:
    void onAddClicked();
    void onAddMoneyClicked();
    void onTakeOutClicked();
    void onHistoryClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onSeePlansClicked();

private:
    QWidget *createHeader();
    QFrame *createPlanBanner();
    QFrame *createEmptyState();
    QFrame *createGoalCard(const GoalProgress &progress);
    QPushButton *createCardButton(const QString &text, const QString &style, int goalId, QWidget *parent);
    void showGoals();
    bool explainIfLocked();
    void openContributionDialog(bool takeOut);
    bool findGoal(int id, Goal &result) const;
    void showProblem(const QString &action, const QString &reason);

    DatabaseManager &database;   // created in main(); this page only uses it
    GoalStore goalStore;         // the goal part of the same database (composition)
    std::vector<Goal> goals;
    std::vector<GoalProgress> progressList;   // one entry per goal, for today
    bool locked = false;

    QPushButton *addButton = nullptr;
    QFrame *planBanner = nullptr;      // shown when the plan does not include goals
    QLabel *planBannerText = nullptr;
    QWidget *cardArea = nullptr;
    QGridLayout *cardGrid = nullptr;
    QFrame *emptyState = nullptr;
    QLabel *emptyIcon = nullptr;
};

#endif // FINORA_GOALSPAGE_H
