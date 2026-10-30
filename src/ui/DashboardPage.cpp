// =====================================================================
//  DashboardPage.cpp
// =====================================================================
#include "DashboardPage.h"

#include <QAbstractButton>
#include <QDate>
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QTime>

#include "PageHeader.h"
#include "Sidebar.h"
#include "Theme.h"
#include "TransactionDialog.h"
#include "UiHelpers.h"
#include "components/ProgressRing.h"
#include "components/Toast.h"
#include "core/Greeting.h"
#include "core/Money.h"
#include "core/Totals.h"

const int RECENT_ROW_COUNT = 6;       // rows in "Recent transactions"
const int SPENDING_ROW_COUNT = 5;     // categories in "Spending by category"
const int ALERT_ROW_COUNT = 4;        // budgets in "Budget alerts"
const int RANGE_SIX_MONTHS = 6;
const int RANGE_TWELVE_MONTHS = 12;

DashboardPage::DashboardPage(DatabaseManager &sharedDatabase, BudgetStore &sharedBudgets, QWidget *parent)
    : Page(parent), database(sharedDatabase), budgetStore(sharedBudgets)
{
    setObjectName("dashboardPage");

    QWidget *content = new QWidget(this);
    content->setObjectName("pageContent");

    // Two rows of cards. The left card of each row is twice as wide.
    QHBoxLayout *middleRow = new QHBoxLayout();
    middleRow->setSpacing(20);
    middleRow->addWidget(createChartCard(), 2);
    middleRow->addWidget(createAlertsCard(), 1);

    QHBoxLayout *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(20);
    bottomRow->addWidget(createRecentCard(), 2);
    bottomRow->addWidget(createSpendingCard(), 1);

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(20);
    contentLayout->addWidget(createHeader());
    contentLayout->addWidget(createWalletRow());
    contentLayout->addLayout(middleRow);
    contentLayout->addLayout(bottomRow);
    contentLayout->addStretch();

    // The page scrolls when the window is too small for every card.
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

// ---- Building the page -------------------------------------------------

QWidget *DashboardPage::createHeader()
{
    QWidget *header = new QWidget(this);

    PageHeader *titleArea = new PageHeader("", "Here's your money at a glance", header);
    greetingLabel = titleArea->findChild<QLabel *>("pageTitle");

    switcher = new MonthSwitcher(header);
    connect(switcher, &MonthSwitcher::monthChanged, this, &DashboardPage::onMonthChanged);

    QPushButton *addButton = new QPushButton("+  Add Transaction", header);
    addButton->setObjectName("primaryButton");
    addButton->setAccessibleName("Add Transaction");
    addButton->setCursor(Qt::PointingHandCursor);
    connect(addButton, &QPushButton::clicked, this, &DashboardPage::onAddTransactionClicked);

    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(titleArea);
    layout->addStretch();
    layout->addWidget(switcher);
    layout->addSpacing(8);
    layout->addWidget(addButton);
    return header;
}

QWidget *DashboardPage::createWalletRow()
{
    QWidget *row = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // One card per currency, in the order of SUPPORTED_CURRENCIES.
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        WalletCard *card = new WalletCard(SUPPORTED_CURRENCIES[i], row);
        walletCards.push_back(card);
        layout->addWidget(card, 1);
    }
    return row;
}

// A white card with a title row (title, subtitle, optional link button)
// and an empty body layout that the caller fills.
QFrame *DashboardPage::createCardFrame(const QString &title, QLabel *&subtitleLabel, QPushButton *linkButton,
                                       QVBoxLayout *&bodyLayout)
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    QLabel *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("cardTitle");
    subtitleLabel = new QLabel(card);
    subtitleLabel->setObjectName("cardSubtitle");

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);

    QHBoxLayout *titleRow = new QHBoxLayout();
    titleRow->addLayout(titleLayout);
    titleRow->addStretch();
    if (linkButton != nullptr) {
        linkButton->setParent(card);
        titleRow->addWidget(linkButton, 0, Qt::AlignTop);
    }

    bodyLayout = new QVBoxLayout();
    bodyLayout->setSpacing(10);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(14);
    layout->addLayout(titleRow);
    layout->addLayout(bodyLayout, 1);
    return card;
}

