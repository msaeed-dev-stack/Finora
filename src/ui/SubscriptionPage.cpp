// =====================================================================
//  SubscriptionPage.cpp
// =====================================================================
#include "SubscriptionPage.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollArea>
#include <QStyle>

#include "CheckoutDialog.h"
#include "PageHeader.h"
#include "Sidebar.h"   // PAGE_BUDGETS
#include "Theme.h"
#include "UiHelpers.h"
#include "components/Toast.h"
#include "core/Money.h"

const int TABLE_ROW_HEIGHT = 30;
const int TABLE_HEADER_HEIGHT = 34;
const int HISTORY_VISIBLE_ROWS = 8;   // more payments than this scroll inside the table
const int WIDE_PAGE_WIDTH = 1000;     // from this page width on, four plan cards fit side by side

// What one plan offers for one feature, as the comparison table shows it.
QString comparisonCell(Feature feature, PlanId plan)
{
    if (feature == FEATURE_BUDGETS) {
        int limit = activeBudgetLimit(plan);
        if (limit == UNLIMITED_BUDGETS) {
            return "Unlimited";
        }
        return "Up to " + QString::number(limit);
    }
    if (!planIncludes(plan, feature)) {
        return QString::fromUtf8("—");
    }
    if (FEATURES[feature].state == FEATURE_PLANNED) {
        return "Soon";
    }
    if (FEATURES[feature].state == FEATURE_NEEDS_ONLINE) {
        return "Online";
    }
    return QString::fromUtf8("✓");
}

// Words for the billing history.
QString paymentReasonText(const std::string &reason)
{
    if (reason == "upgrade") {
        return "Upgrade";
    }
    if (reason == "renewal") {
        return "Renewal";
    }
    return "New subscription";
}

QString paymentStatusText(const std::string &status)
{
    if (status == "succeeded") {
        return "Paid (simulated)";
    }
    if (status == "failed") {
        return "Declined (simulated)";
    }
    return "Pending";
}

// "$12.99"
QString dollarsText(long long cents)
{
    return QString::fromStdString("$" + formatAmount(cents));
}

// Marks a plan card as the person's own plan; finora.qss gives it a violet edge.
void setCurrentCard(QFrame *card, bool current)
{
    card->setProperty("current", current);
    card->style()->unpolish(card);   // Qt reads [current="true"] only when it styles the card
    card->style()->polish(card);
}

// Gives a button the look of another style-sheet name, for example
// "primaryButton" for a paid choice and "secondaryButton" for the others.
void setButtonStyle(QPushButton *button, const QString &styleName)
{
    if (button->objectName() == styleName) {
        return;
    }
    button->setObjectName(styleName);
    button->style()->unpolish(button);
    button->style()->polish(button);
}

// A read-only table in Finora's style, with the given column titles.
QTableWidget *createReadOnlyTable(const QString &objectName, const QStringList &headers, QWidget *parent)
{
    QTableWidget *table = new QTableWidget(parent);
    table->setObjectName(objectName);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(TABLE_ROW_HEIGHT);
    table->horizontalHeader()->setFixedHeight(TABLE_HEADER_HEIGHT);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setFrameShape(QFrame::NoFrame);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return table;
}

