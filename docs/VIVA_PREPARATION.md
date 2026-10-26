# Finora Viva Preparation

Questions your Programming Fundamentals instructor may ask about Finora, with answers that match the real code. Every answer names the file or function it comes from, so you can open the code and check it yourself.

Features that are not built yet are marked **Planned — Not Yet Implemented**. Never present them as finished.

Last updated: Milestone 1, 27 October 2026.

**How to use this file.** Read the short answer first. Then open the file named under "Where in the code" and find the lines yourself. If you cannot explain a line, study it before the viva.

---

## A. Project Overview

### A1. What is Finora?
- **Short answer:** A Windows desktop program for personal finance and budgeting, written in C++17 with Qt 6 Widgets.
- **Detailed explanation:** Finora will record income and expenses, track monthly budgets and savings goals, and handle PKR, USD and EUR without mixing them. At Milestone 1 the window, sidebar navigation, theme and tests work. Every page shows which milestone will add its feature.
- **Where in the code:** `src/main.cpp` starts the program. `src/ui/MainWindow.cpp` builds the window.
- **Example:** Start `build\debug\Finora.exe`, click "Budgets", and the Budgets page says "Coming in Milestone 4".

### A2. What problem does it solve?
- **Short answer:** People lose track of what they earn and spend, overspend without warning, and mix currencies in their totals.
- **Detailed explanation:** A notebook cannot warn you at 80% of a budget, and adding dollars to rupees gives a meaningless total. Finora will keep each currency separate, warn about budgets and show savings progress. All of it works offline.
- **Where in the code:** Problem statement in `README.md`.

### A3. Who are the intended users?
- **Short answer:** Students, young earners and freelancers who want a simple offline money tracker.
- **Detailed explanation:** Freelancers often earn in USD or EUR and spend in PKR, which is why multi-currency support is part of the plan.

### A4. What are the main features?
- **Short answer:** Navigation, theme and tests are done. Transactions, budgets, goals, currencies, reports and saving to a database are planned.
- **Detailed explanation:** See the feature table in `README.md`. Done in Milestone 1: the main window with 7 pages, the sidebar, the light theme, the dashboard greeting, and two automated test programs. Everything else is **Planned — Not Yet Implemented**, Milestones 2 to 7.

### A5. What are the scope and limitations?
- **Short answer:** One user on one Windows computer, offline, three currencies, exchange rates typed in by hand.
- **Detailed explanation:** No cloud, no bank connection, no live exchange rates, and no mobile version. At Milestone 1 there are no finance features yet.
- **Where in the code:** "Known limitations" in `README.md`.

### A6. Why did you choose this project?
- **Short answer:** Answer this in your own words. It must be your real reason.
- **Points you may use if they are true for you:** money tracking is a real daily problem; the project naturally needs loops, conditions, arrays, structures, classes and files; a GUI makes the demonstration clear.

---

## B. C++ Fundamentals

### B1. Which variables and data types did you use, and why?
- **Short answer:** `int` for hours, page numbers and sizes; `bool` for success or failure; `std::string` for text in the core; `QString` for text on screen; `const char *` for fixed labels.
- **Detailed explanation:** `int hour` in `MainWindow::createPages` holds the current hour. `int width` and `int height` in `MainWindow::setStartingSize` hold the window size. `applyLightTheme` returns a `bool`. `double` is not used yet. Money will use `long long` whole numbers of paisa or cents, which is **Planned — Not Yet Implemented** for Milestone 3.
- **Where in the code:** `src/ui/MainWindow.cpp`, `src/ui/Theme.cpp`, `src/core/Greeting.cpp`.
- **Example:** `int maxHeight = screenArea.height() * 9 / 10;`

