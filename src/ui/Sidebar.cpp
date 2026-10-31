// =====================================================================
//  Sidebar.cpp
//  Builds the sidebar: the logo, one button per page, the theme switch
//  and the version.
// =====================================================================
#include "Sidebar.h"

#include <QCoreApplication>
#include <QHBoxLayout>

#include "Theme.h"

// ---- Data for the navigation buttons --------------------------------
// One structure per button: its text, its icon and whether the page is
// still planned. Planned pages get a "Soon" badge, so the sidebar never
// pretends that a feature already works.
struct NavItem {
    const char *label;      // text on the button
    const char *iconName;   // icon file name without ".svg"
    bool planned;           // true while the page only says "coming soon"
};

// One item per page. The position in the array is the page number,
// so NAV_ITEMS[PAGE_BUDGETS] describes the "Budgets" button.
const NavItem NAV_ITEMS[PAGE_COUNT] = {
    {"Dashboard",    "dashboard",    false},
    {"Transactions", "transactions", false},
    {"Budgets",      "budgets",      false},
    {"Goals",        "goals",        true},
    {"Reports",      "reports",      true},
    {"Currency",     "currency",     true},
    {"Subscription", "subscription", false},
    {"Settings",     "settings",     false}
};

Sidebar::Sidebar(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("sidebar");   // the style sheet finds widgets by this name
    setFixedWidth(240);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 22, 16, 16);
    layout->setSpacing(4);

    layout->addWidget(createBrandArea());
    layout->addSpacing(26);

    QLabel *menuLabel = new QLabel("MENU", this);
    menuLabel->setObjectName("sidebarSection");
    layout->addWidget(menuLabel);
    layout->addSpacing(4);

    createNavButtons(layout);
    layout->addStretch();   // empty space pushes the theme switch to the bottom

    // Who is signed in, and Sign out (shown by showAccount()).
    accountArea = new QFrame(this);
    accountArea->setObjectName("accountArea");
    avatarLabel = new QLabel(accountArea);
    avatarLabel->setObjectName("avatar");
    avatarLabel->setFixedSize(34, 34);
    avatarLabel->setAlignment(Qt::AlignCenter);
    accountNameLabel = new QLabel(accountArea);
    accountNameLabel->setObjectName("accountName");
    accountEmailLabel = new QLabel(accountArea);
    accountEmailLabel->setObjectName("accountEmail");
    planButton = new QPushButton(accountArea);
    planButton->setObjectName("planChip");
    planButton->setCursor(Qt::PointingHandCursor);
    planButton->hide();
    connect(planButton, &QPushButton::clicked, this, &Sidebar::onPlanClicked);
    QVBoxLayout *accountText = new QVBoxLayout();
    accountText->setSpacing(0);
    accountText->addWidget(accountNameLabel);
    accountText->addWidget(accountEmailLabel);
    accountText->addSpacing(4);
    accountText->addWidget(planButton, 0, Qt::AlignLeft);
    QHBoxLayout *accountRow = new QHBoxLayout();
    accountRow->setSpacing(10);
    accountRow->addWidget(avatarLabel);
    accountRow->addLayout(accountText, 1);
    signOutButton = new QPushButton("Sign out", accountArea);
    signOutButton->setObjectName("themeToggle");
    signOutButton->setIconSize(QSize(18, 18));
    signOutButton->setCursor(Qt::PointingHandCursor);
    signOutButton->setAccessibleName("Sign out");
    QVBoxLayout *accountLayout = new QVBoxLayout(accountArea);
    accountLayout->setContentsMargins(0, 0, 0, 0);
    accountLayout->setSpacing(8);
    accountLayout->addLayout(accountRow);
    accountLayout->addWidget(signOutButton);
    connect(signOutButton, &QPushButton::clicked, this, &Sidebar::signOutClicked);
    accountArea->hide();
    layout->addWidget(accountArea);
    layout->addSpacing(8);

    themeButton = new QPushButton(this);
    themeButton->setObjectName("themeToggle");
    themeButton->setIconSize(QSize(18, 18));
    themeButton->setCursor(Qt::PointingHandCursor);
    connect(themeButton, &QPushButton::clicked, this, &Sidebar::themeToggleClicked);
    layout->addWidget(themeButton);
    layout->addSpacing(8);

    QLabel *versionLabel = new QLabel("Version " + QCoreApplication::applicationVersion(), this);
    versionLabel->setObjectName("sidebarFooter");
    layout->addWidget(versionLabel);

    refreshTheme();
}

void Sidebar::setCurrentPage(int pageNumber)
{
    // Ignore numbers that do not belong to any page.
    if (pageNumber < 0 || pageNumber >= PAGE_COUNT) {
        return;
    }

    navButtonGroup->button(pageNumber)->setChecked(true);
}

