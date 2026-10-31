// =====================================================================
//  GoalsPage.cpp
// =====================================================================
#include "GoalsPage.h"

#include <QDate>
#include <QDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProgressBar>
#include <QScrollArea>
#include <QVBoxLayout>

#include "GoalDialogs.h"
#include "PageHeader.h"
#include "Sidebar.h"   // PAGE_SUBSCRIPTION
#include "Theme.h"
#include "UiHelpers.h"
#include "components/Toast.h"
#include "core/Money.h"
#include "core/Plans.h"

const int GOAL_CARD_COLUMNS = 2;   // goal cards per row

// "31 Mar 2027" from "2027-03-31".
QString goalDateText(const std::string &isoDate)
{
    return QDate::fromString(QString::fromStdString(isoDate), "yyyy-MM-dd").toString("d MMM yyyy");
}

GoalsPage::GoalsPage(DatabaseManager &sharedDatabase, QWidget *parent)
    : Page(parent), database(sharedDatabase), goalStore(sharedDatabase)
{
    setObjectName("goalsPage");

    QWidget *content = new QWidget(this);
    content->setObjectName("pageContent");

    cardArea = new QWidget(content);
    cardGrid = new QGridLayout(cardArea);
    cardGrid->setContentsMargins(0, 0, 0, 0);
    cardGrid->setHorizontalSpacing(16);
    cardGrid->setVerticalSpacing(16);

    emptyState = createEmptyState();

    // Goals and balances are kept apart on purpose, so the page says so.
    QLabel *balanceNote = new QLabel("A goal only keeps track of money you set aside. Adding or taking out money "
                                     "here does not add a transaction, so your balances and budgets do not "
                                     "change.", content);
    balanceNote->setObjectName("goalsNote");
    balanceNote->setWordWrap(true);

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(20);
    contentLayout->addWidget(createHeader());
    contentLayout->addWidget(balanceNote);
    contentLayout->addWidget(createPlanBanner());
    contentLayout->addWidget(cardArea);
    contentLayout->addWidget(emptyState);
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

QWidget *GoalsPage::createHeader()
{
    QWidget *header = new QWidget(this);

    addButton = new QPushButton("+  New Goal", header);
    addButton->setObjectName("primaryButton");
    addButton->setAccessibleName("New Goal");
    addButton->setCursor(Qt::PointingHandCursor);
    connect(addButton, &QPushButton::clicked, this, &GoalsPage::onAddClicked);

    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(new PageHeader("Savings Goals", "Save step by step for the things that matter", header));
    layout->addStretch();
    layout->addWidget(addButton);
    return header;
}

// Shown when the plan does not include savings goals: the goals stay
// visible, and the banner says what the person can and cannot do.
QFrame *GoalsPage::createPlanBanner()
{
    planBanner = new QFrame(this);

    planBannerText = new QLabel(planBanner);
    planBannerText->setObjectName("infoBanner");
    planBannerText->setWordWrap(true);

    QPushButton *seePlans = new QPushButton("See plans", planBanner);
    seePlans->setObjectName("linkButton");
    seePlans->setCursor(Qt::PointingHandCursor);
    connect(seePlans, &QPushButton::clicked, this, &GoalsPage::onSeePlansClicked);

    QHBoxLayout *layout = new QHBoxLayout(planBanner);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(planBannerText, 1);
    layout->addWidget(seePlans);
    return planBanner;
}

QFrame *GoalsPage::createEmptyState()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    emptyIcon = new QLabel(card);
    emptyIcon->setObjectName("emptyIcon");
    emptyIcon->setFixedSize(64, 64);
    emptyIcon->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("No savings goals yet", card);
    title->setObjectName("emptyTitle");
    title->setAlignment(Qt::AlignCenter);

    QLabel *text = new QLabel("Choose something to save for, such as a laptop or an emergency fund. Finora "
                              "follows your progress and tells you how much to save each month.", card);
    text->setObjectName("emptyText");
    text->setAlignment(Qt::AlignCenter);
    text->setWordWrap(true);

    QPushButton *createButton = new QPushButton("Create your first goal", card);
    createButton->setObjectName("primaryButton");
    createButton->setCursor(Qt::PointingHandCursor);
    connect(createButton, &QPushButton::clicked, this, &GoalsPage::onAddClicked);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(40, 44, 40, 44);
    layout->setSpacing(10);
    layout->addWidget(emptyIcon, 0, Qt::AlignHCenter);
    layout->addSpacing(6);
    layout->addWidget(title);
    layout->addWidget(text);
    layout->addSpacing(8);
    layout->addWidget(createButton, 0, Qt::AlignHCenter);
    return card;
}

