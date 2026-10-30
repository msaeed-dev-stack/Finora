// =====================================================================
//  Page.h
//  The base class of every page in Finora's main window.
// =====================================================================
#ifndef FINORA_PAGE_H
#define FINORA_PAGE_H

#include <QWidget>

// Every page (Dashboard, Transactions, Budgets, ...) inherits from Page.
// MainWindow keeps all of them as Page pointers and calls refresh() when
// a page is shown or the theme changes. Each page overrides refresh()
// with its own code, and C++ calls the right version at run time
// (polymorphism through a virtual function). MainWindow therefore never
// needs to know which kind of page it is talking to.
class Page : public QWidget
{
    Q_OBJECT

public:
    Page(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }

    // Loads fresh data and draws the page again. A page without data,
    // such as a "coming soon" page, keeps this empty version.
    virtual void refresh()
    {
    }
};

#endif // FINORA_PAGE_H