### B2. Where are arithmetic, relational and logical operators used?
- **Short answer:** Arithmetic in `setStartingSize`; relational and logical in `greetingForHour` and `showPage`.
- **Detailed explanation:** `screenArea.width() * 9 / 10` computes 90% of the screen. `hour >= 5 && hour < 12` uses relational operators joined by logical AND. `pageNumber < 0 || pageNumber >= PAGE_COUNT` uses logical OR to reject invalid pages. `!file.open(...)` uses logical NOT in `loadStyleSheet`.
- **Example:** For a screen 1366 pixels wide, `1366 * 9 / 10` is `12294 / 10`, which is `1229`, because integer division drops the remainder.

### B3. Where did you use type casting or conversion?
- **Short answer:** Between `int`, `std::string` and `QString`.
- **Detailed explanation:** `QString::fromStdString(greetingForHour(hour))` in `MainWindow::createPages` turns the core's `std::string` into a `QString` for the screen. `QString::number(milestone)` in `PlaceholderPage::createComingSoonCard` turns an `int` into text. `std::to_string(cases[i].hour)` in the tests turns an `int` into a `std::string`.
- **Example:** `QString::number(3)` gives the text "3", so the heading becomes "Coming in Milestone 3".

### B4. Which loops and control structures did you use?
- **Short answer:** `for` loops and `if / else if / else`.
- **Detailed explanation:** `Sidebar::createNavButtons` and `MainWindow::createPages` each loop 7 times, once per page. `testGreetingForHour` loops over 11 test cases. `greetingForHour` uses an if-else chain. `switch` and `while` are not used yet. A `while` loop will read database rows in Milestone 2, and a `switch` will choose budget status text in Milestone 4. Both are **Planned — Not Yet Implemented**.
- **Example:** `for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++)` runs for 0, 1, 2, 3, 4, 5 and 6.

### B5. Why did you use functions instead of writing everything in `main()`?
- **Short answer:** Each function does one job, so the code is easier to read, test and change.
- **Detailed explanation:** `main()` has only seven statements: it creates the application, sets its name and version, applies the theme, creates and shows the window, and starts the event loop. The greeting rule lives in `greetingForHour`, so the tests can call it directly with any hour.
- **Where in the code:** `src/main.cpp`, `src/core/Greeting.cpp`.

### B6. Where did you use strings, arrays, structures or vectors?
- **Short answer:** Arrays of structures for the buttons, pages and test cases. Strings for all text. Vectors are not used yet.
- **Detailed explanation:** `NAV_ITEMS` in `Sidebar.cpp` is an array of 7 `NavItem` structures, each holding a label and an icon name. `PAGE_INFO` in `MainWindow.cpp` is an array of 7 `PageInfo` structures. `cases` in `finora_tests.cpp` is an array of 11 `GreetingCase` structures. `std::vector` is **Planned — Not Yet Implemented**: it will hold the list of transactions in Milestone 3.
- **Example:** `NAV_ITEMS[PAGE_BUDGETS].label` is "Budgets", because `PAGE_BUDGETS` is 2.

### B7. How do parameters, return values and function calls work in your code?
- **Short answer:** Values go in through parameters and come back through the return value.
- **Detailed explanation:** `greetingForHour(int hour)` receives a copy of the hour and returns a `std::string`. `checkText(TestCounter &counter, ...)` receives the counter by reference, written `&`, so it can increase the caller's counter. `const QString &title` passes text by reference without copying it and without allowing changes.
- **Example:** `greetingForHour(14)` returns "Good Afternoon".

### B8. What are header files and include guards?
- **Short answer:** A `.h` file declares what a class or function looks like. The `.cpp` file contains the code. Include guards stop a header being included twice.
- **Where in the code:** Every header starts with `#ifndef FINORA_..._H` and `#define FINORA_..._H`, and ends with `#endif`.

### B9. Why are the page numbers constants?
- **Short answer:** So each number has one name and cannot change by mistake.
- **Detailed explanation:** `const int PAGE_COUNT = 7;` in `Sidebar.h` is used by the loops, the arrays and the range checks. Adding a page means changing one constant and adding one array entry.