SubscriptionPage::SubscriptionPage(DatabaseManager &sharedDatabase, BudgetStore &sharedBudgets,
                                   SubscriptionService *sharedService, QWidget *parent)
    : Page(parent), database(sharedDatabase), budgetStore(sharedBudgets), store(sharedDatabase),
      service(sharedService)
{
    setObjectName("subscriptionPage");

    QWidget *content = new QWidget(this);
    content->setObjectName("pageContent");

    QLabel *banner = new QLabel("Simulated billing: Finora collects no card details and never charges real "
                                "money. Payments on this page only show how a subscription works.", content);
    banner->setObjectName("infoBanner");
    banner->setWordWrap(true);

    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(16);
    topRow->addWidget(createCurrentPlanCard(), 2);
    topRow->addWidget(createUsageCard(), 1);

    QLabel *plansTitle = new QLabel("Plans", content);
    plansTitle->setObjectName("cardTitle");

    planArea = new QWidget(content);
    planGrid = new QGridLayout(planArea);
    planGrid->setContentsMargins(0, 0, 0, 0);
    planGrid->setHorizontalSpacing(16);
    planGrid->setVerticalSpacing(16);
    for (int i = 0; i < PLAN_COUNT; i++) {
        planCards[i] = createPlanCard(static_cast<PlanId>(i));
    }
    arrangePlanCards(4);

    QLabel *footnote = new QLabel("Prices are proposed for this project, in US dollars per month, and have not "
                                  "been tested on the market.", content);
    footnote->setObjectName("mutedText");
    footnote->setWordWrap(true);

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(20);
    contentLayout->addWidget(createHeader());
    contentLayout->addWidget(banner);
    contentLayout->addLayout(topRow);
    contentLayout->addWidget(plansTitle);
    contentLayout->addWidget(planArea);
    contentLayout->addWidget(footnote);
    contentLayout->addWidget(createComparisonCard());
    contentLayout->addWidget(createHistoryCard());
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

// ---- Building the page ------------------------------------------------------

QWidget *SubscriptionPage::createHeader()
{
    return new PageHeader("Subscription", "Your plan, your billing history and what each plan includes", this);
}

QFrame *SubscriptionPage::createCurrentPlanCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    QLabel *title = new QLabel("Your plan", card);
    title->setObjectName("cardSubtitle");
    currentName = new QLabel(card);
    currentName->setObjectName("planHeroName");
    currentStatus = createChip("", "neutral", card);
    currentPrice = new QLabel(card);
    currentPrice->setObjectName("mutedText");

    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->setSpacing(10);
    nameRow->addWidget(currentName);
    nameRow->addWidget(currentStatus, 0, Qt::AlignVCenter);
    nameRow->addStretch();
    nameRow->addWidget(currentPrice, 0, Qt::AlignVCenter);

    currentDetails = new QLabel(card);
    currentDetails->setObjectName("bodyText");
    currentDetails->setWordWrap(true);
    lastPaymentLabel = new QLabel(card);
    lastPaymentLabel->setObjectName("mutedText");
    lastPaymentLabel->setWordWrap(true);

    // The buttons depend on the status; showActions() builds them.
    actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(8);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 18, 22, 20);
    layout->setSpacing(8);
    layout->addWidget(title);
    layout->addLayout(nameRow);
    layout->addWidget(currentDetails);
    layout->addWidget(lastPaymentLabel);
    layout->addStretch();
    layout->addLayout(actionsLayout);
    return card;
}

QFrame *SubscriptionPage::createUsageCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    QLabel *title = new QLabel("Active budgets", card);
    title->setObjectName("cardSubtitle");
    usageNumber = new QLabel(card);
    usageNumber->setObjectName("usageNumber");
    usageBar = new QProgressBar(card);
    usageBar->setTextVisible(false);
    usageText = new QLabel(card);
    usageText->setObjectName("mutedText");
    usageText->setWordWrap(true);

    QPushButton *manage = new QPushButton("Manage budgets", card);
    manage->setObjectName("linkButton");
    manage->setCursor(Qt::PointingHandCursor);
    connect(manage, &QPushButton::clicked, this, &SubscriptionPage::onManageBudgetsClicked);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 18, 22, 16);
    layout->setSpacing(8);
    layout->addWidget(title);
    layout->addWidget(usageNumber);
    layout->addWidget(usageBar);
    layout->addWidget(usageText);
    layout->addStretch();
    layout->addWidget(manage, 0, Qt::AlignLeft);
    return card;
}