QFrame *DashboardPage::createChartCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createCardFrame("Money flow", chartSubtitle, nullptr, body);

    // PKR | USD | EUR: the chart shows one currency at a time, because
    // Finora never adds different currencies together.
    QFrame *currencySwitch = new QFrame(card);
    currencySwitch->setObjectName("segmented");
    QHBoxLayout *currencyLayout = new QHBoxLayout(currencySwitch);
    currencyLayout->setContentsMargins(3, 3, 3, 3);
    currencyLayout->setSpacing(2);
    currencyGroup = new QButtonGroup(card);
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        QPushButton *button = new QPushButton(QString::fromUtf8(SUPPORTED_CURRENCIES[i].code), currencySwitch);
        button->setObjectName("segmentButton");
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setAccessibleName("Show " + button->text() + " in the chart");
        currencyGroup->addButton(button, i);
        currencyLayout->addWidget(button);
    }
    currencyGroup->button(0)->setChecked(true);

    QFrame *rangeSwitch = new QFrame(card);
    rangeSwitch->setObjectName("segmented");
    QHBoxLayout *rangeLayout = new QHBoxLayout(rangeSwitch);
    rangeLayout->setContentsMargins(3, 3, 3, 3);
    rangeLayout->setSpacing(2);
    rangeGroup = new QButtonGroup(card);
    QPushButton *sixMonths = new QPushButton("6 months", rangeSwitch);
    QPushButton *twelveMonths = new QPushButton("12 months", rangeSwitch);
    QPushButton *rangeButtons[2] = {sixMonths, twelveMonths};
    int rangeValues[2] = {RANGE_SIX_MONTHS, RANGE_TWELVE_MONTHS};
    for (int i = 0; i < 2; i++) {
        rangeButtons[i]->setObjectName("segmentButton");
        rangeButtons[i]->setCheckable(true);
        rangeButtons[i]->setCursor(Qt::PointingHandCursor);
        rangeGroup->addButton(rangeButtons[i], rangeValues[i]);   // the id is the number of months
        rangeLayout->addWidget(rangeButtons[i]);
    }
    sixMonths->setChecked(true);

    connect(currencyGroup, &QButtonGroup::idClicked, this, &DashboardPage::onChartOptionChanged);
    connect(rangeGroup, &QButtonGroup::idClicked, this, &DashboardPage::onChartOptionChanged);

    legendLabel = new QLabel(card);
    legendLabel->setObjectName("mutedText");

    QHBoxLayout *controls = new QHBoxLayout();
    controls->setSpacing(10);
    controls->addWidget(legendLabel);
    controls->addStretch();
    controls->addWidget(currencySwitch);
    controls->addWidget(rangeSwitch);

    flowChart = new MoneyFlowChart(card);
    flowChart->setMinimumHeight(250);

    chartSummary = new QLabel(card);
    chartSummary->setObjectName("mutedText");
    chartSummary->setWordWrap(true);
    connect(flowChart, &MoneyFlowChart::summaryChanged, chartSummary, &QLabel::setText);

    body->addLayout(controls);
    body->addWidget(flowChart, 1);
    body->addWidget(chartSummary);
    return card;
}

QFrame *DashboardPage::createAlertsCard()
{
    QPushButton *manage = new QPushButton("Manage");
    manage->setObjectName("linkButton");
    manage->setAccessibleName("Manage budgets");
    manage->setCursor(Qt::PointingHandCursor);
    connect(manage, &QPushButton::clicked, this, &DashboardPage::onManageBudgetsClicked);

    QVBoxLayout *body = nullptr;
    QFrame *card = createCardFrame("Budget alerts", alertsSubtitle, manage, body);
    card->setMinimumWidth(300);

    alertsLayout = new QVBoxLayout();
    alertsLayout->setSpacing(12);
    body->addLayout(alertsLayout);
    body->addStretch();
    return card;
}

QFrame *DashboardPage::createRecentCard()
{
    QPushButton *viewAll = new QPushButton("View all");
    viewAll->setObjectName("linkButton");
    viewAll->setAccessibleName("View all transactions");
    viewAll->setCursor(Qt::PointingHandCursor);
    connect(viewAll, &QPushButton::clicked, this, &DashboardPage::onViewAllClicked);

    QLabel *subtitle = nullptr;
    QVBoxLayout *body = nullptr;
    QFrame *card = createCardFrame("Recent transactions", subtitle, viewAll, body);
    subtitle->setText("The latest income and spending, every currency");

    recentTable = new QTableWidget(0, 4, card);
    recentTable->setObjectName("recentTable");
    recentTable->setHorizontalHeaderLabels({"Date", "Category", "Description", "Amount"});
    recentTable->setSelectionMode(QAbstractItemView::NoSelection);
    recentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recentTable->setFocusPolicy(Qt::NoFocus);
    recentTable->setShowGrid(false);
    recentTable->setAlternatingRowColors(true);
    recentTable->verticalHeader()->hide();
    recentTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    recentTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    recentTable->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    recentTable->setColumnWidth(0, 110);
    recentTable->setColumnWidth(1, 130);
    recentTable->setColumnWidth(3, 170);
    recentTable->setMinimumHeight(250);
    recentTable->setAccessibleName("Recent transactions");

    recentEmpty = new QLabel("No transactions yet. Use \"+ Add Transaction\" to record your first income "
                             "or expense.", card);
    recentEmpty->setObjectName("emptyText");
    recentEmpty->setAlignment(Qt::AlignCenter);
    recentEmpty->setWordWrap(true);
    recentEmpty->setMinimumHeight(120);

    body->addWidget(recentTable, 1);
    body->addWidget(recentEmpty, 1);
    return card;
}