### B10. What is a pointer here, and what is `nullptr`?
- **Short answer:** A pointer holds the address of an object. `nullptr` means it points to nothing yet.
- **Detailed explanation:** Qt widgets are created with `new`, for example `new QPushButton(...)`, and stored in pointers. `Sidebar *sidebar = nullptr;` in `MainWindow.h` starts empty and is filled in `createLayout`.

---

## C. Object-Oriented Programming

### C1. Which classes and objects are used?
- **Short answer:** `MainWindow`, `Sidebar`, `PageHeader` and `PlaceholderPage`. `main()` creates one `MainWindow` object.
- **Detailed explanation:** `MainWindow window;` in `main.cpp` creates the window object. Its constructor creates one `Sidebar` object and seven `PlaceholderPage` objects. Each page creates one `PageHeader`. The finance classes `Transaction`, `Budget` and `SavingsGoal` are **Planned — Not Yet Implemented**, Milestones 3 to 5.

### C2. What does each class represent?

| Class | Represents | File |
|---|---|---|
| `MainWindow` | The whole window: sidebar plus page area | `src/ui/MainWindow.h/.cpp` |
| `Sidebar` | The navigation bar with logo and 7 buttons | `src/ui/Sidebar.h/.cpp` |
| `PageHeader` | The title and subtitle at the top of a page | `src/ui/PageHeader.h/.cpp` |
| `PlaceholderPage` | A page whose feature is not built yet | `src/ui/PlaceholderPage.h/.cpp` |

### C3. What are constructors, and where are they used?
- **Short answer:** A constructor is a special member function that runs automatically when an object is created. It builds the object's widgets.
- **Detailed explanation:** `MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)` first passes `parent` to the Qt base class through the initializer list. Then it sets the title, creates the layout and pages, connects the signal and shows the dashboard. `PageHeader`'s constructor takes the title and subtitle as parameters.
- **Example:** `new PageHeader("Budgets", "Set and track your spending limits", this)` creates a header with that text.

### C4. How does encapsulation apply to the project?
- **Short answer:** Data members are private. Other classes use public member functions.
- **Detailed explanation:** In `Sidebar.h`, `navButtonGroup` is private. `MainWindow` cannot touch it. It must call the public function `setCurrentPage(pageNumber)`, which first checks that the number is valid. That is encapsulation: the class protects its own data.

### C5. Which data members and member functions does each class have?

| Class | Private data members | Member functions |
|---|---|---|
| `MainWindow` | `sidebar`, `pageStack` | Constructor, `showPage` (slot), `createLayout`, `createPages`, `setStartingSize` |
| `Sidebar` | `navButtonGroup` | Constructor, `setCurrentPage`, `pageSelected` (signal), `onNavButtonClicked` (slot), `createBrandArea`, `createNavButtons`, `createNavIcon` |
| `PageHeader` | `titleLabel`, `subtitleLabel` | Constructor |
| `PlaceholderPage` | none | Constructor, `createComingSoonCard` |

### C6. How do the classes interact?
- **Short answer:** `MainWindow` owns the `Sidebar` and the pages. The `Sidebar` tells `MainWindow` which page was chosen using a signal.
- **Detailed explanation:** `Sidebar` does not know about the pages. It only emits `pageSelected(pageNumber)`. `MainWindow` connects that signal to its `showPage` slot. This keeps each class independent.

### C7. Did you use inheritance?
- **Short answer:** Only to extend Qt classes, which is how Qt programs are written.
- **Detailed explanation:** `class Sidebar : public QFrame` means a `Sidebar` is a `QFrame` with extra parts. `MainWindow` extends `QMainWindow`. `PageHeader` and `PlaceholderPage` extend `QWidget`. I did not create my own class hierarchy.

### C8. You create widgets with `new`. Who deletes them?
- **Short answer:** Qt does. Every widget has a parent, and a parent deletes its children when it is destroyed.
- **Detailed explanation:** `new QPushButton(NAV_ITEMS[pageNumber].label, this)` gives the button the sidebar as its parent. When the window closes, `MainWindow` is destroyed and Qt deletes the whole tree of child widgets. That is why there is no `delete` in the code.

