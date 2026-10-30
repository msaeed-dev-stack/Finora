// =====================================================================
//  Sidebar.h
//  The navigation bar on the left side of Finora's main window.
// =====================================================================
#ifndef FINORA_SIDEBAR_H
#define FINORA_SIDEBAR_H

#include <QButtonGroup>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
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

// Shows the Finora logo, one button for each page and the theme switch.
// When the user clicks a page button, the sidebar sends the signal
// pageSelected(pageNumber).
class Sidebar : public QFrame
{
    Q_OBJECT   // required in any class that declares its own signals or slots

public:
    Sidebar(QWidget *parent = nullptr);

    // Highlights the button that belongs to the given page.
    void setCurrentPage(int pageNumber);

    // Colours the icons again and updates the theme button's text, for
    // example after switching to the dark theme.
    void refreshTheme();

    // Shows who is signed in, with a Sign out button, at the bottom.
    void showAccount(const QString &displayName, const QString &email);

signals:
    // Sent when the user clicks one of the navigation buttons.
    void pageSelected(int pageNumber);

    // Sent when the user clicks "Dark mode" / "Light mode".
    void themeToggleClicked();

    // Sent when the user clicks "Sign out".
    void signOutClicked();

private slots:
    // Runs when a navigation button is clicked.
    void onNavButtonClicked(int pageNumber);

private:
    QWidget *createBrandArea();
    void createNavButtons(QVBoxLayout *layout);
    QIcon createNavIcon(const QString &iconName);

    QButtonGroup *navButtonGroup = nullptr;   // keeps exactly one button selected
    QLabel *logoLabel = nullptr;
    QPushButton *themeButton = nullptr;
    QFrame *accountArea = nullptr;        // hidden until showAccount()
    QLabel *avatarLabel = nullptr;
    QLabel *accountNameLabel = nullptr;
    QLabel *accountEmailLabel = nullptr;
    QPushButton *signOutButton = nullptr;
};

#endif // FINORA_SIDEBAR_H