QFrame *SubscriptionPage::createPlanCard(PlanId plan)
{
    QFrame *card = new QFrame(planArea);
    card->setObjectName("planCard");
    QString name = QString::fromStdString(planName(plan));
    QString price = QString::fromStdString(formatPlanPrice(plan));
    card->setAccessibleName(name + ", " + price + " per month");

    QLabel *nameLabel = new QLabel(name, card);
    nameLabel->setObjectName("planName");
    currentBadges[plan] = createChip("Current plan", "info", card);
    currentBadges[plan]->hide();
    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->setSpacing(8);
    nameRow->addWidget(nameLabel);
    nameRow->addStretch();
    nameRow->addWidget(currentBadges[plan], 0, Qt::AlignVCenter);

    QLabel *priceLabel = new QLabel(price, card);
    priceLabel->setObjectName("planPrice");
    QLabel *period = new QLabel("per month", card);
    period->setObjectName("mutedText");
    QHBoxLayout *priceRow = new QHBoxLayout();
    priceRow->setSpacing(6);
    priceRow->addWidget(priceLabel);
    priceRow->addWidget(period, 0, Qt::AlignBottom);
    priceRow->addStretch();

    QLabel *summary = new QLabel(QString::fromUtf8(PLANS[plan].summary), card);
    summary->setObjectName("cardSubtitle");
    summary->setWordWrap(true);

    QFrame *divider = new QFrame(card);
    divider->setObjectName("divider");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(8);
    layout->addLayout(nameRow);
    layout->addLayout(priceRow);
    layout->addWidget(summary);
    layout->addWidget(divider);

    // A larger plan lists only what it adds to the plan before it.
    if (plan > PLAN_FREE) {
        PlanId smaller = static_cast<PlanId>(plan - 1);
        QLabel *everything = new QLabel("Everything in " + QString::fromStdString(planName(smaller)) + ", plus:", card);
        everything->setObjectName("mutedText");
        everything->setWordWrap(true);
        layout->addWidget(everything);
    }
    for (int f = 0; f < FEATURE_COUNT; f++) {
        if (FEATURES[f].lowestPlan == plan) {
            addFeatureLine(layout, card, static_cast<Feature>(f));
        }
    }
    if (plan == PLAN_BUSINESS) {
        QLabel *note = new QLabel("Team members, shared workspaces, roles and team reports need Finora Online, "
                                  "which is planned. They do not work in this desktop version.", card);
        note->setObjectName("mutedText");
        note->setWordWrap(true);
        layout->addSpacing(4);
        layout->addWidget(note);
    }
    layout->addStretch();

    // One action per card. Its words and look are set by showPlanCards();
    // the button remembers its plan in a property, read back by the slot.
    planButtons[plan] = new QPushButton(card);
    planButtons[plan]->setObjectName("secondaryButton");
    planButtons[plan]->setProperty("plan", static_cast<int>(plan));
    planButtons[plan]->setCursor(Qt::PointingHandCursor);
    connect(planButtons[plan], &QPushButton::clicked, this, &SubscriptionPage::onPlanButtonClicked);
    layout->addSpacing(4);
    layout->addWidget(planButtons[plan]);

    if (plan == PLAN_PRO) {
        trialButton = new QPushButton("Try Pro free for 14 days", card);
        trialButton->setObjectName("linkButton");
        trialButton->setCursor(Qt::PointingHandCursor);
        connect(trialButton, &QPushButton::clicked, this, &SubscriptionPage::onTrialClicked);
        layout->addWidget(trialButton, 0, Qt::AlignHCenter);
    }
    return card;
}

// One line of a plan card: a mark, the feature's name and, for a feature
// that is not built yet, "Soon" or "Online". A green tick is shown only
// for features that really work.
void SubscriptionPage::addFeatureLine(QVBoxLayout *layout, QWidget *card, Feature feature)
{
    QLabel *mark = new QLabel(card);
    mark->setFixedWidth(16);
    QLabel *text = new QLabel(QString::fromStdString(featureName(feature)), card);
    text->setObjectName("featureText");
    text->setWordWrap(true);

    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(8);
    row->addWidget(mark, 0, Qt::AlignTop);
    row->addWidget(text, 1);

    if (FEATURES[feature].state == FEATURE_AVAILABLE) {
        mark->setText(QString::fromUtf8("✓"));
        mark->setObjectName("featureMark");
        text->setAccessibleName(text->text() + ", available");
    } else {
        mark->setText(QString::fromUtf8("○"));
        mark->setObjectName("featureMarkPlanned");
        QLabel *chip = nullptr;
        if (FEATURES[feature].state == FEATURE_PLANNED) {
            chip = createChip("Soon", "neutral", card);
            chip->setToolTip("Planned: not built yet");
            text->setAccessibleName(text->text() + ", planned, not built yet");
        } else {
            chip = createChip("Online", "neutral", card);
            chip->setToolTip("Needs Finora Online, which is planned");
            text->setAccessibleName(text->text() + ", needs Finora Online, planned");
        }
        row->addWidget(chip, 0, Qt::AlignTop);
    }
    layout->addLayout(row);
}