QFrame *DashboardPage::createSpendingCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createCardFrame("Spending by category", spendingSubtitle, nullptr, body);
    card->setMinimumWidth(300);

    spendingLayout = new QVBoxLayout();
    spendingLayout->setSpacing(12);
    body->addLayout(spendingLayout);
    body->addStretch();
    return card;
}

// ---- Showing the data -------------------------------------------------

void DashboardPage::refresh()
{
    if (!database.loadTransactions(transactions)) {
        QMessageBox::warning(this, "Finora", "Could not load the transactions.\n\nReason: " + database.lastError());
    }
    if (!budgetStore.loadBudgets(budgets)) {
        QMessageBox::warning(this, "Finora", "Could not load the budgets.\n\nReason: " + budgetStore.lastError());
    }

    // The greeting follows the time of day. Phase 4B adds the user's name.
    greetingLabel->setText(QString::fromStdString(greetingForHour(QTime::currentTime().hour())));

    switcher->refreshIcons();
    updateLegend();
    showWallets();
    showChart();
    showBudgetAlerts();
    showRecentTransactions();
    showSpending();
}

void DashboardPage::showWallets()
{
    std::vector<CurrencySummary> summaries =
        summarizeByCurrency(transactions, switcher->month().toStdString());
    QString monthName = QDate::fromString(switcher->month() + "-01", "yyyy-MM-dd").toString("MMMM");

    int count = static_cast<int>(walletCards.size());
    for (int i = 0; i < count && i < static_cast<int>(summaries.size()); i++) {
        walletCards[i]->showSummary(summaries[i], monthName);
    }
}

void DashboardPage::showChart()
{
    QString code = chartCurrency();
    int months = rangeGroup->checkedId();
    std::vector<MonthFlow> flows = monthlyFlow(transactions, code.toStdString(),
                                               switcher->month().toStdString(), months);
    chartSubtitle->setText("Income and spending per month in " + code + ", last " + QString::number(months)
                           + " months");
    flowChart->setData(flows, code);
}

