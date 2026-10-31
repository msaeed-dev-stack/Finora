// =====================================================================
//  MainWindow.cpp
// =====================================================================
#include "MainWindow.h"

#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QScreen>

#include "BudgetsPage.h"
#include "CurrencyPage.h"
#include "DashboardPage.h"
#include "GoalsPage.h"
#include "PlaceholderPage.h"
#include "Theme.h"
#include "TransactionsPage.h"
#include "UiHelpers.h"
#include "components/Toast.h"
#include "database/SubscriptionStore.h"

// ---- Text for the pages that are not built yet ----------------------
// One structure per page, in page-number order (see Sidebar.h). Only
// the planned Reports page still uses these texts; the other pages are real.
struct PageInfo {
    const char *title;            // large heading at the top of the page
    const char *subtitle;         // grey line under the heading
    const char *iconName;         // icon file name without ".svg"
    int milestone;                // the milestone that will build this page
    const char *plannedFeatures;  // what the page will let the user do
};

const PageInfo PAGE_INFO[PAGE_COUNT] = {
    {"Dashboard", "Here's your money at a glance", "dashboard", 4,
     "Income, expense and balance summaries for each currency, recent transactions and budget alerts."},
    {"Transactions", "Manage your income and expenses", "transactions", 3,
     "Add, edit, search, filter and delete your income and expense records."},
    {"Budgets", "Monthly spending limits for each category", "budgets", 4,
     "Monthly budgets for each category, with a warning at 80% and an alert at 100%."},
    {"Savings Goals", "Track your progress towards your dreams", "goals", 5,
     "Create savings goals, add contributions and follow your progress."},
    {"Reports", "Analyse your financial data", "reports", 6,
     "Totals by month, category and type, plus export to a CSV file."},
    {"Currency Settings", "Manage currencies and exchange rates", "currency", 5,
     "PKR, USD and EUR, with exchange rates that you enter yourself."},
    {"Subscription", "Your plan, your billing history and what each plan includes", "subscription", 4,
     "Plans, the billing history and a simulated checkout."},
    {"Settings", "Appearance, your data and information about Finora", "settings", 4,
     "Theme, reduced motion and where your data is kept."}
};

// The initializer list sets the reference member `database` and builds
// the BudgetStore member with the same database.
MainWindow::MainWindow(DatabaseManager &sharedDatabase, AuthService *sharedAuth,
                       SubscriptionService *sharedSubscriptions, QWidget *parent)
    : QMainWindow(parent), database(sharedDatabase), auth(sharedAuth), subscriptions(sharedSubscriptions),
      budgetStore(sharedDatabase)
{
    setWindowTitle("Finora");
    setWindowIcon(QIcon(":/icons/logo.svg"));
    setMinimumSize(1024, 640);

    applySavedAppearance();   // before any widget is made, so all start in the right theme
    createLayout();
    createPages();
    setStartingSize();

    pageFade = new QPropertyAnimation(this);
    pageFade->setPropertyName("opacity");
    pageFade->setDuration(180);
    pageFade->setStartValue(0.35);
    pageFade->setEndValue(1.0);
    connect(pageFade, &QPropertyAnimation::finished, this, &MainWindow::onPageFadeFinished);

    // Signals and slots: when the sidebar sends pageSelected(n), Qt
    // automatically calls showPage(n), and so on.
    connect(sidebar, &Sidebar::pageSelected, this, &MainWindow::showPage);
    connect(sidebar, &Sidebar::themeToggleClicked, this, &MainWindow::toggleTheme);

    // With accounts: the sidebar and the greeting show who is signed in.
    if (auth != nullptr) {
        connect(sidebar, &Sidebar::signOutClicked, this, &MainWindow::signOutRequested);
        connect(settingsPage, &SettingsPage::signOutRequested, this, &MainWindow::signOutRequested);
        connect(settingsPage, &SettingsPage::profileChanged, this, &MainWindow::showAccount);
        showAccount();
    }

    showPage(PAGE_DASHBOARD);   // the program always starts on the dashboard
}