// ---- Loading and drawing -------------------------------------------------

void GoalsPage::refresh()
{
    goals.clear();
    // Goals belong to an account: with nobody signed in there is nothing to load.
    int userId = 0;
    if (database.signedInUserId(userId) && !goalStore.loadGoals(goals)) {
        showProblem("load the goals", goalStore.lastError());
    }
    locked = !planIncludes(database.currentPlan(), FEATURE_SAVINGS_GOALS);
    emptyIcon->setPixmap(tintedIcon("goals", themeColors().primaryText, 30));
    showGoals();
}

void GoalsPage::showGoals()
{
    std::string today = QDate::currentDate().toString("yyyy-MM-dd").toStdString();
    progressList.clear();
    int count = static_cast<int>(goals.size());
    for (int i = 0; i < count; i++) {
        progressList.push_back(goalProgressFor(goals[i], today));
    }

    // The plan banner and the locks on the buttons (Phase 4C style).
    QString planNeeded = QString::fromStdString(planName(FEATURES[FEATURE_SAVINGS_GOALS].lowestPlan));
    planBanner->setVisible(locked);
    planBannerText->setText(QString::fromStdString(lockedFeatureMessage(FEATURE_SAVINGS_GOALS, database.currentPlan()))
                            + " Your goals are kept and shown here, and you can delete them. Adding goals "
                              "or money needs " + planNeeded + " or a larger plan.");
    showLockIfNeeded(addButton, locked, planNeeded);

    clearLayout(cardGrid);
    for (int i = 0; i < count; i++) {
        cardGrid->addWidget(createGoalCard(progressList[i]), i / GOAL_CARD_COLUMNS, i % GOAL_CARD_COLUMNS);
    }
    for (int column = 0; column < GOAL_CARD_COLUMNS; column++) {
        cardGrid->setColumnStretch(column, 1);   // equal widths, also with one card
    }
    cardArea->setVisible(count > 0);
    emptyState->setVisible(count == 0);
}

