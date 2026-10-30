# Finora

**Your Money. Your Control.**

Finora is a desktop personal finance and budget management application for Windows. It is written in C++17 with Qt 6 Widgets and SQLite, and is being built as a Programming Fundamentals semester project.

> **Current status: Milestone 4, Phase 4A.** The Transactions page from Milestone 3, plus a **dashboard**, **monthly budgets**, a **light and a dark theme** and a **safe database upgrade** to schema version 2. Sign-in and accounts (Phase 4B) and subscription plans (Phase 4C) are **planned and not yet implemented**. Pages that are not built yet say so and carry a "Soon" badge.

## Student details

| | |
|---|---|
| Student name | `[Your Name]` |
| Roll number | `[Your Roll No.]` |
| Course | Programming Fundamentals (SW112) |
| Instructor | Mr. Sajjad Ali |
| Department | Department of Software Engineering, Mehran University of Engineering and Technology, Jamshoro |
| Semester and year | 1st semester, 1st year |
| Project type | Individual semester project |

## Problem statement

Many people record their money in a notebook or not at all. They cannot easily see how much they earned and spent this month, whether they are staying inside a budget, or how close they are to a savings target. When some income arrives in another currency, such as US dollars from freelance work, adding the numbers together gives totals that are confusing or simply wrong.

Finora is an offline desktop program that records income and expenses, compares spending with monthly budgets, tracks savings goals, and keeps every currency separate unless the user asks for a clearly labelled conversion.

## Objectives

1. Record income and expense transactions with amount, currency, category, date and description.
2. Show monthly summaries, budget usage and savings progress on a dashboard.
3. Warn the user at 80% of a budget and alert them at 100%.
4. Support PKR, USD and EUR with exchange rates that the user enters.
5. Save everything in a local SQLite database so data survives closing the program.
6. Produce reports and export them to CSV.
7. Use clear, modular C++ that demonstrates the Programming Fundamentals syllabus.

## Features and status