---

## D. Algorithms and Financial Logic

### D1. How is the dashboard greeting chosen?
- **Short answer:** `greetingForHour` checks the hour with an if-else chain.
- **Detailed explanation:** First it rejects invalid hours below 0 or above 23 and returns "Hello". Then 5 to 11 gives "Good Morning", 12 to 16 gives "Good Afternoon", and everything else gives "Good Evening". `MainWindow::createPages` gets the hour from `QTime::currentTime().hour()` and passes it in.
- **Where in the code:** `src/core/Greeting.cpp`. Flowchart in `docs/DESIGN.md`, section 4.
- **Example:** At 14:30 the hour is 14. `14 >= 5 && 14 < 12` is false. `14 >= 12 && 14 < 17` is true, so the result is "Good Afternoon".

### D2. How does page switching work?
- **Short answer:** Button click, then signal, then slot, then the page changes.
- **Detailed explanation:** The 7 buttons are in a `QButtonGroup`. Each button's id is its page number. A click makes the group emit `idClicked(id)`. That calls `Sidebar::onNavButtonClicked`, which emits `pageSelected(id)`. That calls `MainWindow::showPage(id)`, which checks the number, shows the page with `pageStack->setCurrentIndex(id)` and highlights the button.
- **Where in the code:** `Sidebar::createNavButtons`, `Sidebar::onNavButtonClicked`, `MainWindow::showPage`.
- **Example:** Clicking "Reports" sends 4, and page 4, the Reports page, appears.

### D3. How is the starting window size calculated?
- **Short answer:** 1280 × 800, reduced to 90% of the screen if the screen is smaller, then centred.
- **Where in the code:** `MainWindow::setStartingSize`. Worked example in `docs/DESIGN.md`, section 5.

### D4. How is total income calculated?
**Planned — Not Yet Implemented** (Milestone 4). Planned approach: a `for` loop goes through the transactions of one currency and one month and adds the amounts whose type is income.

### D5. How are total expenses calculated?
**Planned — Not Yet Implemented** (Milestone 4). Planned approach: the same loop, adding amounts whose type is expense.

### D6. How is the net balance calculated?
**Planned — Not Yet Implemented** (Milestone 4). Planned rule: net balance = total income − total expenses, computed separately for each currency.

### D7. How are transactions categorised and filtered?
**Planned — Not Yet Implemented** (Milestone 3).

### D8. How does the budget alert system work?
**Planned — Not Yet Implemented** (Milestone 4). Agreed rule: an expense counts toward a budget when its category, currency and month all match the budget. A warning shows at 80% of the limit, and an "exceeded" alert at 100%.

### D9. How is savings-goal progress calculated?
**Planned — Not Yet Implemented** (Milestone 5).

### D10. How are currencies converted?
**Planned — Not Yet Implemented** (Milestone 5). Agreed rules are in `docs/DESIGN.md`, section 6: rates are stored to four decimal places, and a conversion rounds once to the nearest paisa or cent.

### D11. How does the program avoid combining different currencies incorrectly?
**Planned — Not Yet Implemented** (Milestones 4 and 5). Agreed rule: totals are always calculated per currency, and a converted total is clearly labelled.

### D12. How are monetary rounding and invalid amounts handled?
**Planned — Not Yet Implemented** (Milestone 3). Agreed rules: amounts are stored as whole numbers of paisa or cents in a `long long`; more than two decimals, zero and negative amounts are rejected.

---

## E. Database and File Handling

### E1. Does the program read any file now?
- **Short answer:** Yes. `loadStyleSheet` reads the style sheet file `light.qss`.
- **Detailed explanation:** It creates a `QFile`, opens it read-only as text, reads all of it with `readAll()`, closes it and returns the text. If `open()` fails, it prints a warning and returns an empty string, and the program continues with Qt's default look.
- **Where in the code:** `src/ui/Theme.cpp`.
- **Note:** The file is built into `Finora.exe` through Qt's resource system. The path `:/styles/light.qss` starts with `:/`, which means "inside the program".

