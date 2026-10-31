# Finora Requirements Checklist

This checklist maps every requirement in the professor's project brief, *PF-Project-withRubrics-10Marks.docx*, to Finora. It lists only work that exists in the code. Anything not built yet is marked **Planned — Not Yet Implemented**.

Last updated: Milestone 4, Phase 4B, 30 October 2026.

## 1. Objectives from the project brief

| # | Requirement in the brief | Finora feature | Source file and function | C++ concept | How it is tested | What to say in the viva | Status |
|---|---|---|---|---|---|---|---|
| 1 | Define a clear problem and application scope | Problem statement, objectives and limitations | `README.md` | None | Read by the instructor | "Finora helps a person record income and expenses, stay inside budgets and track savings, offline, without mixing currencies." | Done, draft |
| 2 | Design program logic using flowcharts and/or pseudocode | Flowcharts and pseudocode for start-up, navigation, greeting, window size, database opening, schema versioning, saving, leap years, date validation, reading amounts, filtering, the add-transaction sequence, the safe database upgrade, checked totals, budget rules, the dashboard data flow, drawing the chart and switching themes | `docs/DESIGN.md` | None | Each flowchart matches the code it describes | "The pseudocode for `budgetStatusFor` in DESIGN.md section 10.5 has the same four steps as the function in `Budget.cpp`." | Done for Milestones 1 to 4A. More are added with each phase |
| 3 | Implement core functionality with variables, operators, conditional statements, loops and functions | Greeting, page switching, validation, leap years, dates, reading and formatting amounts, search and filters, checked totals, month arithmetic, budget status and percentages, the chart scale | `greetingForHour`, `isLeapYear`, `daysInMonth`, `isValidIsoDate`, `validateTransaction`, `parseAmount`, `formatAmount`, `matchesFilter`, `addChecked`, `shiftMonth`, `summarizeByCurrency`, `budgetStatusFor`, `percentUsed`, `chartScaleMaximum` | `if / else if / else`, `switch`, `for` and `while` loops, `%`, `/`, `&&`, `\|\|`, `break`, `continue`, functions | `finora_tests`: 338 checks | "`addChecked` compares the amount with `LLONG_MAX - total` before adding, so a total can never wrap around to a wrong number." | Done for Milestones 1 to 4A |
| 4 | Apply data structures such as arrays, structures, vectors and files | Arrays of structures for buttons, pages, currencies and fonts; the `Transaction`, `TransactionFilter`, `Budget`, `BudgetProgress`, `CurrencySummary`, `MonthFlow`, `CategoryTotal` and `ThemeColors` structures; vectors of transactions, budgets and pages; the `BudgetStatus` enum; the database file, the backup file, the style sheet and the font files | `SUPPORTED_CURRENCIES`, `NAV_ITEMS`, `struct Budget`, `enum BudgetStatus`, `std::vector<Page *> pages`, `loadTransactions`, `loadBudgets`, `loadStyleSheet`, `backupBeforeUpgrade` | Arrays, `struct`, `enum`, `std::vector`, file reading, database file | Database tests: data survives closing and reopening; the backup file is checked. UI tests: the pages show the saved rows | "`allBudgetProgress` loops over a `std::vector<Budget>` and returns a `std::vector<BudgetProgress>`, one result per budget." | Done. CSV file writing **Planned — Not Yet Implemented** (Milestone 6) |
| 5 | Apply basic OOP: classes, objects, constructors, encapsulation, member functions | Interface classes, forms, the database classes, the lock, reusable components, a page base class, the account classes (`Session` with its friend class `AuthService`), and the billing classes (the abstract `PaymentGateway`, `SubscriptionService` as the friend of `SubscriptionStore`) | `MainWindow`, `Sidebar`, `Page`, `DashboardPage`, `TransactionsPage`, `BudgetsPage`, `SettingsPage`, `TransactionDialog`, `BudgetDialog`, `WalletCard`, `MoneyFlowChart`, `ProgressRing`, `DatabaseManager`, `BudgetStore`, `InstanceLock` | Constructors and constructor overloading, a destructor, private data members, reference members, static member functions, copying disabled, **inheritance** (every page inherits from `Page`), **polymorphism** (`virtual void refresh()` and `paintEvent` overridden), **composition** (`MainWindow` has a `BudgetStore`) | UI and database tests create real objects | "`MainWindow` keeps every page as a `Page *` and calls `refresh()`. C++ runs the right page's own version at run time." | Done. `SavingsGoal` **Planned — Not Yet Implemented** (Milestone 5) |
| 6 | Develop a console-based or GUI-based application with suitable C++ tools or libraries | Windows desktop GUI with Qt 6 Widgets, Qt Svg and SQLite through Qt SQL | `src/main.cpp`, `src/ui/`, `src/database/` | `QApplication`, widgets, `QDialog`, `QTableWidget`, `QPainter`, signals and slots, Qt SQL | `Finora.exe` starts without Qt on the PATH | "The brief lists Qt as a suitable GUI library. Finora uses Qt Widgets for windows, QPainter for its own chart and Qt SQL for SQLite." | Done |
| 7 | Integrate C++ with hardware or sensors, where applicable | Not applicable | None | None | None | "Finora is a desktop data-management program, so hardware integration does not apply." | Not applicable |
| 8 | Apply modular and structured programming | Separate folders and libraries for core logic, database, interface and tests; reusable components | `src/core`, `src/database`, `src/ui`, `src/ui/components`, `tests`, `CMakeLists.txt` | Header and source files, include guards, libraries, functions with one job | Everything builds with no compiler warnings reported | "All SQL is inside `src/database`. The pages never contain SQL, and they get every number from the core functions." | Done |
| 9 | Apply input validation, data handling and error management | Forms refuse bad amounts, blank categories, impossible dates and duplicate budgets with a red message. The database checks again. Database failures show a warning and change nothing. Deleting asks first. An upgrade makes a checked backup first and rolls back completely on any failure | `parseAmount`, `validateTransaction`, `validateBudget`, `BudgetDialog::readForm`, `BudgetStore::checkNoDuplicate`, `upgradeSchema`, `backupBeforeUpgrade`, `verifyBackup` | Guard `if` statements, `bool` results, error messages, database transactions | Core tests: typed amounts and budget rules. UI tests: forms refuse bad input. DB tests: two deliberate upgrade failures leave the file byte-for-byte unchanged | "Data is checked three times: in the form, in C++ before any SQL, and by SQLite's own rules." | Done |
| 10 | Demonstrate the completed application through implementation, demonstration and presentation | Viva guide and demonstration guides | `docs/VIVA_PREPARATION.md`, `docs/DEMO_MILESTONE3.md`, `docs/DEMO_MILESTONE4.md` | None | Practise the demo | "I can add a transaction on the dashboard, watch the wallet card and the budget alerts change, and switch to the dark theme." | Started |

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
| Design system: violet brand, light and dark theme, bundled fonts with licences, themed icons, focus rings, reduce motion | 4A | Done: `Theme`, `finora.qss`, `THIRD_PARTY_NOTICES.md` |
| Schema version 2 with a verified backup, one transaction, row and amount checks, foreign key check and rollback after a deliberate failure | 4A | Done: `upgradeSchema`, `upgradeToVersion2`; 3 upgrade tests in `finora_db_tests` |
| Ask before upgrading an older database file | 4A | Done: `askToUpgrade` in `main.cpp` |
| Dashboard: wallet card per currency, this month's income and spending, money-flow chart, recent transactions, budget alerts, spending by category | 4A | Done: `DashboardPage`, `WalletCard`, `MoneyFlowChart` |
| Totals per currency with checked addition; never mix currencies | 4A | Done: `addChecked`, `summarizeByCurrency` |
| Monthly category budgets: create, edit, delete; spent, left, progress; 80% warning, 100% reached, over budget | 4A | Done: `BudgetsPage`, `BudgetDialog`, `BudgetStore`, `budgetStatusFor` |
| Sign-up, sign-in, sign-out with Argon2id password hashes (libsodium, MODERATE), strength meter, general error messages and waiting times | 4B | Done: `AuthService`, `PasswordHasher`, `PasswordPolicy`, `AuthWindow`; 106 account checks |
| Sessions: sign-out, 15 minutes idle, 12-hour limit, crash recovery | 4B | Done: `AuthService`, `AppController` |
| Profile and password change; recovery codes (shown once, hashed, single use) | 4B | Done: `AccountDialogs`, `UserStore` |
| One person's data per account; the user id comes only from the Session | 4B | Done: `DatabaseManager::currentOwner`, `user_id IS :owner` in every query; tested with two accounts |
| Records made before accounts move into an account only with the person's consent | 4B | Done: `claimRecordsWithoutOwner`, the question after sign-up, the Settings card |
| Free, Plus, Pro and Business plans; Free for every new and existing account; schema version 3 with a verified backup | 4C | Done: `Plans`, `SubscriptionStore`, `upgradeToVersion3`; tested on a real version 2 file with accounts |
| One table of entitlements, checked inside the operations (not only on screen); 3 active budgets on Free, the rest paused, never deleted | 4C | Done: `requireFeature`, `BudgetStore`, `ReportExporter`, `loadAuditHistory`; tests call them without any screen |
| Subscription page, plan cards, comparison, billing history, locked-feature explanations, light and dark | 4C | Done: `SubscriptionPage`, `askToSeePlans`, lock icons |
| Simulated checkout (no card fields, no real money); upgrade, smaller plan at renewal, cancel, resume, renewal with grace, 14-day Pro trial, crash recovery, `--billing-demo` | 4C | Done: `PaymentGateway` (abstract), `SimulatedPaymentGateway`, `SubscriptionService`, `CheckoutDialog`; 161 billing checks |
| CSV export (Plus), monthly PDF summary (Pro), audit history (Business) | 4C | Done: `CsvExport`, `ReportExporter` (`QSaveFile`, `QPdfWriter`), `AuditHistoryDialog` |
| Business team members, shared workspaces and roles | 4C | **Planned — Not Yet Implemented** (needs Finora Online, a server); marked "Online" in the program |
| Savings goals with safe contributions and progress | 5 | **Planned — Not Yet Implemented** |
| PKR, USD and EUR with manual exchange rates and documented rounding | 5 | **Planned — Not Yet Implemented** |
| Reports by period, category and type; CSV export with safe quoting | 6 | **Planned — Not Yet Implemented** |
| Final testing, documentation, Windows package | 7 | **Planned — Not Yet Implemented** |

## 3. Rubric evidence

| Rubric item | Marks | Evidence so far |
|---|---|---|
| R1 Project idea, design and creativity | 2 | Problem statement in `README.md`. Architecture, flowcharts, pseudocode, sequence and entity diagrams and the database design in `docs/DESIGN.md`. An original Finora design system with a light and a dark theme, and a dashboard with a chart drawn by Finora itself |
| R2 C++ implementation, OOP and functionality | 3 | Classes, inheritance and polymorphism (`Page::refresh`, `paintEvent`, the abstract class `PaymentGateway`), structures, an enum, arrays, vectors, loops, `switch`, functions, file handling and a SQLite database with a safe upgrade. No compiler warnings reported. Five automated test programs with 982 checks |
| R3 Teamwork, collaboration and professional responsibility | 2 | Individual project. Milestone-by-milestone Git history, honest status labels, licence notices for every outside font and library, and documentation. Ask the instructor how this item is judged for individual work |
| R4 Academic integrity, learning attitude, engagement and demonstration | 3 | Viva preparation guide and two demonstration guides. Planned features are labelled honestly in the program ("Soon" badges, "Coming in Milestone N") and in the documents |