QFrame *SubscriptionPage::createComparisonCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    QLabel *title = new QLabel("Compare every feature", card);
    title->setObjectName("cardTitle");
    QLabel *legend = new QLabel(QString::fromUtf8("✓ included   ·   Soon: planned, not built yet   ·   "
                                                  "Online: needs Finora Online (planned)   ·   — not included"),
                                card);
    legend->setObjectName("cardSubtitle");
    legend->setWordWrap(true);

    comparisonButton = new QPushButton("Show the comparison", card);
    comparisonButton->setObjectName("linkButton");
    comparisonButton->setCursor(Qt::PointingHandCursor);
    connect(comparisonButton, &QPushButton::clicked, this, &SubscriptionPage::onToggleComparisonClicked);

    QHBoxLayout *titleRow = new QHBoxLayout();
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(comparisonButton);

    // One row per feature; both budget features share the "Active budgets" row.
    QStringList headers;
    headers << "Feature";
    for (int p = 0; p < PLAN_COUNT; p++) {
        headers << shortPlanName(static_cast<PlanId>(p));
    }
    comparisonTable = createReadOnlyTable("featureTable", headers, card);
    comparisonTable->setRowCount(FEATURE_COUNT - 1);
    int row = 0;
    for (int f = 0; f < FEATURE_COUNT; f++) {
        Feature feature = static_cast<Feature>(f);
        if (feature == FEATURE_UNLIMITED_BUDGETS) {
            continue;   // shown in the "Active budgets" row
        }
        QString name = QString::fromStdString(featureName(feature));
        if (feature == FEATURE_BUDGETS) {
            name = "Active budgets";
        }
        comparisonTable->setItem(row, 0, new QTableWidgetItem(name));
        for (int p = 0; p < PLAN_COUNT; p++) {
            QTableWidgetItem *item = new QTableWidgetItem(comparisonCell(feature, static_cast<PlanId>(p)));
            item->setTextAlignment(Qt::AlignCenter);
            comparisonTable->setItem(row, p + 1, item);
        }
        row++;
    }
    comparisonTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int p = 0; p < PLAN_COUNT; p++) {
        comparisonTable->horizontalHeader()->setSectionResizeMode(p + 1, QHeaderView::Fixed);
        comparisonTable->setColumnWidth(p + 1, 96);
    }
    // Tall enough for every row, so only the page scrolls.
    comparisonTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    comparisonTable->setFixedHeight(TABLE_HEADER_HEIGHT + comparisonTable->rowCount() * TABLE_ROW_HEIGHT + 2);
    comparisonTable->hide();

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 18, 22, 18);
    layout->setSpacing(8);
    layout->addLayout(titleRow);
    layout->addWidget(legend);
    layout->addWidget(comparisonTable);
    return card;
}

QFrame *SubscriptionPage::createHistoryCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    QLabel *title = new QLabel("Billing history", card);
    title->setObjectName("cardTitle");
    QLabel *subtitle = new QLabel("Every simulated payment, newest first. No real money was charged.", card);
    subtitle->setObjectName("cardSubtitle");
    subtitle->setWordWrap(true);

    historyTable = createReadOnlyTable("historyTable",
                                       {"Date", "Plan", "Reason", "Amount", "Status", "Reference"}, card);
    for (int column = 0; column < 5; column++) {
        historyTable->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }
    historyTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    historyEmpty = new QLabel("No payments yet. Finora Free costs nothing; simulated payments will appear here.",
                              card);
    historyEmpty->setObjectName("emptyText");
    historyEmpty->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 18, 22, 18);
    layout->setSpacing(8);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(historyTable);
    layout->addWidget(historyEmpty);
    return card;
}

// ---- Loading and showing ----------------------------------------------------

void SubscriptionPage::refresh()
{
    // Nothing here shows a message box: a page that cannot load its data
    // says so on the page itself.
    Subscription stored;
    signedIn = store.loadSubscription(stored);
    if (signedIn) {
        subscription = subscriptionAt(stored, currentUtcTime());   // every passed deadline applied
        if (!store.loadPayments(payments)) {
            payments.clear();
        }
    } else {
        subscription = Subscription();
        payments.clear();
    }
    if (!budgetStore.loadBudgets(budgets)) {
        budgets.clear();
    }

    showCurrentPlan();
    showUsage();
    showPlanCards();
    showHistory();
}