### E2. Why was SQLite selected?
- **Short answer:** It is a complete database stored in one file. It needs no server or internet, and Qt supports it directly.
- **Status:** The decision is made, and support was verified on this computer: SQLite 3.53.4 through Qt SQL. The database code is **Planned — Not Yet Implemented** (Milestone 2).

### E3. What information is stored in the database?
**Planned — Not Yet Implemented** (Milestone 2).

### E4. How does the application save, retrieve, update and delete transactions?
**Planned — Not Yet Implemented** (Milestones 2 and 3).

### E5. What happens when the database cannot be opened?
**Planned — Not Yet Implemented** (Milestone 2).

### E6. How is user data preserved after closing the application?
**Planned — Not Yet Implemented** (Milestone 2).

### E7. How can the data be backed up?
**Planned — Not Yet Implemented** (Milestone 2). Planned answer: copy the single database file while Finora is closed.

---

## F. Qt and Desktop UI

### F1. What is Qt?
- **Short answer:** A C++ library for building graphical programs. It provides windows, buttons, layouts and much more.
- **Detailed explanation:** The project brief lists Qt as a suitable GUI library. Finora uses Qt 6.12.0 with its MinGW 13.1 compiler.

### F2. Why did you use Qt Widgets?
- **Short answer:** Widgets are normal C++ classes for desktop windows, so the whole program stays in C++.
- **Detailed explanation:** The other Qt option, Qt Quick, uses a separate language called QML. Widgets fit a C++ course better.

### F3. What is the purpose of `QApplication` and `QMainWindow`?
- **Short answer:** `QApplication` runs the program and its event loop. `QMainWindow` is the main window.
- **Detailed explanation:** In `main.cpp`, `QApplication app(argc, argv);` must be created first. `app.exec()` starts the event loop, which waits for clicks and key presses and sends them to the right widget until the window closes. `MainWindow` extends `QMainWindow` and puts the sidebar and the page stack in its central area.

### F4. How do signals and slots connect the UI to the program logic?
- **Short answer:** A signal is a message an object sends. A slot is a function that runs when the message arrives. `connect()` links them.
- **Example:** `connect(sidebar, &Sidebar::pageSelected, this, &MainWindow::showPage);` in the `MainWindow` constructor. When the sidebar emits `pageSelected(3)`, Qt calls `showPage(3)`.

### F5. What does `Q_OBJECT` do?
- **Short answer:** It lets a class have its own signals and slots.
- **Detailed explanation:** Qt's tool `moc` reads classes marked `Q_OBJECT` and generates extra code for them. `Sidebar` and `MainWindow` have it because they declare signals or slots. `PageHeader` and `PlaceholderPage` do not need it.

### F6. How is the interface organised?
- **Short answer:** Sidebar on the left, pages on the right, one page visible at a time.
- **Detailed explanation:** `MainWindow::createLayout` puts the `Sidebar`, which is 232 pixels wide, and a `QStackedWidget` side by side in a `QHBoxLayout`. The stacked widget holds all 7 pages and shows one. Every page uses `PageHeader` so all titles look the same.

### F7. How is the appearance controlled?
- **Short answer:** A Qt style sheet, `resources/styles/light.qss`, which works like CSS.
- **Detailed explanation:** Widgets get an object name such as `setObjectName("navButton")`, and the style sheet styles that name. For example, `QPushButton#navButton:checked` makes the selected button green. `applyLightTheme` also selects Qt's "Fusion" style and light colours, so the program looks the same on Windows 10 and 11, and even when Windows is in dark mode.

### F8. How do the GUI and the database interact?
**Planned — Not Yet Implemented** (Milestone 2).

