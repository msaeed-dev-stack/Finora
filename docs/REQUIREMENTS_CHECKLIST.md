# Finora Requirements Checklist

This checklist maps every requirement in the professor's project brief, *PF-Project-withRubrics-10Marks.docx*, to Finora. It lists only work that exists in the code. Anything not built yet is marked **Planned — Not Yet Implemented**.

Last updated: Milestone 3, 30 October 2026.

## 1. Objectives from the project brief

| # | Requirement in the brief | Finora feature | Source file and function | C++ concept | How it is tested | What to say in the viva | Status |
|---|---|---|---|---|---|---|---|
| 1 | Define a clear problem and application scope | Problem statement, objectives and limitations | `README.md` | None | Read by the instructor | "Finora helps a person record income and expenses, stay inside budgets and track savings, offline, without mixing currencies." | Done, draft |
| 2 | Design program logic using flowcharts and/or pseudocode | Flowcharts and pseudocode for start-up, navigation, greeting, window size, database opening, schema versioning, saving, leap years, date validation, reading amounts, filtering, and the add-transaction sequence | `docs/DESIGN.md` | None | Each flowchart matches the code it describes | "The pseudocode for `parseAmount` in DESIGN.md has the same steps as the function in `Money.cpp`." | Done for Milestones 1 to 3. More are added with each milestone |
| 3 | Implement core functionality with variables, operators, conditional statements, loops and functions | Greeting, page switching, validation, leap years, dates, reading and formatting amounts, search and filters | `greetingForHour`, `isLeapYear`, `daysInMonth`, `isValidIsoDate`, `validateTransaction`, `parseAmount`, `formatAmount`, `matchesFilter` | `if / else if / else`, `switch`, `for` and `while` loops, `%`, `&&`, `\|\|`, `break`, `continue`, functions | `finora_tests`: 115 checks | "`parseAmount` reads the amount one character at a time in a `for` loop and builds the number with `number * 10 + digit`." | Done for Milestones 1 to 3 |
| 4 | Apply data structures such as arrays, structures, vectors and files | Arrays of structures for buttons, pages and currencies; arrays of category names; the `Transaction` and `TransactionFilter` structures; a `std::vector` of transactions; the database file and the style sheet file | `SUPPORTED_CURRENCIES`, `EXPENSE_CATEGORIES`, `struct Transaction`, `struct TransactionFilter`, `loadTransactions`, `filterTransactions`, `loadStyleSheet` | Arrays, `struct`, `std::vector`, file reading, database file | Database tests: data survives closing and reopening. UI tests: the page shows the rows after a restart | "The page loads every transaction into a `std::vector<Transaction>`; the filters loop over that vector." | Done. CSV file writing **Planned — Not Yet Implemented** (Milestone 6) |
| 5 | Apply basic OOP: classes, objects, constructors, encapsulation, member functions | Interface classes, the form, the database class and the lock class | `MainWindow`, `Sidebar`, `TransactionsPage`, `TransactionDialog`, `DatabaseManager`, `InstanceLock` | Constructors and constructor overloading, a destructor, private data members, a reference member, public member functions, static member functions, copying disabled | UI and database tests create real objects | "`TransactionDialog` has two constructors: one for a new transaction and one that starts with an existing one." | Done. `Budget` and `SavingsGoal` **Planned — Not Yet Implemented** (Milestones 4 and 5) |
| 6 | Develop a console-based or GUI-based application with suitable C++ tools or libraries | Windows desktop GUI with Qt 6 Widgets and SQLite through Qt SQL | `src/main.cpp`, `src/ui/`, `src/database/` | `QApplication`, widgets, `QDialog`, `QTableWidget`, signals and slots, Qt SQL | `Finora.exe` starts without Qt on the PATH | "The brief lists Qt as a suitable GUI library. Finora uses Qt Widgets for windows and Qt SQL for SQLite." | Done |
| 7 | Integrate C++ with hardware or sensors, where applicable | Not applicable | None | None | None | "Finora is a desktop data-management program, so hardware integration does not apply." | Not applicable |
| 8 | Apply modular and structured programming | Separate folders and libraries for core logic, database, interface and tests | `src/core`, `src/database`, `src/ui`, `tests`, `CMakeLists.txt` | Header and source files, include guards, libraries, functions with one job | Everything builds with no compiler warnings reported | "All SQL is inside `DatabaseManager`. The Transactions page never contains SQL." | Done |
| 9 | Apply input validation, data handling and error management | The form refuses bad amounts, blank categories and impossible dates with a red message. The database checks again. Database failures show a warning and change nothing. Deleting asks for confirmation | `parseAmount`, `validateTransaction`, `TransactionDialog::onSaveClicked`, `TransactionsPage::showDatabaseError`, `onDeleteClicked` | Guard `if` statements, `bool` results, error messages | Core tests: 22 typed amounts. UI tests: the form refuses 4 kinds of bad input; a closed database gives a warning | "Data is checked three times: in the form, in `DatabaseManager`, and by SQLite's own rules." | Done |
| 10 | Demonstrate the completed application through implementation, demonstration and presentation | Viva guide and a Milestone 3 demonstration guide | `docs/VIVA_PREPARATION.md`, `docs/DEMO_MILESTONE3.md` | None | Practice the demo | "I can add, edit, filter and delete a transaction, then restart Finora and show it is still there." | Started |

