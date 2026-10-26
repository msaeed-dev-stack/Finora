// =====================================================================
//  Sidebar.h
//  The navigation bar on the left side of Finora's main window.
// =====================================================================
#ifndef FINORA_SIDEBAR_H
#define FINORA_SIDEBAR_H

#include <QButtonGroup>
#include <QFrame>
#include <QIcon>
#include <QVBoxLayout>

// ---- Page numbers ---------------------------------------------------
// Every page has a fixed number. The sidebar buttons and the pages in
// MainWindow use the same numbers in the same order.
const int PAGE_DASHBOARD    = 0;
const int PAGE_TRANSACTIONS = 1;
const int PAGE_BUDGETS      = 2;
const int PAGE_GOALS        = 3;
const int PAGE_REPORTS      = 4;
const int PAGE_CURRENCY     = 5;
const int PAGE_SETTINGS     = 6;
const int PAGE_COUNT        = 7;   // total number of pages

// Shows the Finora logo and one button for each page. When the user
// clicks a button, the sidebar sends the signal pageSelected(pageNumber).
class Sidebar : public QFrame
{
    Q_OBJECT   // required in any class that declares its own signals or slots

public:
    Sidebar(QWidget *parent = nullptr);

    // Highlights the button that belongs to the given page.
    void setCurrentPage(int pageNumber);

signals:
    // Sent when the user clicks one of the navigation buttons.
    void pageSelected(int pageNumber);

private slots:
    // Runs when a navigation button is clicked.
    void onNavButtonClicked(int pageNumber);

private:
    QWidget *createBrandArea();
    void createNavButtons(QVBoxLayout *layout);
    QIcon createNavIcon(const QString &iconName);

    QButtonGroup *navButtonGroup = nullptr;   // keeps exactly one button selected
};

#endif // FINORA_SIDEBAR_H