### F9. How does `Finora.exe` run without Qt installed?
- **Short answer:** After each build, the tool `windeployqt` copies the Qt DLLs, plugins and MinGW runtime next to `Finora.exe`.
- **Where in the code:** The `POST_BUILD` command in `CMakeLists.txt`.

---

## G. Validation, Errors and Testing

### G1. How does the program validate input?
- **Short answer:** Functions check values before using them.
- **Detailed explanation:** `greetingForHour` rejects hours outside 0 to 23. `MainWindow::showPage` and `Sidebar::setCurrentPage` ignore page numbers outside 0 to 6. Validation of user-typed data such as amounts and dates is **Planned — Not Yet Implemented** (Milestone 3).

### G2. What happens if a user enters zero, a negative amount or invalid text?
**Planned — Not Yet Implemented** (Milestone 3). Agreed rule: all three are rejected with a clear message.

### G3. What happens if a transaction is edited or deleted?
**Planned — Not Yet Implemented** (Milestone 3). Deleting will ask for confirmation first.

### G4. How did you test the program?
- **Short answer:** Two automated test programs, run with `ctest --preset debug` or `build.bat`.
- **Detailed explanation:** `finora_tests` checks `greetingForHour` with 11 hours: 11 of 11 pass. `finora_ui_tests` creates the real `MainWindow`, clicks each of the 7 buttons from code, and checks the right page appears and only one button is selected: 22 of 22 pass. Each program returns 0 when all checks pass and 1 otherwise.
- **Where in the code:** `tests/finora_tests.cpp`, `tests/finora_ui_tests.cpp`.

### G5. What edge cases did you consider?
- **Short answer:** The first and last hour of every greeting range, and invalid hours.
- **Detailed explanation:** Mistakes usually happen at boundaries, for example writing `<=` instead of `<`. So the tests use 4 and 5, 11 and 12, 16 and 17, 23 and 0, and the invalid values -1 and 24.

### G6. What bugs did you encounter, and how were they fixed?
- **Wrong compiler.** The computer also has an old GCC 6.3 that cannot build Qt 6. **Fix:** `CMakePresets.json` names the Qt compiler's full path.
- **Interface test crashed at start.** The "offscreen" plugin it needs was not in the build folder. **Fix:** the test is told to look in Qt's plugin folder too; see `set_tests_properties` in `CMakeLists.txt`.
- **Description text cut off.** A wrapped label given a maximum width and centred in a layout received too little height. **Fix:** the label now spans the card's full width and centres its own text, in `PlaceholderPage::createComingSoonCard`.
- **Deprecated chart module.** Qt Charts is deprecated since Qt 6.10. **Decision:** draw simple charts ourselves, in later milestones.

---

## H. Code Walkthrough

### H1. What is the purpose of each major source file?

| File | Purpose |
|---|---|
| `src/main.cpp` | Starts the program, applies the theme, shows the window, runs the event loop |
| `src/core/Greeting.h/.cpp` | Chooses the time-of-day greeting; plain C++ |
| `src/ui/MainWindow.h/.cpp` | Builds the window, creates the pages, switches pages, sets the window size |
| `src/ui/Sidebar.h/.cpp` | Logo, tagline, 7 navigation buttons, version label; defines the page numbers |
| `src/ui/PageHeader.h/.cpp` | Reusable title and subtitle |
| `src/ui/PlaceholderPage.h/.cpp` | "Coming in Milestone N" page for features not built yet |
| `src/ui/Theme.h/.cpp` | Fusion style, light colours, font, loading the style sheet |
| `resources/styles/light.qss` | Colours, spacing and rounded corners |
| `tests/finora_tests.cpp` | Core logic tests |
| `tests/finora_ui_tests.cpp` | Interface tests |
| `CMakeLists.txt` | How to build the programs and tests |

