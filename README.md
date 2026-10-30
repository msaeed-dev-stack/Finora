# Finora

**Your Money. Your Control.**

Finora is a desktop personal finance and budget management application for Windows. It is written in C++17 with Qt 6 Widgets and SQLite, and is being built as a Programming Fundamentals semester project.

> **Current status: Milestone 4, Phase 4B complete; Phase 4C not started.**
> - **From Milestone 3:** the Transactions page.
> - **Phase 4A:** a **dashboard**, **monthly budgets**, a **light and a dark theme**, and a **safe database upgrade** to schema version 2.
> - **Phase 4B:** local **accounts** with Argon2id-protected passwords, recovery codes, sessions, and **one person's data per account**.
> - **Not built yet:** subscription plans (Phase 4C). Pages that are not built yet say so and carry a "Soon" badge.

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
| Accounts: sign-up, sign-in, sign-out, profile, password change; passwords stored only as Argon2id hashes (libsodium) | Done in Phase 4B |
| Password strength meter; general sign-in errors; a waiting time after 3 wrong passwords | Done in Phase 4B |
| Sessions that end after 15 minutes without activity and after 12 hours | Done in Phase 4B |
| Password recovery with 10 single-use recovery codes, stored only as hashes | Done in Phase 4B |
| One person's data per account: every transaction, budget and setting belongs to the signed-in person | Done in Phase 4B |
| Records made before accounts are moved into an account only after the person agrees | Done in Phase 4B |
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

## Accounts, passwords and recovery

Finora starts with a sign-in window. Each person who uses Finora on the computer creates an account and sees only their own transactions, budgets and settings.

> **Local demonstration accounts.** Accounts exist only in the database file on this computer. There is no server, no online account and no email. The sign-in window says so.

- **Passwords** must have at least 10 characters and be at least "Fair" on the strength meter. Common passwords and passwords that contain the person's name or email are refused.
  - Finora never stores a password.
  - It stores a one-way **Argon2id** hash made by **libsodium**: 256 MiB of memory and 3 passes (the MODERATE setting), with a new random salt for every hash.
  - Details and measurements are in [docs/MILESTONE4_PROGRESS.md](docs/MILESTONE4_PROGRESS.md).
- **Signing in.** A wrong password and an unknown email give the same message: "The email or password is not correct." After 3 wrong tries the next one must wait 30 seconds, then 60, and so on, up to 15 minutes.
- **Sessions** end after 15 minutes without activity, 12 hours after signing in, or with **Sign out** (in the sidebar and in Settings).
- **Recovery codes.** When an account is created, Finora shows 10 recovery codes **once**, such as `7KQ4-M2TX-9RHC`.
  - Each code resets a forgotten password once. A used code never works again.
  - Only hashes of the codes are stored.
  - New codes (Settings > Security) replace all old ones.

**Local recovery and production recovery.**

| | Finora on this computer | A production (online) Finora |
|---|---|---|
| Where accounts live | In `finora.db` on this computer | On a server, protected by the provider |
| Forgotten password | Sign in window > "Forgot your password?" > email and one recovery code | A time-limited reset link is emailed to the account's verified address. Recovery codes remain a backup for when email is lost |
| What proves it is you | Something you saved: a single-use code | Access to your email inbox, often plus a second factor |
| Why | A local program cannot send email without an email service and stored service passwords, which must never be put inside a desktop program | The server keeps the secrets and sends the email |

## Automated tests

