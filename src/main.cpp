// =====================================================================
//  main.cpp
//  Finora - Desktop Personal Finance & Budget Management System
//
//  The program starts here. main() creates the Qt application, gives it
//  Finora's look, shows the main window and then waits for the user.
// =====================================================================
#include <QApplication>

#include "ui/MainWindow.h"
#include "ui/Theme.h"

int main(int argc, char *argv[])
{
    // QApplication runs the whole graphical program: it receives mouse
    // clicks and key presses from Windows and delivers them to widgets.
    QApplication app(argc, argv);
    app.setApplicationName("Finora");
    app.setApplicationVersion(FINORA_VERSION);   // the number comes from CMakeLists.txt

    // Style, font and colours. If the style sheet were missing, Finora
    // would still run, just with Qt's plain default look.
    applyLightTheme(app);

    MainWindow window;
    window.show();

    // Start the event loop: wait for user actions until the window is
    // closed, then return the program's exit code to Windows.
    return app.exec();
}