void SubscriptionPage::showCurrentPlan()
{
    showActions();
    if (!signedIn) {
        currentName->setText("No account");
        currentStatus->hide();
        currentPrice->clear();
        currentDetails->setText("Plans belong to accounts. Sign in to see and change your plan.");
        lastPaymentLabel->hide();
        return;
    }

    currentName->setText(QString::fromStdString(planName(subscription.plan)));
    currentStatus->setText(QString::fromStdString(statusName(subscription.status)));
    setTone(currentStatus, planChipTone(subscription));
    currentStatus->show();
    currentPrice->setText(QString::fromStdString(formatPlanPrice(subscription.plan)) + " / month");
    currentDetails->setText(statusDetails());

    if (subscription.lastPaymentReference.empty()) {
        lastPaymentLabel->hide();
    } else {
        QString result = "paid";
        if (subscription.lastPaymentStatus == "failed") {
            result = "declined";
        }
        lastPaymentLabel->setText(QString::fromUtf8("Last simulated payment: %1 · reference %2")
                                      .arg(result, QString::fromStdString(subscription.lastPaymentReference)));
        lastPaymentLabel->show();
    }
}

// What the person's status means, in a sentence or two.
QString SubscriptionPage::statusDetails() const
{
    QString name = QString::fromStdString(planName(subscription.plan));
    switch (subscription.status) {
    case STATUS_TRIAL:
        return "Your free Pro trial ends on " + dateText(subscription.expiresAt) + " ("
               + countText(daysUntil(subscription.expiresAt), "day", "days")
               + " left). Choose a plan before then to keep Pro; otherwise Finora Free applies again "
                 "and nothing is deleted.";
    case STATUS_ACTIVE: {
        QString text = "Renews on " + dateText(subscription.renewsAt) + ".";
        if (subscription.hasScheduledPlan) {
            text += " " + QString::fromStdString(planName(subscription.scheduledPlan)) + " starts on that date.";
        }
        return text;
    }
    case STATUS_PAST_DUE:
        return "The renewal was due on " + dateText(subscription.renewsAt) + ". " + name
               + " stays until " + dateText(subscription.expiresAt)
               + "; after that Finora Free applies. Nothing is deleted.";
    case STATUS_CANCELLED:
        return "Cancelled on " + dateText(subscription.cancelledAt) + ". " + name + " stays until "
               + dateText(subscription.expiresAt) + ", then Finora Free applies. Nothing is deleted.";
    case STATUS_EXPIRED:
        return "Your trial or paid plan has ended, so Finora Free applies again. "
               "All your records and budgets are kept.";
    case STATUS_FREE:
        break;
    }
    return "Free since " + dateText(subscription.startedAt)
           + ". Your records stay the same on every plan, so you can upgrade at any time.";
}

void SubscriptionPage::showUsage()
{
    int total = static_cast<int>(budgets.size());
    int paused = countPausedBudgets(budgets);
    int limit = database.budgetLimit();

    if (limit == UNLIMITED_BUDGETS) {
        usageNumber->setText(QString::number(total));
        usageBar->hide();
        if (signedIn) {
            usageText->setText("No limit on " + QString::fromStdString(planName(subscription.plan)) + ".");
        } else {
            usageText->setText("Plans belong to accounts.");
        }
        return;
    }

    int active = total - paused;
    usageNumber->setText(QString::number(active) + " of " + QString::number(limit));
    usageBar->setRange(0, limit);
    usageBar->setValue(qMin(active, limit));
    usageBar->setAccessibleName(QString::number(active) + " of " + QString::number(limit) + " active budgets used");
    if (active >= limit) {
        setTone(usageBar, "warning");
    } else {
        setTone(usageBar, "success");
    }
    usageBar->show();

    QString text = QString::fromStdString(planName(subscription.plan)) + " tracks up to "
                   + QString::number(limit) + " budgets.";
    if (paused > 0) {
        text += " " + countText(paused, "budget is", "budgets are") + " paused; nothing is deleted.";
    }
    usageText->setText(text);
}