void Sidebar::refreshTheme()
{
    for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++) {
        navButtonGroup->button(pageNumber)->setIcon(createNavIcon(NAV_ITEMS[pageNumber].iconName));
    }

    // The button offers the OTHER theme: a moon in the light theme, a sun in the dark one.
    const ThemeColors &colors = themeColors();
    signOutButton->setIcon(QIcon(tintedIcon("log_out", colors.text, 18)));
    if (currentThemeMode() == THEME_DARK) {
        themeButton->setText("Light mode");
        themeButton->setIcon(QIcon(tintedIcon("sun", colors.text, 18)));
        themeButton->setAccessibleName("Switch to the light theme");
    } else {
        themeButton->setText("Dark mode");
        themeButton->setIcon(QIcon(tintedIcon("moon", colors.text, 18)));
        themeButton->setAccessibleName("Switch to the dark theme");
    }
}

void Sidebar::showAccount(const QString &displayName, const QString &email)
{
    // The avatar shows the first letter of the first two words of the name.
    QStringList words = displayName.split(' ', Qt::SkipEmptyParts);
    QString initials;
    for (int i = 0; i < words.size() && i < 2; i++) {
        initials += words[i].left(1).toUpper();
    }
    avatarLabel->setText(initials);
    accountNameLabel->setText(displayName);
    accountEmailLabel->setText(email);
    accountArea->setAccessibleName("Signed in as " + displayName + ", " + email);
    accountArea->show();
}

void Sidebar::showPlan(const QString &text, const QString &tone)
{
    planButton->setText(text);
    setTone(planButton, tone);
    planButton->setAccessibleName("Your plan: " + text + ". Open the Subscription page");
    planButton->setToolTip("Your plan. Click to open the Subscription page.");
    planButton->show();
}

void Sidebar::onNavButtonClicked(int pageNumber)
{
    emit pageSelected(pageNumber);   // tell MainWindow which page was chosen
}

void Sidebar::onPlanClicked()
{
    emit pageSelected(PAGE_SUBSCRIPTION);
}

// The logo, the name "Finora" and the tagline at the top of the sidebar.
QWidget *Sidebar::createBrandArea()
{
    QWidget *brandArea = new QWidget(this);

    logoLabel = new QLabel(brandArea);
    logoLabel->setPixmap(QIcon(":/icons/logo.svg").pixmap(QSize(40, 40)));
    logoLabel->setAccessibleName("Finora logo");

    QLabel *name = new QLabel("Finora", brandArea);
    name->setObjectName("brandName");

    QLabel *tagline = new QLabel("Your Money. Your Control.", brandArea);
    tagline->setObjectName("brandTagline");

    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(0);
    textLayout->addWidget(name);
    textLayout->addWidget(tagline);

    QHBoxLayout *brandLayout = new QHBoxLayout(brandArea);
    brandLayout->setContentsMargins(6, 0, 0, 0);
    brandLayout->setSpacing(10);
    brandLayout->addWidget(logoLabel);
    brandLayout->addLayout(textLayout);
    brandLayout->addStretch();

    return brandArea;
}

// Creates one checkable button for every page, using a for loop.
void Sidebar::createNavButtons(QVBoxLayout *layout)
{
    navButtonGroup = new QButtonGroup(this);
    navButtonGroup->setObjectName("navButtonGroup");
    navButtonGroup->setExclusive(true);   // selecting one button unselects the others

    for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++) {
        QPushButton *button = new QPushButton(NAV_ITEMS[pageNumber].label, this);
        button->setObjectName("navButton");
        button->setIconSize(QSize(18, 18));
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);

        // A small "Soon" badge inside the button for planned pages.
        if (NAV_ITEMS[pageNumber].planned) {
            QLabel *badge = new QLabel("Soon", button);
            badge->setObjectName("navBadge");
            QHBoxLayout *badgeLayout = new QHBoxLayout(button);
            badgeLayout->setContentsMargins(0, 0, 10, 0);
            badgeLayout->addStretch();
            badgeLayout->addWidget(badge, 0, Qt::AlignVCenter);
            button->setAccessibleName(QString(NAV_ITEMS[pageNumber].label) + " (coming soon)");
        }

        navButtonGroup->addButton(button, pageNumber);   // the button's id is its page number
        layout->addWidget(button);
    }

    // When any button in the group is clicked, Qt calls onNavButtonClicked(id).
    connect(navButtonGroup, &QButtonGroup::idClicked, this, &Sidebar::onNavButtonClicked);
}

// A button icon in two colours, made from one SVG file:
//   Off (normal button)    the muted text colour
//   On  (selected button)  violet
QIcon Sidebar::createNavIcon(const QString &iconName)
{
    const ThemeColors &colors = themeColors();
    QIcon icon;
    icon.addPixmap(tintedIcon(iconName, colors.textMuted, 18), QIcon::Normal, QIcon::Off);
    icon.addPixmap(tintedIcon(iconName, colors.primaryText, 18), QIcon::Normal, QIcon::On);
    return icon;
}