void DashboardPage::showBudgetAlerts()
{
    clearLayout(alertsLayout);
    alertsSubtitle->setText(monthTitle(switcher->month()));

    std::vector<BudgetProgress> progress = allBudgetProgress(budgets, transactions,
                                                             switcher->month().toStdString());

    // Collect the budgets that need attention, the most used first.
    std::vector<BudgetProgress> alerts;
    int count = static_cast<int>(progress.size());
    for (int i = 0; i < count; i++) {
        if (progress[i].status != BUDGET_ON_TRACK) {
            alerts.push_back(progress[i]);
        }
    }
    // A simple selection sort: put the largest percentage at the front.
    int alertTotal = static_cast<int>(alerts.size());
    for (int i = 0; i < alertTotal; i++) {
        int largest = i;
        for (int j = i + 1; j < alertTotal; j++) {
            if (alerts[j].percentUsed > alerts[largest].percentUsed) {
                largest = j;
            }
        }
        if (largest != i) {
            BudgetProgress swap = alerts[i];
            alerts[i] = alerts[largest];
            alerts[largest] = swap;
        }
    }
    alertCount = alertTotal;

    QWidget *card = alertsSubtitle->parentWidget();
    if (budgets.empty()) {
        QLabel *text = new QLabel("No budgets yet. A budget warns you before you spend too much on a category.",
                                  card);
        text->setObjectName("emptyText");
        text->setWordWrap(true);
        QPushButton *create = new QPushButton("Create a budget", card);
        create->setObjectName("secondaryButton");
        create->setCursor(Qt::PointingHandCursor);
        connect(create, &QPushButton::clicked, this, &DashboardPage::onManageBudgetsClicked);
        alertsLayout->addWidget(text);
        alertsLayout->addWidget(create, 0, Qt::AlignLeft);
        return;
    }

    if (alerts.empty()) {
        QLabel *icon = new QLabel(card);
        icon->setPixmap(tintedIcon("check_circle", themeColors().income, 22));
        QString countText = "Your budget is on track";
        if (budgets.size() > 1) {
            countText = "All " + QString::number(budgets.size()) + " budgets are on track";
        }
        QLabel *text = new QLabel(countText + " this month.", card);
        text->setObjectName("bodyText");
        text->setWordWrap(true);
        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(10);
        row->addWidget(icon);
        row->addWidget(text, 1);
        alertsLayout->addLayout(row);
        return;
    }

    for (int i = 0; i < alertTotal && i < ALERT_ROW_COUNT; i++) {
        const BudgetProgress &alert = alerts[i];
        std::string code = alert.budget.currencyCode;
        QString tone = "warning";
        if (alert.status == BUDGET_REACHED || alert.status == BUDGET_OVER) {
            tone = "danger";
        }

        ProgressRing *ring = new ProgressRing(44, card);
        ring->setProgress(alert.percentUsed, tone);

        QLabel *name = new QLabel(QString::fromStdString(alert.budget.category) + "  ·  "
                                  + QString::fromStdString(code), card);
        name->setObjectName("bodyText");
        QLabel *amounts = new QLabel(moneyText(code, alert.spentMinor) + " of "
                                     + moneyText(code, alert.budget.limitMinor), card);
        amounts->setObjectName("mutedText");

        // First line: name and status chip. Second line: the amounts.
        QHBoxLayout *firstLine = new QHBoxLayout();
        firstLine->setSpacing(8);
        firstLine->addWidget(name);
        firstLine->addStretch();
        firstLine->addWidget(createChip(QString::fromStdString(budgetStatusName(alert.status)), tone, card));

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(3);
        textLayout->addLayout(firstLine);
        textLayout->addWidget(amounts);

        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(12);
        row->addWidget(ring);
        row->addLayout(textLayout, 1);
        alertsLayout->addLayout(row);
    }
    if (alertTotal > ALERT_ROW_COUNT) {
        QLabel *more = new QLabel("+ " + QString::number(alertTotal - ALERT_ROW_COUNT) + " more on the Budgets page",
                                  card);
        more->setObjectName("mutedText");
        alertsLayout->addWidget(more);
    }
}

void DashboardPage::showRecentTransactions()
{
    // loadTransactions() returns the newest first, so the first rows are the latest.
    int rowCount = qMin(RECENT_ROW_COUNT, static_cast<int>(transactions.size()));
    recentTable->clearContents();
    recentTable->setRowCount(rowCount);

    const ThemeColors &colors = themeColors();
    for (int row = 0; row < rowCount; row++) {
        const Transaction &transaction = transactions[row];
        bool isIncome = (transaction.type == TYPE_INCOME);

        QDate date = QDate::fromString(QString::fromStdString(transaction.date), "yyyy-MM-dd");
        QTableWidgetItem *dateItem = new QTableWidgetItem(date.toString("dd MMM yyyy"));
        QTableWidgetItem *categoryItem = new QTableWidgetItem(QString::fromStdString(transaction.category));
        QTableWidgetItem *descriptionItem = new QTableWidgetItem(QString::fromStdString(transaction.description));

        // A sign in front of the amount repeats what the colour shows.
        long long signedAmount = transaction.amountMinor;
        if (!isIncome) {
            signedAmount = -signedAmount;
        }
        QTableWidgetItem *amountItem = new QTableWidgetItem(
            QString::fromStdString(formatSignedMoney(transaction.currencyCode, signedAmount)));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (isIncome) {
            amountItem->setForeground(colors.income);
        } else {
            amountItem->setForeground(colors.expenseText);
        }

        recentTable->setItem(row, 0, dateItem);
        recentTable->setItem(row, 1, categoryItem);
        recentTable->setItem(row, 2, descriptionItem);
        recentTable->setItem(row, 3, amountItem);
    }

    recentTable->setVisible(rowCount > 0);
    recentEmpty->setVisible(rowCount == 0);
}