void SubscriptionPage::showPlanCards()
{
    bool canChange = signedIn && service != nullptr;
    for (int i = 0; i < PLAN_COUNT; i++) {
        PlanId plan = static_cast<PlanId>(i);
        bool current = signedIn && subscription.plan == plan;
        currentBadges[i]->setText("Current plan");
        if (current && subscription.status == STATUS_TRIAL) {
            currentBadges[i]->setText("Your trial");
        }
        currentBadges[i]->setVisible(current);
        setCurrentCard(planCards[i], current);

        // The card's button says what choosing this plan would do now
        // (planChangeFor() in core/Subscription.h). Only real actions are
        // enabled; "Current plan" is shown, but cannot be clicked.
        QPushButton *button = planButtons[i];
        button->setVisible(canChange);
        if (!canChange) {
            continue;
        }
        QString name = shortPlanName(plan);
        QString style = "secondaryButton";
        bool enabled = true;
        switch (planChangeFor(subscription, plan)) {
        case CHANGE_SUBSCRIBE:
            button->setText("Choose " + name);
            style = "primaryButton";
            break;
        case CHANGE_UPGRADE:
            button->setText("Upgrade to " + name);
            style = "primaryButton";
            break;
        case CHANGE_RENEW:
            button->setText("Pay to renew " + name);
            style = "primaryButton";
            break;
        case CHANGE_DOWNGRADE:
            button->setText("Switch at renewal");
            break;
        case CHANGE_CANCEL:
            button->setText("Switch to Free");
            break;
        case CHANGE_RESUME:
            button->setText("Keep " + name);
            break;
        case CHANGE_NONE:
            enabled = false;
            if (subscription.hasScheduledPlan && subscription.scheduledPlan == plan) {
                button->setText("Starts on " + dateText(subscription.renewsAt));
            } else if (subscription.status == STATUS_CANCELLED && plan == PLAN_FREE) {
                button->setText("From " + dateText(subscription.expiresAt));
            } else {
                button->setText("Current plan");
            }
            break;
        }
        setButtonStyle(button, style);
        button->setEnabled(enabled);
        button->setAccessibleName(button->text() + ": " + QString::fromStdString(planName(plan)));
    }
    trialButton->setVisible(canChange && canStartTrial(subscription));
}

// The buttons of the "Your plan" card, which depend on the status.
void SubscriptionPage::showActions()
{
    clearLayout(actionsLayout);
    if (!signedIn || service == nullptr) {
        return;
    }
    QString name = QString::fromStdString(planName(subscription.plan));
    QString price = QString::fromStdString(formatPlanPrice(subscription.plan));
    QPushButton *button = nullptr;

    switch (subscription.status) {
    case STATUS_FREE:
    case STATUS_EXPIRED:
        if (canStartTrial(subscription)) {
            button = addAction("Try Pro free for 14 days", "primaryButton");
            connect(button, &QPushButton::clicked, this, &SubscriptionPage::onTrialClicked);
        }
        break;
    case STATUS_TRIAL:
        button = addAction("Keep Pro for " + price + " a month", "primaryButton");
        connect(button, &QPushButton::clicked, this, &SubscriptionPage::onPayCurrentPlanClicked);
        button = addAction("End the trial", "secondaryButton");
        connect(button, &QPushButton::clicked, this, &SubscriptionPage::onCancelClicked);
        break;
    case STATUS_ACTIVE:
        if (subscription.hasScheduledPlan) {
            button = addAction("Keep " + name, "secondaryButton");
            connect(button, &QPushButton::clicked, this, &SubscriptionPage::onKeepPlanClicked);
        }
        button = addAction("Cancel subscription", "secondaryButton");
        connect(button, &QPushButton::clicked, this, &SubscriptionPage::onCancelClicked);
        break;
    case STATUS_PAST_DUE:
        button = addAction("Pay " + price + " to renew", "primaryButton");
        connect(button, &QPushButton::clicked, this, &SubscriptionPage::onPayCurrentPlanClicked);
        button = addAction("Stop now", "secondaryButton");
        connect(button, &QPushButton::clicked, this, &SubscriptionPage::onCancelClicked);
        break;
    case STATUS_CANCELLED:
        button = addAction("Resume " + name, "primaryButton");
        connect(button, &QPushButton::clicked, this, &SubscriptionPage::onKeepPlanClicked);
        break;
    }

    // Only in the --billing-demo mode: make the current period end now.
    bool hasPeriod = subscription.status == STATUS_TRIAL || subscription.status == STATUS_ACTIVE
                     || subscription.status == STATUS_PAST_DUE || subscription.status == STATUS_CANCELLED;
    if (service->isDemoMode() && hasPeriod) {
        button = addAction("Demo: end this period now", "linkButton");
        button->setToolTip("Demonstration mode only: makes the current billing period or trial end now.");
        connect(button, &QPushButton::clicked, this, &SubscriptionPage::onSimulateEndClicked);
    }
    actionsLayout->addStretch();
}