### H2. Explain `greetingForHour` line by line.
```cpp
std::string greetingForHour(int hour)          // takes an int, returns text
{
    if (hour < 0 || hour > 23) {               // invalid hour?
        return "Hello";                        //   safe neutral greeting
    }
    if (hour >= 5 && hour < 12) {              // 05:00 to 11:59
        return "Good Morning";
    } else if (hour >= 12 && hour < 17) {      // 12:00 to 16:59
        return "Good Afternoon";
    } else {                                   // 17:00 to 04:59
        return "Good Evening";
    }
}
```

### H3. Explain `MainWindow::showPage` line by line.
```cpp
void MainWindow::showPage(int pageNumber)
{
    if (pageNumber < 0 || pageNumber >= PAGE_COUNT) {   // only 0 to 6 are pages
        return;                                         // ignore anything else
    }
    pageStack->setCurrentIndex(pageNumber);   // show that page
    sidebar->setCurrentPage(pageNumber);      // highlight its button
}
```

### H4. Explain `Sidebar::createNavButtons`.
It creates a `QButtonGroup` and makes it exclusive, so only one button can be selected. A `for` loop runs 7 times. Each time it creates a `QPushButton` with the label from `NAV_ITEMS[pageNumber]`, sets its icon and makes it checkable. It then adds the button to the group with id `pageNumber`, and adds it to the layout. After the loop, `connect` links the group's `idClicked` signal to `onNavButtonClicked`.

### H5. Trace a complete example from user action to result.
**Implemented trace, navigation:**
1. The user clicks "Budgets".
2. `QButtonGroup` emits `idClicked(2)`, because the Budgets button has id 2.
3. `Sidebar::onNavButtonClicked(2)` runs and emits `pageSelected(2)`.
4. `MainWindow::showPage(2)` runs. `2 < 0` is false and `2 >= 7` is false, so the number is valid.
5. `pageStack->setCurrentIndex(2)` shows the Budgets page.
6. `sidebar->setCurrentPage(2)` makes the Budgets button green.

**Financial trace:** from typing an amount to seeing a new total. **Planned — Not Yet Implemented** (Milestones 3 and 4).

### H6. Where does each syllabus concept appear?
See `docs/CPP_CONCEPTS_MAP.md`.

---

## I. Design Decisions and Limitations

### I1. Why did you choose this technology stack?
- **Short answer:** C++17 is the course language, Qt Widgets is a mature C++ GUI library named in the brief, CMake builds it, and SQLite needs no server.
- **Detailed explanation:** All of it was already installed and verified on this computer: Qt 6.12.0, MinGW 13.1.0, CMake 3.30.5, Ninja 1.12.1.

### I2. Why a local database rather than a cloud backend?
- **Short answer:** Simpler, private, and works without internet.
- **Status:** The database itself is **Planned — Not Yet Implemented** (Milestone 2).

### I3. Why is internet access not required?
- **Short answer:** Everything runs and is stored on the user's computer. Exchange rates are typed in by the user, and the program never claims they are live.

### I4. Why will money be stored as whole numbers?
- **Short answer:** Decimal fractions like 0.1 cannot be stored exactly in a `double`, so repeated additions can produce errors such as 0.30000000000000004.
- **Detailed explanation:** Storing PKR 1,200.50 as 120050 paisa in a `long long` keeps every addition exact. The rule is agreed and documented in `docs/DESIGN.md`, section 6. The code is **Planned — Not Yet Implemented** (Milestone 3).

### I5. Why not use Qt Charts?
- **Short answer:** Qt Charts is deprecated since Qt 6.10, and its replacement needs QML. Simple bar and line charts drawn with a few loops are easier to explain.
- **Status:** Charts are **Planned — Not Yet Implemented** (Milestones 4 and 6).

### I6. What limitations remain?
At Milestone 1 there are no finance features. The program has a light theme only. Build paths assume Qt is in `C:\Qt`. The greeting does not update while the window stays open.

### I7. What would you improve in a future version?
Answer this after the project is complete. Possible points: live exchange rates as an option, a dark theme, password protection, charts with more detail.

