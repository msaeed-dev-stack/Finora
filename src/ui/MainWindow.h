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

class MainWindow : public QMainWindow
{
    Q_OBJECT   // required because this class has a slot (showPage)

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    // Shows the page with the given number and highlights its button.
    void showPage(int pageNumber);

private:
    void createLayout();
    void createPages();
    void setStartingSize();

    Sidebar *sidebar = nullptr;
    QStackedWidget *pageStack = nullptr;
};

#endif // FINORA_MAINWINDOW_H
