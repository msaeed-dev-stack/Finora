// =====================================================================
//  BudgetsPage.cpp
// =====================================================================
#include "BudgetsPage.h"

#include <QDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "BudgetDialog.h"
#include "PageHeader.h"
#include "Theme.h"
#include "UiHelpers.h"
#include "components/ProgressRing.h"
#include "components/Toast.h"
#include "core/Money.h"
#include "core/Totals.h"

const int CARD_COLUMNS = 3;   // budget cards per row

// The chip tone and ring colour for each status (see finora.qss).
QString toneForStatus(BudgetStatus status)
{
    switch (status) {
    case BUDGET_ON_TRACK:
        return "success";
    case BUDGET_WARNING:
        return "warning";
    case BUDGET_REACHED:
    case BUDGET_OVER:
        return "danger";
    }
    return "neutral";
}

BudgetsPage::BudgetsPage(DatabaseManager &sharedDatabase, BudgetStore &sharedBudgets, QWidget *parent)
    : Page(parent), database(sharedDatabase), budgetStore(sharedBudgets)
{
    setObjectName("budgetsPage");

    // Everything scrolls, so many budgets still fit on a small screen.
    QWidget *content = new QWidget(this);
    content->setObjectName("pageContent");

    summaryRow = new QWidget(content);
    summaryLayout = new QHBoxLayout(summaryRow);
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    summaryLayout->setSpacing(16);

    cardArea = new QWidget(content);
    cardGrid = new QGridLayout(cardArea);
    cardGrid->setContentsMargins(0, 0, 0, 0);
    cardGrid->setHorizontalSpacing(16);
    cardGrid->setVerticalSpacing(16);

    emptyState = createEmptyState();

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(20);
    contentLayout->addWidget(createHeader());
    contentLayout->addWidget(summaryRow);
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

QWidget *BudgetsPage::createHeader()
{
    QWidget *header = new QWidget(this);

    switcher = new MonthSwitcher(header);
    connect(switcher, &MonthSwitcher::monthChanged, this, &BudgetsPage::onMonthChanged);

    QPushButton *addButton = new QPushButton("+  New Budget", header);
    addButton->setObjectName("primaryButton");
    addButton->setAccessibleName("New Budget");
    addButton->setCursor(Qt::PointingHandCursor);
    connect(addButton, &QPushButton::clicked, this, &BudgetsPage::onAddClicked);

    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(new PageHeader("Budgets", "Monthly spending limits for each category", header));
    layout->addStretch();
    layout->addWidget(switcher);
    layout->addSpacing(8);
    layout->addWidget(addButton);
    return header;
}

QFrame *BudgetsPage::createEmptyState()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    emptyIcon = new QLabel(card);
    emptyIcon->setObjectName("emptyIcon");
    emptyIcon->setFixedSize(64, 64);
    emptyIcon->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("No budgets yet", card);
    title->setObjectName("emptyTitle");
    title->setAlignment(Qt::AlignCenter);

    QLabel *text = new QLabel("Set a monthly limit for a category such as Food or Transport. Finora adds up "
                              "your spending and warns you at 80% of the limit.", card);
    text->setObjectName("emptyText");
    text->setAlignment(Qt::AlignCenter);
    text->setWordWrap(true);

    QPushButton *createButton = new QPushButton("Create your first budget", card);
    createButton->setObjectName("primaryButton");
    createButton->setCursor(Qt::PointingHandCursor);
    connect(createButton, &QPushButton::clicked, this, &BudgetsPage::onAddClicked);

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

void BudgetsPage::refresh()
{
    if (!budgetStore.loadBudgets(budgets)) {
        showDatabaseError("load the budgets", budgetStore.lastError());
    }
    if (!database.loadTransactions(transactions)) {
        showDatabaseError("load the transactions", database.lastError());
    }
    // Icons are pictures, so they are coloured again for the current theme.
    switcher->refreshIcons();
    emptyIcon->setPixmap(tintedIcon("budgets", themeColors().primaryText, 30));
    showBudgets();
}

void BudgetsPage::showBudgets()
{
    progressList = allBudgetProgress(budgets, transactions, switcher->month().toStdString());

    clearLayout(cardGrid);
    clearLayout(summaryLayout);

    int count = static_cast<int>(progressList.size());
    for (int i = 0; i < count; i++) {
        // i / CARD_COLUMNS is the row and i % CARD_COLUMNS the column:
        // cards 0, 1, 2 fill the first row, 3, 4, 5 the second, and so on.
        cardGrid->addWidget(createBudgetCard(progressList[i]), i / CARD_COLUMNS, i % CARD_COLUMNS);
    }
    for (int column = 0; column < CARD_COLUMNS; column++) {
        cardGrid->setColumnStretch(column, 1);   // equal widths, also with fewer cards
    }

    // One summary per currency that has budgets. Currencies are never mixed.
    for (int c = 0; c < SUPPORTED_CURRENCY_COUNT; c++) {
        std::string code = SUPPORTED_CURRENCIES[c].code;
        bool used = false;
        for (int i = 0; i < count; i++) {
            if (progressList[i].budget.currencyCode == code) {
                used = true;
                break;
            }
        }
        if (used) {
            summaryLayout->addWidget(createCurrencySummary(code), 1);
        }
    }
    summaryLayout->addStretch();

    cardArea->setVisible(count > 0);
    summaryRow->setVisible(count > 0);
    emptyState->setVisible(count == 0);
}

// A card for one budget: ring, category, amounts, status and buttons.
QFrame *BudgetsPage::createBudgetCard(const BudgetProgress &progress)
{
    const Budget &budget = progress.budget;
    std::string code = budget.currencyCode;
    QString tone = toneForStatus(progress.status);

    QFrame *card = new QFrame(cardArea);
    card->setObjectName("card");
    card->setProperty("budgetId", budget.id);
    card->setAccessibleName(QString::fromStdString(budget.category) + " budget in "
                            + QString::fromStdString(code));

    ProgressRing *ring = new ProgressRing(76, card);
    ring->setProgress(progress.percentUsed, tone);

    QLabel *category = new QLabel(QString::fromStdString(budget.category), card);
    category->setObjectName("budgetCategory");
    QLabel *currency = new QLabel(QString::fromStdString(code) + " · per month", card);
    currency->setObjectName("mutedText");
    QLabel *chip = createChip(QString::fromStdString(budgetStatusName(progress.status)), tone, card);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);
    titleLayout->addWidget(category);
    titleLayout->addWidget(currency);
    titleLayout->addWidget(chip, 0, Qt::AlignLeft);

    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(14);
    topRow->addWidget(ring);
    topRow->addLayout(titleLayout, 1);

    // Spent, limit and what is left (or how much it is over).
    QLabel *spentValue = new QLabel(moneyText(code, progress.spentMinor), card);
    spentValue->setObjectName("amountText");
    QLabel *limitValue = new QLabel(moneyText(code, budget.limitMinor), card);
    limitValue->setObjectName("amountText");

    QLabel *leftName = new QLabel("Left", card);
    leftName->setObjectName("mutedText");
    QLabel *leftValue = new QLabel(card);
    if (progress.overflow) {
        spentValue->setText("Too large to show");
        leftValue->setText("-");
        leftValue->setObjectName("amountText");
    } else if (progress.remainingMinor < 0) {
        leftName->setText("Over by");
        leftValue->setText(moneyText(code, -progress.remainingMinor));
        leftValue->setObjectName("expenseAmount");
    } else if (progress.remainingMinor == 0) {
        leftValue->setText(moneyText(code, 0));
        leftValue->setObjectName("amountText");
    } else {
        leftValue->setText(moneyText(code, progress.remainingMinor));
        leftValue->setObjectName("incomeAmount");
    }

    QLabel *spentName = new QLabel("Spent in " + shortMonthName(switcher->month()), card);
    spentName->setObjectName("mutedText");
    QLabel *limitName = new QLabel("Monthly limit", card);
    limitName->setObjectName("mutedText");

    QGridLayout *amounts = new QGridLayout();
    amounts->setVerticalSpacing(6);
    amounts->addWidget(spentName, 0, 0);
    amounts->addWidget(spentValue, 0, 1, Qt::AlignRight);
    amounts->addWidget(limitName, 1, 0);
    amounts->addWidget(limitValue, 1, 1, Qt::AlignRight);
    amounts->addWidget(leftName, 2, 0);
    amounts->addWidget(leftValue, 2, 1, Qt::AlignRight);

    // Each button remembers which budget it belongs to in a property;
    // the slot reads it back from sender().
    QPushButton *editButton = new QPushButton("Edit", card);
    editButton->setObjectName("secondaryButton");
    editButton->setProperty("budgetId", budget.id);
    editButton->setAccessibleName("Edit " + category->text() + " budget");
    editButton->setCursor(Qt::PointingHandCursor);
    QPushButton *deleteButton = new QPushButton("Delete", card);
    deleteButton->setObjectName("dangerButton");
    deleteButton->setProperty("budgetId", budget.id);
    deleteButton->setAccessibleName("Delete " + category->text() + " budget");
    deleteButton->setCursor(Qt::PointingHandCursor);
    connect(editButton, &QPushButton::clicked, this, &BudgetsPage::onEditClicked);
    connect(deleteButton, &QPushButton::clicked, this, &BudgetsPage::onDeleteClicked);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->setSpacing(8);
    buttons->addStretch();
    buttons->addWidget(editButton);
    buttons->addWidget(deleteButton);

    QFrame *divider = new QFrame(card);
    divider->setObjectName("divider");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 16);
    layout->setSpacing(12);
    layout->addLayout(topRow);
    layout->addWidget(divider);
    layout->addLayout(amounts);
    layout->addLayout(buttons);
    return card;
}

