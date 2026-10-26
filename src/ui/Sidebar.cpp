// =====================================================================
//  Sidebar.cpp
//  Builds the sidebar: the logo, one button per page, and the version.
// =====================================================================
#include "Sidebar.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

// ---- Data for the navigation buttons --------------------------------
// One structure per button: the text it shows and the name of its icon.
struct NavItem {
    const char *label;      // text on the button
    const char *iconName;   // icon file name without ".svg"
};

// One item per page. The position in the array is the page number,
// so NAV_ITEMS[PAGE_BUDGETS] describes the "Budgets" button.
const NavItem NAV_ITEMS[PAGE_COUNT] = {
    {"Dashboard",    "dashboard"},
    {"Transactions", "transactions"},
    {"Budgets",      "budgets"},
    {"Goals",        "goals"},
    {"Reports",      "reports"},
    {"Currency",     "currency"},
    {"Settings",     "settings"}
};

Sidebar::Sidebar(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("sidebar");   // the style sheet finds widgets by this name
    setFixedWidth(232);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 22, 16, 16);
    layout->setSpacing(4);

    layout->addWidget(createBrandArea());
    layout->addSpacing(28);
    createNavButtons(layout);
    layout->addStretch();   // empty space pushes the version label to the bottom

    QLabel *versionLabel = new QLabel("Version " + QCoreApplication::applicationVersion(), this);
    versionLabel->setObjectName("sidebarFooter");
    layout->addWidget(versionLabel);
}

void Sidebar::setCurrentPage(int pageNumber)
{
    // Ignore numbers that do not belong to any page.
    if (pageNumber < 0 || pageNumber >= PAGE_COUNT) {
        return;
    }

    navButtonGroup->button(pageNumber)->setChecked(true);
}

void Sidebar::onNavButtonClicked(int pageNumber)
{
    emit pageSelected(pageNumber);   // tell MainWindow which page was chosen
}

// The logo, the name "Finora" and the tagline at the top of the sidebar.
QWidget *Sidebar::createBrandArea()
{
    QWidget *brandArea = new QWidget(this);

    QLabel *logo = new QLabel(brandArea);
    logo->setPixmap(QIcon(":/icons/logo.svg").pixmap(QSize(38, 38)));
    logo->setAccessibleName("Finora logo");

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
    brandLayout->addWidget(logo);
    brandLayout->addLayout(textLayout);
    brandLayout->addStretch();

    return brandArea;
}

// Creates one checkable button for every page, using a for loop.
void Sidebar::createNavButtons(QVBoxLayout *layout)
{
    navButtonGroup = new QButtonGroup(this);
    navButtonGroup->setExclusive(true);   // selecting one button unselects the others

    for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++) {
        QPushButton *button = new QPushButton(NAV_ITEMS[pageNumber].label, this);
        button->setObjectName("navButton");
        button->setIcon(createNavIcon(NAV_ITEMS[pageNumber].iconName));
        button->setIconSize(QSize(18, 18));
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);

        navButtonGroup->addButton(button, pageNumber);   // the button's id is its page number
        layout->addWidget(button);
    }

    // When any button in the group is clicked, Qt calls onNavButtonClicked(id).
    connect(navButtonGroup, &QButtonGroup::idClicked, this, &Sidebar::onNavButtonClicked);
}

// A button icon made from two image files:
//   icons/<name>.svg         grey icon for a normal button
//   icons/<name>_active.svg  white icon for the selected (green) button
QIcon Sidebar::createNavIcon(const QString &iconName)
{
    QIcon icon;
    icon.addFile(":/icons/" + iconName + ".svg", QSize(), QIcon::Normal, QIcon::Off);
    icon.addFile(":/icons/" + iconName + "_active.svg", QSize(), QIcon::Normal, QIcon::On);
    return icon;
}