| Feature | Status |
|---|---|
| Main window with sidebar navigation for 7 pages | Done in Milestone 1 |
| Light theme with hand-drawn icons | Done in Milestone 1 |
| Time-of-day greeting on the dashboard | Done in Milestone 1 |
| SQLite database created automatically, kept after closing | Done in Milestone 2 |
| Transaction validation: amount, currency, type, category, real calendar date | Done in Milestone 2 |
| Saving and reading transactions in the database | Done in Milestone 2 |
| Clear error dialog if the database cannot be opened | Done in Milestone 2 |
| Allow only one running copy of Finora, using Qt's QLockFile | Done in Milestone 3, first task |
| Transactions page: list, add, edit, delete with confirmation | Done in Milestone 3 |
| Search by description or category; filter by type, currency, category and period | Done in Milestone 3 |
| Typed amounts such as 1,200.50 or 1,20,000 read exactly, with a "Will be saved as" preview | Done in Milestone 3 |
| Automated core, interface and database tests | Done in Milestones 1 to 4A |
| Finora design system: violet brand, light and dark theme, Inter and Manrope fonts, recoloured icons | Done in Milestone 4, Phase 4A |
| Dashboard: one wallet card per currency (balance, this month's income and spending), money-flow chart, budget alerts, recent transactions, spending by category | Done in Phase 4A |
| Totals per currency with checked addition; currencies are never added together | Done in Phase 4A |
| Budgets: create, edit and delete a monthly limit per category and currency; spent, left, progress ring; "Nearing limit" at 80%, "Limit reached" at 100%, "Over budget" above it | Done in Phase 4A |
| Database schema version 2 with a verified backup before the upgrade, one transaction, row and amount checks, foreign key check and rollback on failure | Done in Phase 4A |
| Settings: theme, reduce motion, data location, font licences | Done in Phase 4A |
| Sign-up, sign-in, recovery codes, profiles, one person's data per account | Planned: Milestone 4, Phase 4B |
| Free, Plus, Pro and Business plans with a simulated checkout | Planned: Milestone 4, Phase 4C |
| Savings goals; PKR, USD and EUR with manual exchange rates | Planned: Milestone 5 |
| Reports and CSV export | Planned: Milestone 6 |
| Final testing, documentation and Windows package | Planned: Milestone 7 |

## Requirements to build

- Windows 10 or 11, 64-bit
- Qt 6.12.0 for MinGW 64-bit, installed at `C:\Qt\6.12.0\mingw_64`, including the Qt SQL module and its SQLite driver
- MinGW 13.1.0 64-bit, installed with Qt at `C:\Qt\Tools\mingw1310_64`
- CMake 3.30 and Ninja, installed with Qt in `C:\Qt\Tools`

If Qt is installed in a different folder, change the paths in `CMakePresets.json` and `build.bat`.

An older GCC 6.3 in `C:\MinGW\bin` cannot build Qt 6 programs. The build files always choose the Qt compiler instead.

## Build and run

**Easiest way.** Double-click `build.bat`, or run it from a terminal in the project folder. It configures, builds and runs all tests:

```bat
build.bat
```

Then start the program by double-clicking `build\debug\Finora.exe`, or from the terminal:

```bat
build\debug\Finora.exe
```

After every build, `windeployqt` copies the Qt DLLs, plugins, the SQLite driver and the MinGW runtime next to `Finora.exe`, so it runs without Qt on the PATH. Close Finora before building again, because Windows cannot replace a running `.exe` file.

**Step by step in PowerShell:**

```powershell
$env:PATH = "C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;$env:PATH"
cmake --preset debug           # 1. configure: find Qt and the compiler
cmake --build --preset debug   # 2. compile and link
ctest --preset debug           # 3. run the automated tests
.\build\debug\Finora.exe       # 4. start Finora
```

For an optimised build, use `release` instead of `debug`, or run `build.bat release`.

## Data storage, backup and reset

Finora keeps all its data in one SQLite database file:

```text
C:\Users\<your user name>\AppData\Local\Finora\finora.db
```

To open that folder, press **Win + R**, type `%LOCALAPPDATA%\Finora` and press Enter.

- **First start:** Finora creates the folder, the file and its tables automatically.
- **Upgrading from Milestone 2 or 3:** a file with schema version 1 is upgraded to version 2 only after you click **Upgrade and continue**. Finora first saves a complete copy in `%LOCALAPPDATA%\Finora\backups\` (for example `finora-v1-backup-2026-10-30_15-04-59.db`) and checks it. If any step of the upgrade fails, every change is rolled back and the file stays exactly as it was. **Quit without changes** closes Finora and changes nothing.
- **Back up:** close Finora, then copy `finora.db` to a safe place such as a USB drive.
- **Restore:** close Finora, then copy your backup over `finora.db`.
- **Start with an empty database:** close Finora and delete `finora.db`. A new, empty one is created at the next start. This permanently deletes all records.
- **Look inside:** close Finora and open `finora.db` in any SQLite viewer, such as the free DB Browser for SQLite.
- **`finora.lock`:** while Finora is open, this small file in the same folder marks the database as in use. A second copy of Finora sees it, says Finora is already open, and closes. Finora deletes the file when it closes. After a crash the file may stay behind; the next start sees that the old copy is no longer running and takes the lock over, so you never need to delete it yourself.
- **If the file cannot be opened:** Finora shows the file path and the reason, then closes with exit code 1. It never runs without saving.

The automated tests never use this file. They work in a temporary folder that is deleted when they finish.

Database design, schema and versioning are explained in [docs/DESIGN.md](docs/DESIGN.md), section 7.

## Automated tests

| Test program | What it checks | Last result, 30 Oct 2026, Windows |
|---|---|---|
| `finora_tests` | Greeting, leap years, days per month, real calendar dates, currencies, transaction validation, reading and formatting amounts, search and filters; checked addition, months, totals per currency, money flow, spending per category, chart scale, every edge of the budget rules | 199 of 199 checks pass |
| `finora_ui_tests` | The real main window and sidebar; the Transactions page on a real database; the form's validation; the dashboard's wallet cards, chart (including the arrow keys) and alerts; budgets created, refused, edited and deleted through the real forms; theme switching, reduce motion, and the saved theme after a restart | 131 of 131 checks pass |
| `finora_db_tests` | Real SQLite files: creating and reopening, saving, reading, updating, deleting, punctuation and Urdu text, invalid data, error messages, open failures, data kept after closing, SQLite's own rules, ids never reused, only one running copy; the upgrade from version 1 with its checked backup, two deliberate upgrade failures that leave the file byte-for-byte unchanged, settings, budgets, timestamps | 157 of 157 checks pass |

`finora_ui_tests --screenshots <folder>` also saves a picture of every page in both themes and of the forms.

Run a test program directly to see every check:

```powershell
.\build\debug\finora_db_tests.exe
```

## Project structure

```text
Finora/
├── CMakeLists.txt             Build script: programs, libraries and tests
├── CMakePresets.json          Qt and compiler paths for debug and release builds
├── build.bat                  One-click build and test script
├── src/
│   ├── main.cpp               Program entry point: theme, database, window
│   ├── core/                  Plain C++ logic without Qt, tested directly
│   │   ├── Greeting.h/.cpp        Time-of-day greeting
│   │   ├── Transaction.h/.cpp     Transaction record, currencies, categories, validation, dates
│   │   ├── Money.h/.cpp           Reading typed amounts; writing 1,200.50 and +PKR 1,200.50
│   │   ├── TransactionFilter.h/.cpp  Search and filters
│   │   ├── Totals.h/.cpp          Checked totals per currency, money flow per month, spending per category
│   │   └── Budget.h/.cpp          Budget record, 80% / 100% / over rules, progress
│   ├── database/              All SQL lives here
│   │   ├── DatabaseManager.h/.cpp Open SQLite, create and upgrade tables, backup, transactions, settings
│   │   ├── BudgetStore.h/.cpp     Save, load, change and delete budgets
│   │   └── InstanceLock.h/.cpp    Let only one copy of Finora run at a time
│   └── ui/                    Qt Widgets user interface
│       ├── MainWindow.h/.cpp      Window: sidebar, pages, theme switching
│       ├── Page.h                 Base class of every page (virtual refresh())
│       ├── Sidebar.h/.cpp         Logo, navigation buttons, theme button
│       ├── PageHeader.h/.cpp      Title and subtitle used on every page
│       ├── PlaceholderPage.h/.cpp "Coming in Milestone N" page
│       ├── DashboardPage.h/.cpp   The dashboard
│       ├── TransactionsPage.h/.cpp   The Transactions page
│       ├── TransactionDialog.h/.cpp  The Add / Edit Transaction form
│       ├── BudgetsPage.h/.cpp     The Budgets page
│       ├── BudgetDialog.h/.cpp    The New / Edit Budget form
│       ├── SettingsPage.h/.cpp    Appearance, data location, licences
│       ├── Theme.h/.cpp           Colour tokens of both themes, fonts, style sheet, icons
│       ├── UiHelpers.h/.cpp       Month names, money text, chips
│       └── components/            Reusable pieces: WalletCard, MoneyFlowChart,
│                                  ProgressRing, MonthSwitcher, Toast
├── resources/
│   ├── icons/                 Hand-drawn SVG icons, recoloured for each theme
│   ├── fonts/                 Inter and Manrope with their licence files (OFL 1.1)
│   └── styles/finora.qss      One style sheet for both themes, with {{colour}} names
├── tests/
│   ├── finora_tests.cpp       Core logic tests
│   ├── finora_ui_tests.cpp    Interface tests
│   └── finora_db_tests.cpp    Database tests on real SQLite files
└── docs/                      Design, requirements and viva preparation
```

## Documentation

- [Design: architecture, flowcharts, pseudocode, money rules, database](docs/DESIGN.md)
- [Requirements checklist](docs/REQUIREMENTS_CHECKLIST.md)
- [C++ concepts map](docs/CPP_CONCEPTS_MAP.md)
- [Viva preparation](docs/VIVA_PREPARATION.md)
- [Milestone 3 demonstration guide](docs/DEMO_MILESTONE3.md)
- [Milestone 4 demonstration guide](docs/DEMO_MILESTONE4.md)
- [Third-party notices: fonts, Qt, SQLite](THIRD_PARTY_NOTICES.md)

## Known limitations at Milestone 4, Phase 4A

- There are no accounts yet. Everything belongs to "this computer"; the account tables exist in schema version 2 but the sign-in screens come in Phase 4B. Existing records will be moved into an account only if the user confirms it.
- Different currencies are never added together, so there is no single "total balance" across PKR, USD and EUR. That needs exchange rates (Milestone 5).
- Search and filters work on the list already loaded into memory. That is fast for one person's records; very large data sets would need filtering in SQL.
- Search, and matching a budget to its category, ignore upper and lower case only for the English letters A to Z.
- The Transactions table is always sorted newest first; clicking a column header does not re-sort it.
- On the Transactions page, "Food" and "food" are still listed as two categories in the filter. Budgets and the dashboard treat them as one.
- A budget is the same limit every month; there are no different limits for different months yet.
- Savings goals, reports and exchange rates are not built yet; their pages say "Coming in Milestone N".
- The build paths assume Qt is installed in `C:\Qt`.
- The greeting is chosen each time the dashboard is shown; it does not change while you stay on the dashboard.
- Only one copy of Finora can run at a time, because SQLite lets only one program write to the database at once. A second copy says Finora is already open and closes. This is checked per Windows user account; two different Windows users have separate data folders and separate databases. Details are in [docs/DESIGN.md](docs/DESIGN.md), section 7.9.