// "PKR budgets: PKR 41,200.00 spent of PKR 60,000.00" for one currency.
QFrame *BudgetsPage::createCurrencySummary(const std::string &currencyCode)
{
    long long totalLimit = 0;
    long long totalSpent = 0;
    bool overflow = false;
    int budgetCount = 0;

    int count = static_cast<int>(progressList.size());
    for (int i = 0; i < count; i++) {
        const BudgetProgress &progress = progressList[i];
        if (progress.budget.currencyCode != currencyCode) {
            continue;
        }
        budgetCount++;
        if (progress.overflow || !addChecked(totalLimit, progress.budget.limitMinor)
            || !addChecked(totalSpent, progress.spentMinor)) {
            overflow = true;
        }
    }

    QFrame *card = new QFrame(summaryRow);
    card->setObjectName("card");

    QLabel *title = new QLabel(QString::fromStdString(currencyCode) + " budgets", card);
    title->setObjectName("cardTitle");
    QString countText = QString::number(budgetCount) + " budget";
    if (budgetCount != 1) {
        countText += "s";
    }
    QLabel *countLabel = new QLabel(countText + " · " + monthTitle(switcher->month()), card);
    countLabel->setObjectName("cardSubtitle");

    QLabel *amounts = new QLabel(card);
    amounts->setObjectName("amountText");
    QProgressBar *bar = new QProgressBar(card);
    bar->setTextVisible(false);
    bar->setRange(0, 100);

    if (overflow) {
        amounts->setText("Totals too large to show");
        bar->setValue(100);
        setTone(bar, "danger");
    } else {
        amounts->setText(moneyText(currencyCode, totalSpent) + " spent of " + moneyText(currencyCode, totalLimit));
        bar->setValue(qMin(100, percentUsed(totalSpent, totalLimit)));
        // The bar uses the same colours as the cards: teal, amber or rose.
        setTone(bar, toneForStatus(budgetStatusFor(totalSpent, totalLimit)));
    }
    bar->setAccessibleName(QString::number(percentUsed(totalSpent, totalLimit)) + "% of all "
                           + QString::fromStdString(currencyCode) + " budgets used");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 14, 18, 16);
    layout->setSpacing(6);
    layout->addWidget(title);
    layout->addWidget(countLabel);
    layout->addSpacing(4);
    layout->addWidget(amounts);
    layout->addWidget(bar);
    return card;
}

