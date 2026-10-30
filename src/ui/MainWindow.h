// =====================================================================
//  MainWindow.h
//  Finora's main window: the sidebar on the left and the pages on the
//  right. A QStackedWidget holds every page but shows only one at a time.
// =====================================================================
#ifndef FINORA_MAINWINDOW_H
#define FINORA_MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>

#include "Sidebar.h"
#include "database/DatabaseManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT   // required because this class has a slot (showPage)

public:
    // `database` is the one DatabaseManager created in main(). The window
    // passes it on to the pages that need it; it does not own it.
    MainWindow(DatabaseManager &database, QWidget *parent = nullptr);

private slots:
    // Shows the page with the given number and highlights its button.
    void showPage(int pageNumber);

private:
    void createLayout();
    void createPages(DatabaseManager &database);
    void setStartingSize();

    Sidebar *sidebar = nullptr;
    QStackedWidget *pageStack = nullptr;
};

#endif // FINORA_MAINWINDOW_H