---

## J. Demonstration and Unexpected Questions

### J1. Suggested live demonstration, Milestone 1
1. Run `build.bat` and show "100% tests passed".
2. Start `build\debug\Finora.exe` by double-clicking it.
3. Point out the greeting and explain `greetingForHour`.
4. Click each sidebar button and show the page change. Explain the signal-and-slot chain from H5.
5. Open `src/ui/Sidebar.cpp` and show the `for` loop and the `NAV_ITEMS` array.
6. Run `build\debug\finora_tests.exe` and show the 11 PASS lines.

This sequence will be extended at every milestone.

### J2. Most likely difficult follow-up questions
- **"What happens if `showPage(9)` is called?"** The check `pageNumber >= PAGE_COUNT` is true, so the function returns and nothing changes.
- **"Why `hour < 12` and not `hour <= 12`?"** 12:00 is afternoon. With `<=`, the hour 12 would wrongly give "Good Morning". The test for hour 12 would catch this.
- **"Why is there no `delete` for the widgets you create with `new`?"** Qt's parent-child ownership deletes them; see C8.
- **"What is `this`?"** A pointer to the current object. `new QPushButton(..., this)` makes the current sidebar the button's parent.
- **"Why do the core files not include Qt?"** So the logic stays plain C++ and can be tested by a small console program.

### J3. Questions to check your own understanding
1. Which function runs first when the program starts?
2. What would happen if `PAGE_COUNT` were changed to 6 but `PAGE_INFO` still had 7 entries?
3. What does `navButtonGroup->setExclusive(true)` guarantee?
4. Why does `loadStyleSheet` return an empty string instead of stopping the program?
5. What does a test program's return value of 1 mean?

### J4. Challenge questions on edge cases and trade-offs
- **"Is the greeting wrong at midnight?"** No. Hour 0 is outside 5 to 16, so it gives "Good Evening". The test covers hour 0.
- **"What if the screen is smaller than the minimum window size?"** `setStartingSize` asks for 90% of the screen, but Qt never makes the window smaller than the minimum of 960 × 600.
- **"Why compile the interface files twice?"** The program and the interface test are two separate executables, and both need the same window code. They share the source files.

### J5. Did you use any outside help or tools?
- **How to answer:** Be fully honest. Name every tool and resource you used, for example the Qt documentation, course notes, and any AI assistant. Explain which parts you studied, changed or tested yourself.
- **Why it matters:** The academic integrity criterion, R4, rewards acknowledging external resources and being able to explain the work.

---

## Quick Revision

- **Finora:** offline Windows finance app in C++17 and Qt 6 Widgets; Milestone 1 of 7 is done.
- **Start-up:** `main` creates `QApplication`, calls `applyLightTheme`, creates `MainWindow`, calls `show()` and then `exec()`.
- **Navigation:** click, then `idClicked(id)`, then `onNavButtonClicked`, then `pageSelected(id)`, then `showPage(id)`.
- **Page numbers:** constants in `Sidebar.h`, 0 to 6; `PAGE_COUNT = 7`.
- **Greeting:** 5 to 11 Morning, 12 to 16 Afternoon, otherwise Evening; outside 0 to 23 gives "Hello".
- **Arrays of structures:** `NAV_ITEMS`, `PAGE_INFO`, test `cases`.
- **Loops:** `for` in `createNavButtons`, `createPages` and the tests.
- **Encapsulation:** private `navButtonGroup`; public `setCurrentPage` validates first.
- **Constructors:** build the widgets; the initializer list passes `parent` to the Qt base class.
- **Memory:** Qt parents delete their child widgets, so there is no `delete`.
- **File reading:** `loadStyleSheet` uses `QFile` open, `readAll` and close; a failure gives an empty string.
- **Tests:** 11 core checks and 22 interface checks, all passing on 27 October 2026.
- **Not built yet:** database, transactions, budgets, goals, currencies, reports. Say "planned for Milestone N".