// ---- Actions ---------------------------------------------------------------

bool BudgetsPage::saveNewBudget(Budget &budget)
{
    if (!budgetStore.addBudget(budget)) {
        showDatabaseError("save the budget", budgetStore.lastError());
        return false;
    }
    refresh();
    Toast::showMessage(this, "Budget created.");
    return true;
}

bool BudgetsPage::saveEditedBudget(const Budget &budget)
{
    if (!budgetStore.updateBudget(budget)) {
        showDatabaseError("update the budget", budgetStore.lastError());
        return false;
    }
    refresh();
    Toast::showMessage(this, "Budget updated.");
    return true;
}

bool BudgetsPage::deleteBudgetById(int id)
{
    if (!budgetStore.deleteBudget(id)) {
        showDatabaseError("delete the budget", budgetStore.lastError());
        return false;
    }
    refresh();
    Toast::showMessage(this, "Budget deleted.");
    return true;
}

MonthSwitcher *BudgetsPage::monthSwitcher() const
{
    return switcher;
}

const std::vector<BudgetProgress> &BudgetsPage::shownProgress() const
{
    return progressList;
}

void BudgetsPage::onAddClicked()
{
    BudgetDialog dialog(budgets, this);
    if (dialog.exec() == QDialog::Accepted) {
        Budget newBudget = dialog.budget();
        saveNewBudget(newBudget);
    }
}

void BudgetsPage::onEditClicked()
{
    // sender() is the button that was clicked; its property says which budget.
    int id = sender()->property("budgetId").toInt();
    Budget existing;
    if (!findBudget(id, existing)) {
        return;
    }
    BudgetDialog dialog(existing, budgets, this);
    if (dialog.exec() == QDialog::Accepted) {
        saveEditedBudget(dialog.budget());
    }
}

void BudgetsPage::onDeleteClicked()
{
    int id = sender()->property("budgetId").toInt();
    Budget existing;
    if (!findBudget(id, existing)) {
        return;
    }

    // Ask first. "No" is the default button, so pressing Enter is safe.
    QString name = QString::fromStdString(existing.category) + " ("
                   + QString::fromStdString(formatMoney(existing.currencyCode, existing.limitMinor)) + " per month)";
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, "Delete budget",
        "Delete the budget " + name + "?\n\nYour transactions are not deleted; only the limit is removed.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::Yes) {
        deleteBudgetById(id);
    }
}

void BudgetsPage::onMonthChanged()
{
    showBudgets();
}

bool BudgetsPage::findBudget(int id, Budget &result) const
{
    int count = static_cast<int>(budgets.size());
    for (int i = 0; i < count; i++) {
        if (budgets[i].id == id) {
            result = budgets[i];
            return true;
        }
    }
    return false;
}

void BudgetsPage::showDatabaseError(const QString &action, const QString &reason)
{
    QMessageBox::warning(this, "Finora", "Could not " + action + ".\n\nReason: " + reason);
}
