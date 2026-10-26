// =====================================================================
//  MainWindow.cpp
// =====================================================================
#include "MainWindow.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QScreen>
#include <QTime>

#include "PlaceholderPage.h"
#include "core/Greeting.h"

// ---- Text for the pages that are not built yet ----------------------
// One structure per page, in page-number order (see Sidebar.h).
struct PageInfo {
    const char *title;            // large heading at the top of the page
    const char *subtitle;         // grey line under the heading
    const char *iconName;         // icon file name without ".svg"
    int milestone;                // the milestone that will build this page
    const char *plannedFeatures;  // what the page will let the user do
};

const PageInfo PAGE_INFO[PAGE_COUNT] = {
    {"Dashboard", "Here's your financial overview", "dashboard", 4,
     "Income, expense and balance summaries for each currency, recent transactions and budget alerts."},
    {"Transactions", "Manage your income and expenses", "transactions", 3,
     "Add, edit, search, filter and delete your income and expense records."},
    {"Budgets", "Set and track your spending limits", "budgets", 4,
     "Monthly budgets for each category, with a warning at 80% and an alert at 100%."},
    {"Savings Goals", "Track your progress towards your dreams", "goals", 5,
     "Create savings goals, add contributions and follow your progress."},
    {"Reports", "Analyse your financial data", "reports", 6,
     "Totals by month, category and type, plus export to a CSV file."},
    {"Currency Settings", "Manage currencies and exchange rates", "currency", 5,
     "PKR, USD and EUR, with exchange rates that you enter yourself."},
    {"Settings", "Customise your application", "settings", 6,
     "Your default currency and other preferences."}
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Finora");
    setWindowIcon(QIcon(":/icons/logo.svg"));
    setMinimumSize(960, 600);

    createLayout();
    createPages();
    setStartingSize();

    // Signal and slot: when the sidebar sends pageSelected(n),
    // Qt automatically calls showPage(n).
    connect(sidebar, &Sidebar::pageSelected, this, &MainWindow::showPage);

    showPage(PAGE_DASHBOARD);   // the program always starts on the dashboard
}

void MainWindow::showPage(int pageNumber)
{
    // Ignore numbers that do not belong to any page.
    if (pageNumber < 0 || pageNumber >= PAGE_COUNT) {
        return;
    }

    pageStack->setCurrentIndex(pageNumber);   // show the page
    sidebar->setCurrentPage(pageNumber);      // highlight its button
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

// Creates one page for every sidebar button, using a for loop.
void MainWindow::createPages()
{
    for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++) {
        QString title = PAGE_INFO[pageNumber].title;

        // The dashboard greets the user according to the time of day.
        if (pageNumber == PAGE_DASHBOARD) {
            int hour = QTime::currentTime().hour();
            title = QString::fromStdString(greetingForHour(hour));
        }

        PlaceholderPage *page = new PlaceholderPage(title,
                                                    PAGE_INFO[pageNumber].subtitle,
                                                    PAGE_INFO[pageNumber].iconName,
                                                    PAGE_INFO[pageNumber].milestone,
                                                    PAGE_INFO[pageNumber].plannedFeatures,
                                                    pageStack);
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
