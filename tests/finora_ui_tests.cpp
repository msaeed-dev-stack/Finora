// =====================================================================
//  finora_ui_tests.cpp
//  Automated checks for Finora's main window.
//
//  The test builds the real MainWindow, clicks every sidebar button from
//  code, and checks that the matching page appears. CTest runs it with
//  QT_QPA_PLATFORM=offscreen, so the window is drawn in memory only.
//
//  Optional:  finora_ui_tests --screenshots <folder>
//  saves a picture of every page into <folder>.
// =====================================================================
#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QDir>
#include <QStackedWidget>

#include <iostream>
#include <string>

#include "ui/MainWindow.h"
#include "ui/Theme.h"

// Counts how many checks ran and how many of them failed.
struct TestCounter {
    int checksRun = 0;
    int checksFailed = 0;
};

// Records one check: prints PASS when the condition is true, FAIL otherwise.
void check(TestCounter &counter, const std::string &testName, bool passed)
{
    counter.checksRun++;

    if (passed) {
        std::cout << "PASS  " << testName << "\n";
    } else {
        counter.checksFailed++;
        std::cout << "FAIL  " << testName << "\n";
    }
}

// Counts the selected sidebar buttons. It must always be exactly 1.
int countCheckedButtons(QButtonGroup *group)
{
    int checkedCount = 0;

    for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++) {
        if (group->button(pageNumber)->isChecked()) {
            checkedCount++;
        }
    }
    return checkedCount;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Finora");
    app.setApplicationVersion(FINORA_VERSION);

    TestCounter counter;
    std::cout << "Finora interface tests\n";
    std::cout << "----------------------\n";

    check(counter, "light theme and style sheet load", applyLightTheme(app));

    MainWindow window;
    window.resize(1280, 800);
    window.show();

    // Find the page area and the sidebar buttons inside the real window.
    QStackedWidget *pageStack = window.findChild<QStackedWidget *>("pageStack");
    QButtonGroup *navGroup = window.findChild<QButtonGroup *>();

    check(counter, "page area exists", pageStack != nullptr);
    check(counter, "sidebar button group exists", navGroup != nullptr);
    if (pageStack == nullptr || navGroup == nullptr) {
        std::cout << "Cannot continue without the page area and the buttons.\n";
        return 1;
    }

    check(counter, "window title is Finora", window.windowTitle() == "Finora");
    check(counter, "there are 7 pages", pageStack->count() == PAGE_COUNT);
    check(counter, "there are 7 sidebar buttons", navGroup->buttons().size() == PAGE_COUNT);
    check(counter, "starts on the dashboard", pageStack->currentIndex() == PAGE_DASHBOARD);
    check(counter, "dashboard button starts selected", navGroup->button(PAGE_DASHBOARD)->isChecked());

    // Optional screenshots for checking the design by eye.
    QString screenshotFolder;
    if (argc == 3 && std::string(argv[1]) == "--screenshots") {
        screenshotFolder = QString::fromLocal8Bit(argv[2]);
        QDir().mkpath(screenshotFolder);
    }

    // Click every sidebar button and check that the right page appears.
    for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++) {
        QAbstractButton *button = navGroup->button(pageNumber);
        button->setFocus();   // a real mouse click also moves the keyboard focus
        button->click();
        app.processEvents();

        std::string buttonName = button->text().toStdString();
        check(counter, "clicking " + buttonName + " shows page " + std::to_string(pageNumber),
              pageStack->currentIndex() == pageNumber);
        check(counter, "only the " + buttonName + " button is selected",
              button->isChecked() && countCheckedButtons(navGroup) == 1);

        if (!screenshotFolder.isEmpty()) {
            QString fileName = screenshotFolder + "/page_" + QString::number(pageNumber) + ".png";
            window.grab().save(fileName);
        }
    }

    std::cout << "----------------------\n";
    std::cout << counter.checksRun << " checks, " << counter.checksFailed << " failed\n";

    if (counter.checksFailed > 0) {
        return 1;
    }
    return 0;
}