bool MainWindow::isDarkTheme() const
{
    return currentThemeMode() == THEME_DARK;
}

void MainWindow::showPage(int pageNumber)
{
    // Ignore numbers that do not belong to any page.
    if (pageNumber < 0 || pageNumber >= PAGE_COUNT) {
        return;
    }

    pageStack->setCurrentIndex(pageNumber);   // show the page
    sidebar->setCurrentPage(pageNumber);      // highlight its button
    pages[pageNumber]->refresh();             // virtual call: the page's own refresh()
    fadeInPage(pages[pageNumber]);

    // A plan can change on the Subscription page or end with time, so
    // the plan under the name is read again whenever a page is shown.
    if (auth != nullptr) {
        showPlanIndicator();
    }
}

void MainWindow::setDarkTheme(bool dark)
{
    ThemeMode mode = THEME_LIGHT;
    if (dark) {
        mode = THEME_DARK;
    }
    applyTheme(*qApp, mode);

    QString value = "light";
    if (dark) {
        value = "dark";
    }
    database.saveSetting("theme", value);

    // Pictures (icons, chart colours) are made again in the new colours.
    sidebar->refreshTheme();
    settingsPage->refresh();
    pages[pageStack->currentIndex()]->refresh();

    if (dark) {
        Toast::showMessage(this, "Dark theme on");
    } else {
        Toast::showMessage(this, "Light theme on");
    }
}

void MainWindow::setReducedMotionEnabled(bool reduced)
{
    setReducedMotion(reduced);
    QString value = "0";
    if (reduced) {
        value = "1";
    }
    database.saveSetting("reduced_motion", value);
    settingsPage->refresh();

    if (reduced) {
        Toast::showMessage(this, "Reduced motion on: changes appear without animation");
    } else {
        Toast::showMessage(this, "Animations on");
    }
}

// The signed-in person's name in the sidebar and in the dashboard greeting.
void MainWindow::showAccount()
{
    const Session &session = auth->session();
    sidebar->showAccount(QString::fromStdString(session.displayName()), QString::fromStdString(session.email()));
    dashboardPage->setUserName(QString::fromStdString(session.firstName()));
    showPlanIndicator();
}

// The person's plan under the name in the sidebar (Phase 4C).
void MainWindow::showPlanIndicator()
{
    SubscriptionStore store(database);
    Subscription stored;
    if (!store.loadSubscription(stored)) {
        return;   // nobody signed in: the account area is not shown anyway
    }
    Subscription now = subscriptionAt(stored, currentUtcTime());
    sidebar->showPlan(planChipText(now), planChipTone(now));
}

void MainWindow::toggleTheme()
{
    setDarkTheme(!isDarkTheme());
}

// Theme and motion setting saved in the database. A missing or unknown
// value means the defaults: light theme, animations on.
void MainWindow::applySavedAppearance()
{
    setReducedMotion(database.setting("reduced_motion", "0") == "1");

    ThemeMode mode = THEME_LIGHT;
    if (database.setting("theme", "light") == "dark") {
        mode = THEME_DARK;
    }
    applyTheme(*qApp, mode);
}