QPushButton *SubscriptionPage::addAction(const QString &text, const QString &style)
{
    QPushButton *button = new QPushButton(text, actionsLayout->parentWidget());
    button->setObjectName(style);
    button->setCursor(Qt::PointingHandCursor);
    actionsLayout->addWidget(button);
    return button;
}

void SubscriptionPage::showHistory()
{
    int rows = static_cast<int>(payments.size());
    historyTable->setRowCount(rows);
    for (int i = 0; i < rows; i++) {
        const PaymentRecord &payment = payments[i];
        QString cells[6] = {
            dateText(payment.createdAt),
            QString::fromStdString(planName(payment.plan)),
            paymentReasonText(payment.reason),
            dollarsText(payment.amountMinor),
            paymentStatusText(payment.status),
            QString::fromStdString(payment.reference)
        };
        for (int column = 0; column < 6; column++) {
            QTableWidgetItem *item = new QTableWidgetItem(cells[column]);
            if (column == 4 && !payment.failureReason.empty()) {
                item->setToolTip(QString::fromStdString(payment.failureReason));
            }
            historyTable->setItem(i, column, item);
        }
    }
    int visibleRows = qMin(rows, HISTORY_VISIBLE_ROWS);
    historyTable->setFixedHeight(TABLE_HEADER_HEIGHT + visibleRows * TABLE_ROW_HEIGHT + 2);
    historyTable->setVisible(rows > 0);
    historyEmpty->setVisible(rows == 0);
}

// Puts the plan cards into the grid, `columns` cards per row.
void SubscriptionPage::arrangePlanCards(int columns)
{
    for (int i = 0; i < PLAN_COUNT; i++) {
        planGrid->removeWidget(planCards[i]);
    }
    for (int i = 0; i < PLAN_COUNT; i++) {
        // i / columns is the row and i % columns the column, as on the Budgets page.
        planGrid->addWidget(planCards[i], i / columns, i % columns);
    }
    for (int column = 0; column < PLAN_COUNT; column++) {
        int stretch = 0;
        if (column < columns) {
            stretch = 1;   // equal widths for the columns in use
        }
        planGrid->setColumnStretch(column, stretch);
    }
    columnCount = columns;
}

void SubscriptionPage::resizeEvent(QResizeEvent *event)
{
    Page::resizeEvent(event);
    int columns = 2;
    if (event->size().width() >= WIDE_PAGE_WIDTH) {
        columns = 4;
    }
    if (columns != columnCount) {
        arrangePlanCards(columns);
    }
}

// ---- Actions ----------------------------------------------------------------
// Every button calls SubscriptionService, which checks everything again:
// the screen only says which plan was clicked.

void SubscriptionPage::onManageBudgetsClicked()
{
    emit openPageRequested(PAGE_BUDGETS);
}

void SubscriptionPage::onPlanButtonClicked()
{
    // sender() is the button that was clicked; its property says which plan.
    choosePlan(static_cast<PlanId>(sender()->property("plan").toInt()));
}

void SubscriptionPage::onPayCurrentPlanClicked()
{
    choosePlan(subscription.plan);   // keep Pro after the trial, or pay the renewal
}

// What happens when a plan is chosen depends on the change it means.
void SubscriptionPage::choosePlan(PlanId plan)
{
    if (service == nullptr) {
        return;
    }
    PlanChange change = CHANGE_NONE;
    if (!service->changeFor(plan, change)) {
        showServiceError();
        return;
    }
    QString name = QString::fromStdString(planName(plan));
    QString current = QString::fromStdString(planName(subscription.plan));

    if (changeNeedsPayment(change)) {
        openCheckout(plan, change);
    } else if (change == CHANGE_DOWNGRADE) {
        QString text = "You keep " + current + " until " + dateText(subscription.renewsAt)
                       + ". Nothing is charged now; from then on, " + name + " applies.";
        if (subscription.status == STATUS_CANCELLED) {
            text = "Your cancellation is withdrawn. You keep " + current + " until " + dateText(subscription.expiresAt)
                   + ", then " + name + " applies. Nothing is charged now.";
        }
        if (confirmAction("Switch to " + name + " at the renewal date?", text, "Switch at renewal", "Keep " + current)) {
            if (service->switchAtRenewal(plan)) {
                afterChange(name + " will start at the renewal date.");
            } else {
                showServiceError();
            }
        }
    } else if (change == CHANGE_CANCEL) {
        onCancelClicked();
    } else if (change == CHANGE_RESUME) {
        onKeepPlanClicked();
    }
}