// A card for one goal: name, status, progress bar, amounts, advice and buttons.
QFrame *GoalsPage::createGoalCard(const GoalProgress &progress)
{
    const Goal &goal = progress.goal;
    std::string code = goal.currencyCode;

    QFrame *card = new QFrame(cardArea);
    card->setObjectName("card");
    card->setProperty("goalId", goal.id);
    card->setAccessibleName(QString::fromStdString(goal.name) + " savings goal");

    QLabel *name = new QLabel(QString::fromStdString(goal.name), card);
    name->setObjectName("goalName");
    QString dateLine = QString::fromStdString(code) + " · no target date";
    if (!goal.targetDate.empty()) {
        dateLine = QString::fromStdString(code) + " · by " + goalDateText(goal.targetDate);
    }
    QLabel *details = new QLabel(dateLine, card);
    details->setObjectName("mutedText");

    // The status chip and the colour of the bar.
    QString status = "In progress";
    QString tone = "info";
    if (progress.reached) {
        status = "Reached";
        tone = "success";
    } else if (progress.overdue) {
        status = "Date passed";
        tone = "danger";
    }
    QLabel *chip = createChip(status, tone, card);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);
    titleLayout->addWidget(name);
    titleLayout->addWidget(details);
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->addLayout(titleLayout, 1);
    topRow->addWidget(chip, 0, Qt::AlignTop);

    QProgressBar *bar = new QProgressBar(card);
    bar->setTextVisible(false);
    bar->setRange(0, 100);
    bar->setValue(progress.percent);
    if (tone != "info") {
        setTone(bar, tone);
    }
    bar->setAccessibleName(QString::number(progress.percent) + "% of " + name->text() + " saved");
    QLabel *percent = new QLabel(QString::number(progress.percent) + "% saved", card);
    percent->setObjectName("mutedText");

    // Saved, target and what is still needed.
    QLabel *savedValue = new QLabel(moneyText(code, goal.savedMinor), card);
    savedValue->setObjectName("incomeAmount");
    QLabel *targetValue = new QLabel(moneyText(code, goal.targetMinor), card);
    targetValue->setObjectName("amountText");
    QLabel *neededValue = new QLabel(moneyText(code, progress.remainingMinor), card);
    neededValue->setObjectName("amountText");
    QLabel *savedName = new QLabel("Saved", card);
    savedName->setObjectName("mutedText");
    QLabel *targetName = new QLabel("Target", card);
    targetName->setObjectName("mutedText");
    QLabel *neededName = new QLabel("Still needed", card);
    neededName->setObjectName("mutedText");

    QGridLayout *amounts = new QGridLayout();
    amounts->setVerticalSpacing(6);
    amounts->addWidget(savedName, 0, 0);
    amounts->addWidget(savedValue, 0, 1, Qt::AlignRight);
    amounts->addWidget(targetName, 1, 0);
    amounts->addWidget(targetValue, 1, 1, Qt::AlignRight);
    amounts->addWidget(neededName, 2, 0);
    amounts->addWidget(neededValue, 2, 1, Qt::AlignRight);

    // One sentence of advice, worked out in core/Goal.cpp.
    QString advice = "No target date: save at your own pace.";
    if (progress.reached) {
        advice = "Goal reached. Well done!";
    } else if (progress.overdue) {
        advice = "The target date has passed. Edit the goal to choose a new date.";
    } else if (progress.monthsLeft > 0) {
        advice = "Save about " + moneyText(code, progress.monthlyNeededMinor) + " a month to reach it by "
                 + goalDateText(goal.targetDate) + " (" + countText(progress.monthsLeft, "month", "months")
                 + ", counting this one).";
    }
    QLabel *adviceLabel = new QLabel(advice, card);
    adviceLabel->setObjectName("goalAdvice");
    adviceLabel->setWordWrap(true);

    // Each button remembers which goal it belongs to in a property; the
    // slot reads it back from sender(). History and Delete work on every
    // plan; the others show a lock below Finora Plus.
    QString planNeeded = QString::fromStdString(planName(FEATURES[FEATURE_SAVINGS_GOALS].lowestPlan));
    QString addMoneyStyle = "primaryButton";
    if (locked) {
        addMoneyStyle = "secondaryButton";   // the grey lock is easier to see on a light button
    }
    QPushButton *addMoney = createCardButton("Add money", addMoneyStyle, goal.id, card);
    QPushButton *takeOut = createCardButton("Take out", "secondaryButton", goal.id, card);
    QPushButton *history = createCardButton("History", "secondaryButton", goal.id, card);
    QPushButton *edit = createCardButton("Edit", "secondaryButton", goal.id, card);
    QPushButton *remove = createCardButton("Delete", "dangerButton", goal.id, card);
    addMoney->setAccessibleName("Add money to " + name->text());
    takeOut->setAccessibleName("Take money out of " + name->text());
    history->setAccessibleName("History of " + name->text());
    edit->setAccessibleName("Edit " + name->text());
    remove->setAccessibleName("Delete " + name->text());
    showLockIfNeeded(addMoney, locked, planNeeded);
    showLockIfNeeded(takeOut, locked, planNeeded);
    showLockIfNeeded(edit, locked, planNeeded);
    takeOut->setEnabled(goal.savedMinor > 0);   // nothing to take out yet
    connect(addMoney, &QPushButton::clicked, this, &GoalsPage::onAddMoneyClicked);
    connect(takeOut, &QPushButton::clicked, this, &GoalsPage::onTakeOutClicked);
    connect(history, &QPushButton::clicked, this, &GoalsPage::onHistoryClicked);
    connect(edit, &QPushButton::clicked, this, &GoalsPage::onEditClicked);
    connect(remove, &QPushButton::clicked, this, &GoalsPage::onDeleteClicked);

    QHBoxLayout *moneyButtons = new QHBoxLayout();
    moneyButtons->setSpacing(8);
    moneyButtons->addWidget(addMoney);
    moneyButtons->addWidget(takeOut);
    moneyButtons->addStretch();
    moneyButtons->addWidget(history);
    QHBoxLayout *goalButtons = new QHBoxLayout();
    goalButtons->setSpacing(8);
    goalButtons->addStretch();
    goalButtons->addWidget(edit);
    goalButtons->addWidget(remove);

    QFrame *divider = new QFrame(card);
    divider->setObjectName("divider");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 16);
    layout->setSpacing(10);
    layout->addLayout(topRow);
    layout->addWidget(bar);
    layout->addWidget(percent);
    layout->addWidget(divider);
    layout->addLayout(amounts);
    layout->addWidget(adviceLabel);
    layout->addLayout(moneyButtons);
    layout->addLayout(goalButtons);
    return card;
}

