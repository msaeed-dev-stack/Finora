# C++ Concepts Map

Where each C++ concept appears in Finora's real source code. Only concepts that exist in the code are listed in the first table. Concepts that are not used yet are listed separately, with the milestone where they will appear naturally.

Last updated: Milestone 1, 27 October 2026.

## Concepts used in the code

| C++ Concept | Where It Is Used | Example Function or File | Why It Is Used | How I Can Explain It |
|---|---|---|---|---|
| Variables | `int hour`, `int width`, `int height`, `int maxWidth`, `int maxHeight` | `MainWindow::createPages`, `MainWindow::setStartingSize` | To hold values that are worked out while the program runs | "A variable is a named box in memory. `hour` holds the current hour so the greeting can be chosen." |
| Constants | `const int PAGE_COUNT = 7`, `PAGE_DASHBOARD = 0` up to `PAGE_SETTINGS = 6`, `const int CASE_COUNT = 11` | `src/ui/Sidebar.h`, `tests/finora_tests.cpp` | Numbers that must never change get one name, so a typo cannot change them | "If I add a page, I change `PAGE_COUNT` in one place and every loop uses the new value." |
| `int` | Hours, page numbers, sizes, milestone numbers, test counters | `greetingForHour(int hour)`, `PageInfo::milestone` | Whole numbers | "Hours and page numbers are whole numbers, so `int` fits." |
| `bool` | Return value of `applyLightTheme`, the `passed` parameter of `check` | `src/ui/Theme.cpp`, `tests/finora_ui_tests.cpp` | A yes or no answer | "`applyLightTheme` returns `true` if the style sheet loaded." |
| `std::string` | Greeting text, test names | `greetingForHour`, `checkText` | Text in the plain C++ core | "The core uses the standard `string` class, so it does not depend on Qt." |
| C-strings (`const char *`) | Labels and icon names in the structures | `NavItem`, `PageInfo` | Fixed text that never changes | "`const char *` points to text that is built into the program." |
| `QString` | All text shown in the window | `PageHeader`, `PlaceholderPage` | Qt widgets need Qt's string class | "`QString` is Qt's version of `string`. I convert between the two where the core meets the interface." |
| Arithmetic operators | `*`, `/`, `+`, `-` | `MainWindow::setStartingSize` | To compute 90% of the screen and the centre position | "`screenArea.width() * 9 / 10` is 90% using whole-number arithmetic." |
| Relational operators | `<`, `>`, `>=`, `==` | `greetingForHour`, `MainWindow::showPage`, tests | To compare values | "`hour >= 5 && hour < 12` checks that the hour is in the morning range." |
| Logical operators | `&&`, `\|\|`, `!` | `greetingForHour`, `MainWindow::showPage`, `loadStyleSheet` | To combine or reverse conditions | "`hour < 0 \|\| hour > 23` is true when the hour is outside the valid range." |
| Assignment and increment | `=`, `++` | Every `for` loop, `TestCounter` updates | To store values and count | "`counter.checksRun++` adds one after each check." |
| Type conversion | `QString::fromStdString`, `QString::number`, `std::to_string`, `toStdString` | `MainWindow::createPages`, `PlaceholderPage::createComingSoonCard`, tests | To move values between `int`, `std::string` and `QString` | "`QString::number(milestone)` turns the number 3 into the text "3"." |
| `if / else if / else` | Three-way choice of greeting | `greetingForHour` | To choose one result out of several | "Only the first true condition runs, so each hour gets exactly one greeting." |
| Guard `if` with early `return` | Rejects invalid page numbers and missing files | `MainWindow::showPage`, `Sidebar::setCurrentPage`, `loadStyleSheet` | To stop before bad data is used | "If the page number is not 0 to 6, the function returns at once." |
| `for` loops | Create 7 buttons, create 7 pages, run 11 test cases, count selected buttons | `Sidebar::createNavButtons`, `MainWindow::createPages`, `testGreetingForHour`, `countCheckedButtons` | To repeat the same steps a known number of times | "The loop runs from 0 to `PAGE_COUNT - 1` and makes one button each time." |
| Functions | Each task has its own function | `greetingForHour`, `loadStyleSheet`, `applyLightTheme`, `checkText` | One job per function makes code readable and testable | "`greetingForHour` only chooses a greeting, so I can test it alone." |
| Parameters and return values | `greetingForHour(int hour)` returns `std::string` | `src/core/Greeting.cpp` | To pass data in and get a result back | "I pass in 9 and get back "Good Morning"." |
| Pass by reference | `TestCounter &counter`, `const QString &title` | `checkText`, `PageHeader` constructor | To let a function update the caller's variable, or to avoid copying | "`&` means the function works on the original counter, not a copy." |
| Default arguments | `QWidget *parent = nullptr` | Every interface class constructor | The caller may leave out the parent | "If no parent is given, the widget becomes a top-level window." |
| Arrays | `NAV_ITEMS[PAGE_COUNT]`, `PAGE_INFO[PAGE_COUNT]`, `cases[CASE_COUNT]` | `Sidebar.cpp`, `MainWindow.cpp`, `finora_tests.cpp` | To keep a fixed list of items in order | "`NAV_ITEMS[2]` is the Budgets button because the index is the page number." |
| Structures | `NavItem`, `PageInfo`, `TestCounter`, `GreetingCase` | `Sidebar.cpp`, `MainWindow.cpp`, tests | To group related values into one item | "`PageInfo` groups a page's title, subtitle, icon, milestone and description." |
| Classes and objects | `MainWindow`, `Sidebar`, `PageHeader`, `PlaceholderPage` | `src/ui/` | Each part of the screen is one class | "`MainWindow window;` in `main()` creates one object of the class." |
| Constructors and initializer lists | `MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)` | Every interface class | To build the object's widgets when it is created | "The constructor runs automatically and builds the sidebar and pages." |
| Encapsulation | Private data members `sidebar`, `pageStack`, `navButtonGroup`; public `setCurrentPage` | `MainWindow.h`, `Sidebar.h` | Other code cannot change the internals directly | "Other classes cannot touch the button group. They must call `setCurrentPage`." |
| Member functions | `Sidebar::setCurrentPage`, `MainWindow::showPage`, `PlaceholderPage::createComingSoonCard` | `src/ui/` | Behaviour that belongs to one class | "A member function works on its own object's data." |
| Inheritance from Qt classes | `class Sidebar : public QFrame`, `class MainWindow : public QMainWindow` | `src/ui/*.h` | Qt widgets are built by extending Qt's classes | "My `Sidebar` is a `QFrame` with extra buttons. I do not build my own class hierarchy." |
| Pointers, `new` and `nullptr` | `new QPushButton(...)`, `Sidebar *sidebar = nullptr` | Every interface constructor | Qt widgets are created with `new` and owned by their parent | "The parent deletes its child widgets automatically when it closes, so I never call `delete`." |
| Header files and include guards | `#ifndef FINORA_SIDEBAR_H` | Every `.h` file | Declarations in `.h`, code in `.cpp`, no double inclusion | "The guard stops the same header being included twice in one file." |
| Modular programming | `src/core`, `src/ui`, `tests`, library `finora_core` | `CMakeLists.txt` | Small files with one purpose | "The core library is shared by the program and the tests." |
| File handling: reading | `QFile::open` and `readAll` | `loadStyleSheet` in `Theme.cpp` | To read the style sheet file | "It opens the file read-only, reads all the text, closes it, and returns the text." |
| Input validation | Hours outside 0 to 23, page numbers outside 0 to 6 | `greetingForHour`, `MainWindow::showPage` | To stop bad values causing wrong results | "An hour of 24 returns "Hello" instead of a wrong greeting." |
| Error handling | A missing style sheet returns an empty string, and `applyLightTheme` returns `false` | `Theme.cpp` | The program keeps working when a file is missing | "If the file cannot be opened, Finora keeps Qt's default look instead of crashing." |
| Signals and slots | `connect(sidebar, &Sidebar::pageSelected, this, &MainWindow::showPage)` | `MainWindow` constructor, `Sidebar::createNavButtons` | Qt's way to call a function when something happens | "When the sidebar emits `pageSelected`, Qt calls `showPage` automatically." |
| Program exit codes | Test programs return 0 for success and 1 for failure | `tests/*.cpp` | CTest reads the code to report pass or fail | "Returning 1 tells the build script that a test failed." |

## Concepts not used yet

| C++ Concept | Status | Where it will appear naturally |
|---|---|---|
| `double` | **Planned — Not Yet Implemented** | Exchange-rate display and percentages, Milestones 4 and 5. Money itself will use whole numbers in `long long` |
| `long long` | **Planned — Not Yet Implemented** | Money amounts in paisa or cents, Milestone 3 |
| `char` as a single character | **Planned — Not Yet Implemented** | Checking each typed character of an amount while parsing it, Milestone 3 |
| `switch` | **Planned — Not Yet Implemented** | Choosing the text and colour for a budget status, Milestone 4 |
| `while` loop | **Planned — Not Yet Implemented** | Reading database rows one by one, Milestone 2 |
| `do-while` loop | **Planned — Not Yet Implemented** | No natural place found yet. Please confirm whether the syllabus needs it before an example is added |
| `std::vector` | **Planned — Not Yet Implemented** | The list of transactions loaded from the database, Milestone 3 |
| Database operations | **Planned — Not Yet Implemented** | SQLite through Qt SQL, Milestone 2 |
| File handling: writing | **Planned — Not Yet Implemented** | CSV export with `std::ofstream`, Milestone 6 |