void DashboardPage::showSpending()
{
    clearLayout(spendingLayout);
    QString code = chartCurrency();
    spendingSubtitle->setText(monthTitle(switcher->month()) + " · " + code);

    bool overflow = false;
    std::vector<CategoryTotal> totals = spendingByCategory(transactions, code.toStdString(),
                                                           switcher->month().toStdString(), overflow);
    QWidget *card = spendingSubtitle->parentWidget();

    // The month's total spending, for each category's share in percent.
    long long monthTotal = 0;
    int count = static_cast<int>(totals.size());
    for (int i = 0; i < count; i++) {
        if (!addChecked(monthTotal, totals[i].amountMinor)) {
            overflow = true;
        }
    }

    if (overflow) {
        QLabel *text = new QLabel("These totals are too large to show.", card);
        text->setObjectName("emptyText");
        spendingLayout->addWidget(text);
        return;
    }
    if (totals.empty()) {
        QLabel *text = new QLabel("No " + code + " spending in " + monthTitle(switcher->month()) + ".", card);
        text->setObjectName("emptyText");
        text->setWordWrap(true);
        spendingLayout->addWidget(text);
        return;
    }

    for (int i = 0; i < count && i < SPENDING_ROW_COUNT; i++) {
        int share = percentUsed(totals[i].amountMinor, monthTotal);

        QLabel *name = new QLabel(QString::fromStdString(totals[i].category), card);
        name->setObjectName("bodyText");
        QLabel *amount = new QLabel(moneyText(code.toStdString(), totals[i].amountMinor) + "  ·  "
                                    + QString::number(share) + "%", card);
        amount->setObjectName("mutedText");

        QHBoxLayout *textRow = new QHBoxLayout();
        textRow->addWidget(name);
        textRow->addStretch();
        textRow->addWidget(amount);

        QProgressBar *bar = new QProgressBar(card);
        bar->setRange(0, 100);
        bar->setValue(share);
        bar->setTextVisible(false);
        bar->setAccessibleName(name->text() + ": " + QString::number(share) + "% of spending");

        QVBoxLayout *rowLayout = new QVBoxLayout();
        rowLayout->setSpacing(5);
        rowLayout->addLayout(textRow);
        rowLayout->addWidget(bar);
        spendingLayout->addLayout(rowLayout);
    }
    if (count > SPENDING_ROW_COUNT) {
        QLabel *more = new QLabel("+ " + QString::number(count - SPENDING_ROW_COUNT) + " more categories", card);
        more->setObjectName("mutedText");
        spendingLayout->addWidget(more);
    }
}

// "● Income   ● Spent" in the chart's colours. The words carry the
// meaning; the dots only match the bars.
void DashboardPage::updateLegend()
{
    const ThemeColors &colors = themeColors();
    legendLabel->setText("<span style=\"color:" + colors.income.name() + "\">&#9679;</span> Income"
                         "&nbsp;&nbsp;&nbsp;<span style=\"color:" + colors.expense.name()
                         + "\">&#9679;</span> Spent");
}

// ---- Actions ------------------------------------------------------------

bool DashboardPage::saveNewTransaction(Transaction &transaction)
{
    if (!database.addTransaction(transaction)) {
        QMessageBox::warning(this, "Finora", "Could not save the transaction.\n\nReason: " + database.lastError());
        return false;
    }
    refresh();
    Toast::showMessage(this, "Transaction added.");
    return true;
}

void DashboardPage::onAddTransactionClicked()
{
    TransactionDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Transaction newTransaction = dialog.transaction();
        saveNewTransaction(newTransaction);
    }
}

void DashboardPage::onViewAllClicked()
{
    emit openPageRequested(PAGE_TRANSACTIONS);
}

void DashboardPage::onManageBudgetsClicked()
{
    emit openPageRequested(PAGE_BUDGETS);
}

void DashboardPage::onChartOptionChanged()
{
    showChart();
    showSpending();   // the spending card follows the chart's currency
}

void DashboardPage::onMonthChanged()
{
    showWallets();
    showChart();
    showBudgetAlerts();
    showSpending();
}

// ---- For the tests -------------------------------------------------------

MonthSwitcher *DashboardPage::monthSwitcher() const
{
    return switcher;
}

MoneyFlowChart *DashboardPage::chart() const
{
    return flowChart;
}

WalletCard *DashboardPage::walletCard(const QString &currencyCode) const
{
    int count = static_cast<int>(walletCards.size());
    for (int i = 0; i < count; i++) {
        if (walletCards[i]->currencyCode() == currencyCode) {
            return walletCards[i];
        }
    }
    return nullptr;
}

QString DashboardPage::chartCurrency() const
{
    return currencyGroup->checkedButton()->text();
}

void DashboardPage::setChartCurrency(const QString &currencyCode)
{
    QList<QAbstractButton *> buttons = currencyGroup->buttons();
    for (int i = 0; i < buttons.size(); i++) {
        if (buttons[i]->text() == currencyCode) {
            buttons[i]->setChecked(true);
        }
    }
    onChartOptionChanged();
}

int DashboardPage::budgetAlertCount() const
{
    return alertCount;
}