## 2. Finora's functional requirements

| Requirement | Milestone | Status |
|---|---|---|
| Buildable window with navigation | 1 | Done |
| SQLite database, schema version 1, error handling, data kept after restart | 2 | Done |
| Database location in AppLocalDataLocation; path can be given for tests | 2 | Done |
| SQLite driver deployed next to `Finora.exe` | 2 | Done |
| Single-instance guard with Qt's `QLockFile`, added before any screen can edit or save | 3, first task | Done: `InstanceLock` in `main.cpp`; tested in `finora_db_tests` and with two real copies of Finora |
| List transactions from SQLite, newest first | 3 | Done: `loadTransactions`, `TransactionsPage::showTransactions` |
| Add income and expense transactions through a form | 3 | Done: `TransactionDialog`, `saveNewTransaction` |
| Edit existing transactions | 3 | Done: `TransactionDialog(existing)`, `updateTransaction` |
| Delete transactions with confirmation | 3 | Done: `QMessageBox::question`, `deleteTransaction` |
| Search by description or category; filter by type, currency, category and period | 3 | Done: `TransactionFilter`, `filterTransactions` |
| Validate amounts, currencies, categories and dates; success and error messages | 3 | Done |
| Data still available after restarting Finora | 3 | Done: tested with a real SQLite file in `finora_ui_tests` and `finora_db_tests` |
| Dashboard summaries per currency with checked totals; recent transactions | 4 | **Planned — Not Yet Implemented** |
| Monthly category budgets as schema version 2, 80% warning and 100% alert | 4 | **Planned — Not Yet Implemented** |
| Savings goals with safe contributions and progress | 5 | **Planned — Not Yet Implemented** |
| PKR, USD and EUR with manual exchange rates and documented rounding | 5 | **Planned — Not Yet Implemented** |
| Reports by period, category and type; CSV export with safe quoting | 6 | **Planned — Not Yet Implemented** |
| Final testing, documentation, Windows package | 7 | **Planned — Not Yet Implemented** |

## 3. Rubric evidence

| Rubric item | Marks | Evidence so far |
|---|---|---|
| R1 Project idea, design and creativity | 2 | Problem statement in `README.md`. Architecture, flowcharts, pseudocode, a sequence diagram and the database design in `docs/DESIGN.md`. A custom interface that follows the Finora reference design |
| R2 C++ implementation, OOP and functionality | 3 | Classes, structures, arrays, vectors, loops, `switch`, functions, file handling and a SQLite database with full add, edit, delete, search and filter. No compiler warnings reported. Three automated test programs with 284 checks |
| R3 Teamwork, collaboration and professional responsibility | 2 | Individual project. Milestone-by-milestone Git history, honest status labels and documentation. Ask the instructor how this item is judged for individual work |
| R4 Academic integrity, learning attitude, engagement and demonstration | 3 | Viva preparation guide and a demonstration guide. Planned features are labelled honestly in the program and the documents |