// Sidebar on the left, page area on the right.
void MainWindow::createLayout()
{
    QWidget *centralArea = new QWidget(this);

    sidebar = new Sidebar(centralArea);
    pageStack = new QStackedWidget(centralArea);
    pageStack->setObjectName("pageStack");

    QHBoxLayout *mainLayout = new QHBoxLayout(centralArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(pageStack, 1);   // the page area takes all remaining width

    setCentralWidget(centralArea);
}

// Creates one page for every sidebar button, using a for loop and a
// switch. Every page is stored as a Page pointer (its base class).
void MainWindow::createPages()
{
    for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++) {
        Page *page = nullptr;

        switch (pageNumber) {
        case PAGE_DASHBOARD: {
            dashboardPage = new DashboardPage(database, budgetStore, pageStack);
            connect(dashboardPage, &DashboardPage::openPageRequested, this, &MainWindow::showPage);
            page = dashboardPage;
            break;
        }
        case PAGE_TRANSACTIONS: {
            TransactionsPage *transactionsPage = new TransactionsPage(database, pageStack);
            connect(transactionsPage, &TransactionsPage::openPageRequested, this, &MainWindow::showPage);
            page = transactionsPage;
            break;
        }
        case PAGE_BUDGETS: {
            BudgetsPage *budgetsPage = new BudgetsPage(database, budgetStore, pageStack);
            connect(budgetsPage, &BudgetsPage::openPageRequested, this, &MainWindow::showPage);
            page = budgetsPage;
            break;
        }
        case PAGE_GOALS: {
            GoalsPage *goalsPage = new GoalsPage(database, pageStack);
            connect(goalsPage, &GoalsPage::openPageRequested, this, &MainWindow::showPage);
            page = goalsPage;
            break;
        }
        case PAGE_CURRENCY:
            page = new CurrencyPage(database, pageStack);
            break;
        case PAGE_SUBSCRIPTION:
            subscriptionPage = new SubscriptionPage(database, budgetStore, subscriptions, pageStack);
            connect(subscriptionPage, &SubscriptionPage::openPageRequested, this, &MainWindow::showPage);
            connect(subscriptionPage, &SubscriptionPage::planChanged, this, &MainWindow::showPlanIndicator);
            page = subscriptionPage;
            break;
        case PAGE_SETTINGS:
            settingsPage = new SettingsPage(database, auth, pageStack);
            connect(settingsPage, &SettingsPage::darkThemeChosen, this, &MainWindow::setDarkTheme);
            connect(settingsPage, &SettingsPage::reducedMotionChosen, this, &MainWindow::setReducedMotionEnabled);
            connect(settingsPage, &SettingsPage::openPageRequested, this, &MainWindow::showPage);
            page = settingsPage;
            break;
        default:
            // Reports is planned: an honest "coming soon" page.
            page = new PlaceholderPage(PAGE_INFO[pageNumber].title, PAGE_INFO[pageNumber].subtitle,
                                       PAGE_INFO[pageNumber].iconName, PAGE_INFO[pageNumber].milestone,
                                       PAGE_INFO[pageNumber].plannedFeatures, pageStack);
            break;
        }

        pages.push_back(page);
        pageStack->addWidget(page);
    }
}

// Opens at 1280 x 800, but never larger than 90% of the screen,
// so the whole window fits on small laptop screens too.
void MainWindow::setStartingSize()
{
    QRect screenArea = screen()->availableGeometry();   // the screen without the taskbar

    int width = 1280;
    int height = 800;
    int maxWidth = screenArea.width() * 9 / 10;
    int maxHeight = screenArea.height() * 9 / 10;

    if (width > maxWidth) {
        width = maxWidth;
    }
    if (height > maxHeight) {
        height = maxHeight;
    }

    resize(width, height);

    // Place the window in the middle of the screen.
    int left = screenArea.x() + (screenArea.width() - width) / 2;
    int top = screenArea.y() + (screenArea.height() - height) / 2;
    move(left, top);
}

// A short fade-in of the page that was just shown, unless the user
// asked for reduced motion.
void MainWindow::fadeInPage(QWidget *page)
{
    if (fadingPage != nullptr) {
        fadingPage->setGraphicsEffect(nullptr);   // end an unfinished fade at once
        fadingPage = nullptr;
    }
    if (isReducedMotion() || !isVisible()) {
        return;
    }

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(page);
    page->setGraphicsEffect(effect);   // the page now owns the effect
    fadingPage = page;
    pageFade->setTargetObject(effect);
    pageFade->start();
}

void MainWindow::onPageFadeFinished()
{
    // Removing the effect deletes it, and the page is drawn normally again.
    if (fadingPage != nullptr) {
        fadingPage->setGraphicsEffect(nullptr);
        fadingPage = nullptr;
    }
}
