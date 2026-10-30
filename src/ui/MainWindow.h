// =====================================================================
//  MainWindow.h
//  Finora's main window: the sidebar on the left and the pages on the
//  right. A QStackedWidget holds every page but shows only one at a time.
// =====================================================================
#ifndef FINORA_MAINWINDOW_H
#define FINORA_MAINWINDOW_H

#include <QMainWindow>
#include <QPropertyAnimation>
#include <QStackedWidget>

#include <vector>

#include "Page.h"
#include "SettingsPage.h"
#include "Sidebar.h"
#include "database/BudgetStore.h"
#include "database/DatabaseManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT   // required because this class has slots

public:
    // `sharedDatabase` is the one DatabaseManager created in main(). The
    // window passes it on to the pages; it does not own it. The window
    // first applies the theme and motion setting saved in the database.
    MainWindow(DatabaseManager &sharedDatabase, QWidget *parent = nullptr);

    bool isDarkTheme() const;

public slots:
    // Shows the page with the given number, highlights its button and
    // lets the page load fresh data.
    void showPage(int pageNumber);

    // Switches the whole program to the dark (true) or light theme and
    // saves the choice.
    void setDarkTheme(bool dark);

    // Turns "reduce motion" on or off and saves the choice.
    void setReducedMotionEnabled(bool reduced);

private slots:
    void toggleTheme();
    void onPageFadeFinished();

private:
    void applySavedAppearance();
    void createLayout();
    void createPages();
    void setStartingSize();
    void fadeInPage(QWidget *page);

    DatabaseManager &database;   // created in main(); the window only uses it
    BudgetStore budgetStore;     // the budget part of the same database
    Sidebar *sidebar = nullptr;
    QStackedWidget *pageStack = nullptr;
    SettingsPage *settingsPage = nullptr;

    // Every page as a Page pointer, in page-number order. Calling
    // pages[n]->refresh() runs the refresh() of that page's own class.
    std::vector<Page *> pages;

    QPropertyAnimation *pageFade = nullptr;   // a short fade when a page is shown
    QWidget *fadingPage = nullptr;            // the page that is fading in, if any
};

#endif // FINORA_MAINWINDOW_H