QPushButton *GoalsPage::createCardButton(const QString &text, const QString &style, int goalId, QWidget *parent)
{
    QPushButton *button = new QPushButton(text, parent);
    button->setObjectName(style);
    button->setProperty("goalId", goalId);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

// ---- Actions ---------------------------------------------------------------

bool GoalsPage::saveNewGoal(Goal &goal)
{
    if (!goalStore.addGoal(goal)) {
        showProblem("save the goal", goalStore.lastError());
        return false;
    }
    refresh();
    Toast::showMessage(this, "Goal created.");
    return true;
}

bool GoalsPage::saveEditedGoal(const Goal &goal)
{
    if (!goalStore.updateGoal(goal)) {
        showProblem("update the goal", goalStore.lastError());
        return false;
    }
    refresh();
    Toast::showMessage(this, "Goal updated.");
    return true;
}

bool GoalsPage::saveContribution(GoalContribution &contribution)
{
    if (!goalStore.addContribution(contribution)) {
        showProblem("save the money", goalStore.lastError());
        return false;
    }
    refresh();
    if (contribution.amountMinor > 0) {
        Toast::showMessage(this, "Money added to the goal.");
    } else {
        Toast::showMessage(this, "Money taken out of the goal.");
    }
    return true;
}

bool GoalsPage::deleteGoalById(int id)
{
    if (!goalStore.deleteGoal(id)) {
        showProblem("delete the goal", goalStore.lastError());
        return false;
    }
    refresh();
    Toast::showMessage(this, "Goal deleted.");
    return true;
}

const std::vector<GoalProgress> &GoalsPage::shownProgress() const
{
    return progressList;
}

bool GoalsPage::isLocked() const
{
    return locked;
}

void GoalsPage::onAddClicked()
{
    if (explainIfLocked()) {
        return;
    }
    GoalDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Goal newGoal = dialog.goal();
        saveNewGoal(newGoal);
    }
}

void GoalsPage::onAddMoneyClicked()
{
    openContributionDialog(false);
}

void GoalsPage::onTakeOutClicked()
{
    openContributionDialog(true);
}

void GoalsPage::openContributionDialog(bool takeOut)
{
    // sender() is the button that was clicked; its property says which goal.
    int id = sender()->property("goalId").toInt();
    Goal goal;
    if (!findGoal(id, goal) || explainIfLocked()) {
        return;
    }
    ContributionDialog dialog(goal, takeOut, this);
    if (dialog.exec() == QDialog::Accepted) {
        GoalContribution contribution = dialog.contribution();
        saveContribution(contribution);
    }
}

void GoalsPage::onHistoryClicked()
{
    int id = sender()->property("goalId").toInt();
    Goal goal;
    if (!findGoal(id, goal)) {
        return;
    }
    GoalHistoryDialog dialog(goalStore, goal, this);
    dialog.exec();
}

void GoalsPage::onEditClicked()
{
    int id = sender()->property("goalId").toInt();
    Goal goal;
    if (!findGoal(id, goal) || explainIfLocked()) {
        return;
    }
    GoalDialog dialog(goal, this);
    if (dialog.exec() == QDialog::Accepted) {
        saveEditedGoal(dialog.goal());
    }
}

void GoalsPage::onDeleteClicked()
{
    int id = sender()->property("goalId").toInt();
    Goal goal;
    if (!findGoal(id, goal)) {
        return;
    }
    // Ask first. "No" is the default button, so pressing Enter is safe.
    QString name = QString::fromStdString(goal.name) + " (" + moneyText(goal.currencyCode, goal.savedMinor)
                   + " saved)";
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, "Delete goal",
        "Delete the goal " + name + " and its history?\n\nYour transactions are not touched; Finora only "
        "stops following this goal.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::Yes) {
        deleteGoalById(id);
    }
}

void GoalsPage::onSeePlansClicked()
{
    emit openPageRequested(PAGE_SUBSCRIPTION);
}

// Below Finora Plus, explains the plans instead of opening a form and
// returns true. This only saves the person a wasted form: GoalStore
// checks the plan again before anything is saved, whatever screen asks.
bool GoalsPage::explainIfLocked()
{
    PlanId plan = database.currentPlan();
    if (planIncludes(plan, FEATURE_SAVINGS_GOALS)) {
        return false;
    }
    QString explanation = QString::fromStdString(lockedFeatureMessage(FEATURE_SAVINGS_GOALS, plan));
    if (askToSeePlans(this, "Savings goals", explanation)) {
        emit openPageRequested(PAGE_SUBSCRIPTION);
    }
    return true;
}

bool GoalsPage::findGoal(int id, Goal &result) const
{
    int count = static_cast<int>(goals.size());
    for (int i = 0; i < count; i++) {
        if (goals[i].id == id) {
            result = goals[i];
            return true;
        }
    }
    return false;
}

void GoalsPage::showProblem(const QString &action, const QString &reason)
{
    // A change refused by the plan gets the plans explained, not an error.
    if (goalStore.planBlocked()) {
        if (askToSeePlans(this, "Savings goals", reason)) {
            emit openPageRequested(PAGE_SUBSCRIPTION);
        }
        return;
    }
    QMessageBox::warning(this, "Finora", "Could not " + action + ".\n\nReason: " + reason);
}