| Test program | What it checks | Last result, 30 Oct 2026, Windows |
|---|---|---|
| `finora_tests` | Greeting, leap years, days per month, real calendar dates, currencies, transaction validation, reading and formatting amounts, search and filters; checked addition, months, totals per currency, money flow, spending per category, chart scale, every edge of the budget rules; the password policy and strength meter, email and name rules, recovery code format, sign-in delays | 241 of 241 checks pass |
| `finora_ui_tests` | The real main window and sidebar; the Transactions page on a real database; the form's validation; the dashboard's wallet cards, chart (including the arrow keys) and alerts; budgets created, refused, edited and deleted through the real forms; theme switching, reduce motion, and the saved theme after a restart; the sign-in window: sign-up, recovery codes, moving older records, sign-out, a wrong password, recovery, a second account, automatic sign-out | 164 of 164 checks pass |
| `finora_auth_tests` | Argon2id with libsodium, including its measured 256 MiB memory use and a deliberate out-of-memory failure; sign-up with no password anywhere in the database file; general error messages and waiting times; sessions; profile and password changes; single-use recovery codes; one person's data per account; moving older records | 106 of 106 checks pass |
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
│   ├── main.cpp               Program entry point: libsodium, database, session, windows
│   ├── auth/                  Accounts (the only code that uses libsodium)
│   │   ├── PasswordHasher.h/.cpp  Argon2id hashing, recovery codes, wiping secrets
│   │   └── AuthService.h/.cpp     Sign-up, sign-in, sessions, profile, recovery
│   ├── core/                  Plain C++ logic without Qt, tested directly
│   │   ├── Greeting.h/.cpp        Time-of-day greeting
│   │   ├── Transaction.h/.cpp     Transaction record, currencies, categories, validation, dates
│   │   ├── Money.h/.cpp           Reading typed amounts; writing 1,200.50 and +PKR 1,200.50
│   │   ├── TransactionFilter.h/.cpp  Search and filters
│   │   ├── Totals.h/.cpp          Checked totals per currency, money flow per month, spending per category
│   │   ├── Budget.h/.cpp          Budget record, 80% / 100% / over rules, progress
│   │   ├── PasswordPolicy.h/.cpp  Password rules and strength meter
│   │   ├── AccountRules.h/.cpp    Email, name, recovery code and sign-in delay rules
│   │   └── Session.h/.cpp         Who is signed in (only AuthService may change it)
│   ├── database/              All SQL lives here
│   │   ├── DatabaseManager.h/.cpp Open SQLite, upgrade, backup, transactions, settings; one owner per row
│   │   ├── BudgetStore.h/.cpp     Save, load, change and delete budgets
│   │   ├── UserStore.h/.cpp       Accounts, recovery codes, sessions, audit history
│   │   └── InstanceLock.h/.cpp    Let only one copy of Finora run at a time
│   └── ui/                    Qt Widgets user interface
│       ├── AppController.h/.cpp   Sign-in window first, main window after; idle sign-out
│       ├── AuthWindow.h/.cpp      Welcome, sign in, create account, recovery codes, reset
│       ├── AccountDialogs.h/.cpp  Edit profile, change password, new recovery codes
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
│       └── components/            Reusable pieces: WalletCard, MoneyFlowChart, ProgressRing,
│                                  MonthSwitcher, Toast, PasswordField, RecoveryCodesPanel
├── third_party/libsodium-1.0.22/  The verified libsodium DLL, headers, ISC licence, VERIFICATION.md
├── resources/
│   ├── icons/                 Hand-drawn SVG icons, recoloured for each theme
│   ├── fonts/                 Inter and Manrope with their licence files (OFL 1.1)
│   └── styles/finora.qss      One style sheet for both themes, with {{colour}} names
├── tests/
│   ├── finora_tests.cpp       Core logic tests
│   ├── finora_ui_tests.cpp    Interface tests
│   ├── finora_db_tests.cpp    Database tests on real SQLite files
│   └── finora_auth_tests.cpp  Account and security tests
└── docs/                      Design, requirements, progress record and viva preparation
```

## Documentation

- [Design: architecture, flowcharts, pseudocode, money rules, database](docs/DESIGN.md)
- [Requirements checklist](docs/REQUIREMENTS_CHECKLIST.md)
- [C++ concepts map](docs/CPP_CONCEPTS_MAP.md)
- [Viva preparation](docs/VIVA_PREPARATION.md)
- [Milestone 3 demonstration guide](docs/DEMO_MILESTONE3.md)
- [Milestone 4 demonstration guide](docs/DEMO_MILESTONE4.md)
- [Milestone 4 progress record: stages, files, tests, Argon2id settings](docs/MILESTONE4_PROGRESS.md)
- [Third-party notices: fonts, libsodium, Qt, SQLite](THIRD_PARTY_NOTICES.md)

## Known limitations at Milestone 4, Phase 4B

- Accounts are local demonstration accounts: no server, no email, no online sign-in. A forgotten password is reset with a recovery code (see "Accounts, passwords and recovery").
- Creating an account says when an email address already has an account on this computer; signing in and resetting never reveal it.
- Qt's own copies of a typed password cannot be wiped from memory reliably; Finora empties the password box at once and overwrites its own copies with zeros.
- Hashing a password takes about 0.65 s with the MODERATE setting, so signing in shows a short busy pointer, and creating an account about 1.5 s.
- Subscription plans (Phase 4C) are not built yet.
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