// The simulated checkout. A declined payment also changes the billing
// history, so the page is shown again in both cases.
void SubscriptionPage::openCheckout(PlanId plan, PlanChange change)
{
    CheckoutDialog dialog(*service, plan, change, this);
    if (dialog.exec() == QDialog::Accepted) {
        afterChange("You are on " + QString::fromStdString(planName(plan)) + ". Simulated payment "
                    + dialog.paymentReference() + ".");
    } else if (dialog.paymentAttempted()) {
        refresh();
        emit planChanged();
    }
}

void SubscriptionPage::onCancelClicked()
{
    if (service == nullptr) {
        return;
    }
    QString name = QString::fromStdString(planName(subscription.plan));
    QString title = "Cancel " + name + "?";
    QString text = "You keep " + name + " until " + dateText(subscription.renewsAt)
                   + ". After that, Finora Free applies. Nothing is deleted: budgets over the Free limit are "
                     "paused, not removed.";
    QString yesText = "Cancel subscription";
    QString noText = "Keep " + name;
    if (subscription.status == STATUS_TRIAL) {
        title = "End your Pro trial now?";
        text = "Finora Free applies again at once. Nothing is deleted.";
        yesText = "End the trial";
        noText = "Keep the trial";
    } else if (subscription.status == STATUS_PAST_DUE) {
        title = "Stop " + name + " now?";
        text = "The renewal is not paid, so Finora Free applies at once. Nothing is deleted.";
        yesText = "Stop now";
        noText = "Not now";
    }
    if (!confirmAction(title, text, yesText, noText)) {
        return;
    }
    // The message is made before afterChange() loads the new status.
    QString message = "Finora Free applies again.";
    if (subscription.status == STATUS_ACTIVE) {
        message = name + " is cancelled. It stays until " + dateText(subscription.renewsAt) + ".";
    }
    if (service->cancel()) {
        afterChange(message);
    } else {
        showServiceError();
    }
}

void SubscriptionPage::onKeepPlanClicked()
{
    if (service == nullptr) {
        return;
    }
    if (service->keepCurrentPlan()) {
        afterChange(QString::fromStdString(planName(subscription.plan)) + " continues.");
    } else {
        showServiceError();
    }
}

void SubscriptionPage::onTrialClicked()
{
    if (service == nullptr) {
        return;
    }
    if (service->startTrial()) {
        afterChange("Your free Pro trial has started. Nothing is charged.");
    } else {
        showServiceError();
    }
}

void SubscriptionPage::onSimulateEndClicked()
{
    if (service == nullptr) {
        return;
    }
    if (service->simulatePeriodEnd()) {
        afterChange("Demonstration: the current period has ended.");
    } else {
        showServiceError();
    }
}

// Asks before a change. The safe answer (`noText`) is the default button.
bool SubscriptionPage::confirmAction(const QString &title, const QString &text, const QString &yesText,
                                     const QString &noText)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(title);
    box.setText(title);
    box.setInformativeText(text);
    QPushButton *yes = box.addButton(yesText, QMessageBox::AcceptRole);
    QPushButton *no = box.addButton(noText, QMessageBox::RejectRole);
    box.setDefaultButton(no);
    box.exec();
    return box.clickedButton() == yes;
}

// After a change: show the page again, tell MainWindow, and confirm it.
void SubscriptionPage::afterChange(const QString &message)
{
    refresh();
    emit planChanged();
    Toast::showMessage(this, message);
}

void SubscriptionPage::showServiceError()
{
    QMessageBox::warning(this, "Finora", service->lastError());
    refresh();
}

void SubscriptionPage::onToggleComparisonClicked()
{
    // isHidden() says whether the table itself was hidden; isVisible()
    // would also be false while the whole page is not on the screen.
    bool show = comparisonTable->isHidden();
    comparisonTable->setVisible(show);
    if (show) {
        comparisonButton->setText("Hide the comparison");
    } else {
        comparisonButton->setText("Show the comparison");
    }
}

// ---- For the tests ------------------------------------------------------------

bool SubscriptionPage::isSignedIn() const
{
    return signedIn;
}

const Subscription &SubscriptionPage::shownSubscription() const
{
    return subscription;
}

int SubscriptionPage::shownPaymentCount() const
{
    return static_cast<int>(payments.size());
}

int SubscriptionPage::planColumns() const
{
    return columnCount;
}
