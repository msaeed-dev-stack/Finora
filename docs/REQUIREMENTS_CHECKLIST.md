# Finora Requirements Checklist

This checklist maps every requirement in the professor's project brief, *PF-Project-withRubrics-10Marks.docx*, to Finora. It lists only work that exists in the code. Anything not built yet is marked **Planned — Not Yet Implemented**.

Last updated: Milestone 1, 27 October 2026.

## 1. Objectives from the project brief

| # | Requirement in the brief | Finora feature | Source file and function | C++ concept | How it is tested | What to say in the viva | Status |
|---|---|---|---|---|---|---|---|
| 1 | Define a clear problem and application scope | Problem statement, objectives and limitations | `README.md` | None | Read by the instructor | "Finora helps a person record income and expenses, stay inside budgets and track savings, offline, without mixing currencies." | Done, draft |
| 2 | Design program logic using flowcharts and/or pseudocode | Flowcharts and pseudocode for start-up, page navigation, greeting and window size | `docs/DESIGN.md` | None | Each flowchart matches the code it describes | "DESIGN.md shows the steps of each algorithm. For example, the greeting flowchart has the same three decisions as `greetingForHour`." | Started. More flowcharts are added with each milestone |
| 3 | Implement core functionality with variables, operators, conditional statements, loops and functions | Greeting, page switching, window sizing | `greetingForHour`, `MainWindow::showPage`, `MainWindow::setStartingSize`, `Sidebar::createNavButtons` | `if / else if / else`, `&&`, `\|\|`, `for` loops, arithmetic, functions | `finora_tests`: 11 checks. `finora_ui_tests`: 22 checks | "`greetingForHour` uses relational and logical operators in an if-else chain. `createNavButtons` creates seven buttons with a for loop." | Done for Milestone 1 features |
| 4 | Apply data structures such as arrays, structures, vectors and files | Arrays of structures describe the buttons and pages. A style sheet file is read at start-up | `NAV_ITEMS` in `Sidebar.cpp`, `PAGE_INFO` in `MainWindow.cpp`, `loadStyleSheet` in `Theme.cpp` | Arrays, `struct`, file reading with `QFile` | UI test checks 7 pages and 7 buttons and that the style sheet loads | "Each page's text is one `PageInfo` structure. The seven structures sit in an array indexed by page number." | Arrays, structures and file reading done. Vectors **Planned — Not Yet Implemented** (Milestone 3). Database and CSV files **Planned — Not Yet Implemented** (Milestones 2 and 6) |
| 5 | Apply basic OOP: classes, objects, constructors, encapsulation, member functions | Interface classes | `MainWindow`, `Sidebar`, `PageHeader`, `PlaceholderPage` | Classes, constructors with initializer lists, private data members, public member functions | UI test creates a real `MainWindow` object and uses it | "`Sidebar` keeps its button group private. Other classes can only call `setCurrentPage`." | Done for the interface. Finance classes such as `Transaction`, `Budget` and `SavingsGoal` **Planned — Not Yet Implemented** (Milestones 3 to 5) |
| 6 | Develop a console-based or GUI-based application with suitable C++ tools or libraries | Windows desktop GUI with Qt 6 Widgets | `src/main.cpp`, `src/ui/` | Qt `QApplication`, widgets, signals and slots | `Finora.exe` starts without Qt on the PATH. The UI test passes | "The brief lists Qt as a suitable GUI library. Finora uses Qt Widgets, which gives native Windows windows." | Done |
| 7 | Integrate C++ with hardware or sensors, where applicable | Not applicable | None | None | None | "Finora is a desktop data-management program, so hardware integration does not apply." | Not applicable |
| 8 | Apply modular and structured programming | Separate folders and files for core logic, interface and tests. Core logic is a library | `src/core`, `src/ui`, `tests`, `CMakeLists.txt` | Header and source files, include guards, functions with one job | Everything builds with zero warnings | "Each file has one job. The core library has no Qt, so it can be tested on its own." | Done |
| 9 | Apply input validation, data handling and error management | Invalid hours and page numbers are rejected. A missing style sheet does not crash the program | `greetingForHour`, `MainWindow::showPage`, `Sidebar::setCurrentPage`, `loadStyleSheet` | Guard `if` statements, `bool` return values | Core tests for hours -1 and 24 | "Before using a page number, `showPage` checks it is between 0 and 6." | Started. Amount, date and form validation **Planned — Not Yet Implemented** (Milestone 3) |
| 10 | Demonstrate the completed application through implementation, demonstration and presentation | Viva guide and demo sequence | `docs/VIVA_PREPARATION.md` | None | Practice the demo sequence | "I will show navigation now; each milestone adds a feature to the demo." | Started |

## 2. Finora's functional requirements

| Requirement | Milestone | Status |
|---|---|---|
| Buildable window with navigation | 1 | Done |
| SQLite database, schema, error handling, data kept after restart | 2 | **Planned — Not Yet Implemented** |
| Add, edit, delete, list, search and filter transactions, with validation | 3 | **Planned — Not Yet Implemented** |
| Dashboard summaries per currency, recent transactions | 4 | **Planned — Not Yet Implemented** |
| Monthly category budgets, 80% warning and 100% alert | 4 | **Planned — Not Yet Implemented** |
| Savings goals with safe contributions and progress | 5 | **Planned — Not Yet Implemented** |
| PKR, USD and EUR with manual exchange rates and documented rounding | 5 | **Planned — Not Yet Implemented** |
| Reports by period, category and type; CSV export with safe quoting | 6 | **Planned — Not Yet Implemented** |
| Final testing, documentation, Windows package | 7 | **Planned — Not Yet Implemented** |

## 3. Rubric evidence

| Rubric item | Marks | Evidence so far |
|---|---|---|
| R1 Project idea, design and creativity | 2 | Problem statement in `README.md`. Architecture, flowcharts and pseudocode in `docs/DESIGN.md`. Custom interface design with hand-drawn icons |
| R2 C++ implementation, OOP and functionality | 3 | Classes, structures, arrays, loops, functions and file reading in `src/`. Zero compiler warnings. Two automated test programs |
| R3 Teamwork, collaboration and professional responsibility | 2 | Individual project. Milestone-by-milestone Git history, honest status labels and documentation. Ask the instructor how this item is judged for individual work |
| R4 Academic integrity, learning attitude, engagement and demonstration | 3 | Viva preparation guide. Planned features are labelled honestly in the program and the documents |
