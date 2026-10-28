# Finora Requirements Checklist

This checklist maps every requirement in the professor's project brief, *PF-Project-withRubrics-10Marks.docx*, to Finora. It lists only work that exists in the code. Anything not built yet is marked **Planned — Not Yet Implemented**.

Last updated: Milestone 2, 28 October 2026.

## 1. Objectives from the project brief

| # | Requirement in the brief | Finora feature | Source file and function | C++ concept | How it is tested | What to say in the viva | Status |
|---|---|---|---|---|---|---|---|
| 1 | Define a clear problem and application scope | Problem statement, objectives and limitations | `README.md` | None | Read by the instructor | "Finora helps a person record income and expenses, stay inside budgets and track savings, offline, without mixing currencies." | Done, draft |
| 2 | Design program logic using flowcharts and/or pseudocode | Flowcharts and pseudocode for start-up, navigation, greeting, window size, database opening, schema versioning, saving, leap years and date validation | `docs/DESIGN.md` | None | Each flowchart matches the code it describes | "The date-validation pseudocode in DESIGN.md has the same steps as `isValidIsoDate`." | Done for Milestones 1 and 2. More flowcharts are added with each milestone |
| 3 | Implement core functionality with variables, operators, conditional statements, loops and functions | Greeting, page switching, window sizing, transaction validation, leap years, date checking | `greetingForHour`, `MainWindow::showPage`, `isLeapYear`, `daysInMonth`, `isValidIsoDate`, `validateTransaction` | `if / else if / else`, `switch`, `for` and `while` loops, `%`, `&&`, `\|\|`, functions | `finora_tests`: 59 checks | "`isLeapYear` uses the remainder operator `%`. `daysInMonth` uses a `switch`." | Done for Milestones 1 and 2 |
| 4 | Apply data structures such as arrays, structures, vectors and files | Arrays of structures for buttons, pages and currencies. A `Transaction` structure. A database file and a style sheet file | `NAV_ITEMS`, `PAGE_INFO`, `SUPPORTED_CURRENCIES`, `struct Transaction`, `DatabaseManager`, `loadStyleSheet` | Arrays, `struct`, file reading, database file | UI test: 7 pages and buttons. Database test: data survives closing and reopening the file | "Each transaction is one `Transaction` structure. It is saved as one row in the `transactions` table of the SQLite file." | Arrays, structures, files and database done. Vectors **Planned — Not Yet Implemented** (Milestone 3). CSV file writing **Planned — Not Yet Implemented** (Milestone 6) |
| 5 | Apply basic OOP: classes, objects, constructors, encapsulation, member functions | Interface classes and the database class | `MainWindow`, `Sidebar`, `PageHeader`, `PlaceholderPage`, `DatabaseManager` | Constructors, a destructor, private data members, public member functions, a static member function, copying disabled | UI test and database test create real objects | "`DatabaseManager` keeps its connection name and state private. Its destructor closes the database automatically." | Done. `Budget` and `SavingsGoal` **Planned — Not Yet Implemented** (Milestones 4 and 5) |
| 6 | Develop a console-based or GUI-based application with suitable C++ tools or libraries | Windows desktop GUI with Qt 6 Widgets and a SQLite database through Qt SQL | `src/main.cpp`, `src/ui/`, `src/database/` | Qt `QApplication`, widgets, signals and slots, Qt SQL | `Finora.exe` starts without Qt on the PATH and creates its database | "The brief lists Qt as a suitable GUI library. Finora uses Qt Widgets for windows and Qt SQL for SQLite." | Done |
| 7 | Integrate C++ with hardware or sensors, where applicable | Not applicable | None | None | None | "Finora is a desktop data-management program, so hardware integration does not apply." | Not applicable |
| 8 | Apply modular and structured programming | Separate folders and libraries for core logic, database, interface and tests | `src/core`, `src/database`, `src/ui`, `tests`, `CMakeLists.txt` | Header and source files, include guards, libraries, functions with one job | Everything builds with no compiler warnings reported | "All SQL is inside `DatabaseManager`. The interface never sees SQL classes." | Done |
| 9 | Apply input validation, data handling and error management | Invalid transactions are rejected with clear messages. Database failures show a dialog and stop the program. SQLite rules block bad data even from raw SQL | `validateTransaction`, `DatabaseManager::open`, `addTransaction`, `findTransactionById`, `showDatabaseError` in `main.cpp` | Guard `if` statements, `bool` results, error messages | Database test: 10 invalid transactions rejected, 4 kinds of open failure, SQLite CHECK and FOREIGN KEY rules | "Data is checked twice: first in C++, then by SQLite's CHECK and FOREIGN KEY rules." | Done for the database layer. Form validation **Planned — Not Yet Implemented** (Milestone 3) |
| 10 | Demonstrate the completed application through implementation, demonstration and presentation | Viva guide and demo sequence | `docs/VIVA_PREPARATION.md` | None | Practice the demo sequence | "I can show the database file being created and the tests that prove it works." | Started |

## 2. Finora's functional requirements

| Requirement | Milestone | Status |
|---|---|---|
| Buildable window with navigation | 1 | Done |
| SQLite database, schema version 1, error handling, data kept after restart | 2 | Done |
| Database location in AppLocalDataLocation; path can be given for tests | 2 | Done |
| SQLite driver deployed next to `Finora.exe` | 2 | Done |
| Single-instance guard with Qt's `QLockFile`, added before any screen can edit or save | 3, first task | Done: `InstanceLock` in `main.cpp`, tested in `finora_db_tests` |
| Add, edit, delete, list, search and filter transactions on screen, with validation | 3 | **Planned — Not Yet Implemented** |
| Dashboard summaries per currency with checked totals; recent transactions | 4 | **Planned — Not Yet Implemented** |
| Monthly category budgets as schema version 2, 80% warning and 100% alert | 4 | **Planned — Not Yet Implemented** |
| Savings goals with safe contributions and progress | 5 | **Planned — Not Yet Implemented** |
| PKR, USD and EUR with manual exchange rates and documented rounding | 5 | **Planned — Not Yet Implemented** |
| Reports by period, category and type; CSV export with safe quoting | 6 | **Planned — Not Yet Implemented** |
| Final testing, documentation, Windows package | 7 | **Planned — Not Yet Implemented** |

## 3. Rubric evidence

| Rubric item | Marks | Evidence so far |
|---|---|---|
| R1 Project idea, design and creativity | 2 | Problem statement in `README.md`. Architecture, flowcharts, pseudocode and the database design in `docs/DESIGN.md`. Custom interface with hand-drawn icons |
| R2 C++ implementation, OOP and functionality | 3 | Classes, structures, arrays, loops, `switch`, functions, file handling and a SQLite database. No compiler warnings reported. Three automated test programs with 143 checks |
| R3 Teamwork, collaboration and professional responsibility | 2 | Individual project. Milestone-by-milestone Git history, honest status labels and documentation. Ask the instructor how this item is judged for individual work |
| R4 Academic integrity, learning attitude, engagement and demonstration | 3 | Viva preparation guide. Planned features are labelled honestly in the program and the documents |
