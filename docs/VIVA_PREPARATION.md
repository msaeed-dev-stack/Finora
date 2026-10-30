# Finora Viva Preparation

Questions your Programming Fundamentals instructor may ask about Finora, with answers that match the real code. Every answer names the file or function it comes from, so you can open the code and check it yourself.

Features that are not built yet are marked **Planned — Not Yet Implemented**. Never present them as finished.

Last updated: Milestone 4, Phase 4B, 30 October 2026.

**How to use this file.** Read the short answer first. Then open the file named under "Where in the code" and find the lines yourself. If you cannot explain a line, study it before the viva.

---

## A. Project Overview

### A1. What is Finora?
- **Short answer:** A Windows desktop program for personal finance and budgeting, written in C++17 with Qt 6 Widgets and a SQLite database.
- **Detailed explanation:** Finora will record income and expenses, track monthly budgets and savings goals, and handle PKR, USD and EUR without mixing them. At Milestone 3 the window, navigation, theme, database, single-copy guard and the Transactions page work. The other finance pages are planned, and each says which milestone adds it.
- **Where in the code:** `src/main.cpp` starts the program; `src/database/DatabaseManager.cpp` handles the data.

### A2. What problem does it solve?
- **Short answer:** People lose track of what they earn and spend, overspend without warning, and mix currencies in their totals.
- **Detailed explanation:** A notebook cannot warn you at 80% of a budget, and adding dollars to rupees gives a meaningless total. Finora keeps each currency separate, will warn about budgets and show savings progress, and works offline.

### A3. Who are the intended users?
- **Short answer:** Students, young earners and freelancers who want a simple offline money tracker.
- **Detailed explanation:** Freelancers often earn in USD or EUR and spend in PKR, which is why the database already stores a currency with every transaction.

### A4. What are the main features?
- **Short answer:** Done: navigation, the Transactions page with add, edit, delete, search and filters, the dashboard, monthly budgets, a light and a dark theme, settings, a safe database upgrade, and local accounts with Argon2id passwords and one person's data per account. Planned: subscription plans (Phase 4C), goals, exchange rates and reports.
- **Detailed explanation:** See the feature table in `README.md` and sections K and L of this guide.

### A5. What are the scope and limitations?
- **Short answer:** One user on one Windows computer, offline, three currencies, exchange rates typed in by hand.
- **Detailed explanation:** No cloud, no bank connection, no live exchange rates, no mobile version. See "Known limitations" in `README.md`.

### A6. Why did you choose this project?
- **Short answer:** Answer this in your own words. It must be your real reason.
- **Points you may use if they are true for you:** money tracking is a real daily problem; the project naturally needs loops, conditions, arrays, structures, classes and files; a GUI makes the demonstration clear.

---

## B. C++ Fundamentals

### B1. Which variables and data types did you use, and why?
- **Short answer:** `int` for whole numbers, `long long` for money, `bool` for yes/no results, `char` when checking dates, `std::string` and `QString` for text.
- **Detailed explanation:** `Transaction::amountMinor` is a `long long` because money is stored in paisa and needs a large range. `isValidIsoDate` looks at each `char` of the date. `DatabaseManager::open` returns a `bool`. `double` is not used: money must stay exact.
- **Where in the code:** `src/core/Transaction.h`, `src/core/Transaction.cpp`.

### B2. Where are arithmetic, relational and logical operators used?
- **Short answer:** Arithmetic in `digitsToNumber` and `setStartingSize`; the remainder operator in `isLeapYear`; relational and logical operators in every validation function.
- **Example:** `number = number * 10 + (text[i] - '0');` builds 2026 from "2026". `year % 4 == 0` tests divisibility by 4. `transaction.type != TYPE_INCOME && transaction.type != TYPE_EXPENSE` detects an unknown type.

### B3. Where did you use type casting or conversion?
- **Short answer:** `static_cast<int>`, character arithmetic, and conversions between `std::string`, `QString` and numbers.
- **Detailed explanation:** `isBlank` uses `static_cast<int>(text.length())` because `length()` returns an unsigned size. `text[i] - '0'` turns a digit character into its number. `DatabaseManager` converts with `QString::fromStdString` before SQL and `toStdString()` and `toLongLong()` after reading.

### B4. Which loops and control structures did you use?
- **Short answer:** `for`, `while`, `if / else if / else` and `switch`.
- **Detailed explanation:** `for` loops create the buttons and pages, check each date character, and insert the three currencies. `while (query.next())` in `DatabaseManager::tableNames` reads database rows until none are left. `switch (month)` in `daysInMonth` returns 31, 30, 28 or 29. A `do-while` loop is not used yet: **Planned — Not Yet Implemented**.

### B5. Why did you use functions instead of writing everything in `main()`?
- **Short answer:** Each function does one job, so it is easy to read, test and change.
- **Detailed explanation:** `main()` has ten top-level statements. It creates the application, sets the name and version, applies the theme, creates the database object and gets the path. Then an `if` opens the database and, on failure, shows an error and returns 1. Finally it creates and shows the window and starts the event loop. The leap-year rule lives in `isLeapYear`, so the tests can call it directly.

### B6. Where did you use strings, arrays, structures or vectors?
- **Short answer:** Structures for transactions and currencies; arrays of structures for buttons, pages, currencies and test cases; strings everywhere. Vectors are not used yet.
- **Detailed explanation:** `struct Transaction` holds one record. `SUPPORTED_CURRENCIES` is an array of 3 `CurrencyInfo` structures and is used both to validate codes and to fill the `currencies` table. Since Milestone 3, a `std::vector<Transaction>` holds the list loaded from SQLite: `loadTransactions` fills it with `push_back`, and `filterTransactions` returns a new vector with only the matching transactions.

### B7. How do parameters, return values and function calls work in your code?
- **Short answer:** Values come in through parameters and go out through return values or reference parameters.
- **Detailed explanation:** `daysInMonth(2024, 2)` returns 29. `findTransactionById(int id, Transaction &result)` returns `true` or `false`, and fills the caller's `result` structure through the reference `&`. `addTransaction(Transaction &transaction)` writes the new id back into the caller's structure.

### B8. What are header files and include guards?
- **Short answer:** A `.h` file declares what exists; a `.cpp` file contains the code; include guards stop a header being included twice.
- **Where in the code:** every header starts with `#ifndef FINORA_..._H`.

### B9. Why are page numbers, the maximum amount and the schema version constants?
- **Short answer:** So each value has one name and cannot change by mistake.
- **Example:** `MAX_AMOUNT_MINOR` is written once in `Transaction.h`, and `validateTransaction` and the tests use the name.

### B10. What is a pointer here, and what is `nullptr`?
- **Short answer:** A pointer holds the address of an object; `nullptr` means it points to nothing yet.
- **Detailed explanation:** Qt widgets are created with `new` and stored in pointers such as `Sidebar *sidebar`. The database code uses no pointers at all.

### B11. Why is money stored as `long long` and not `double`?
- **Short answer:** A `double` cannot store most decimal fractions exactly, so sums drift. Whole paisa add up exactly.
- **Detailed explanation:** In a `double`, 0.1 + 0.2 gives 0.30000000000000004. Storing PKR 1,200.50 as the whole number 120050 avoids this. A `long long` is 64 bits, so its maximum is 9,223,372,036,854,775,807. An `int` would stop at 2,147,483,647 paisa, which is only about 21.4 million rupees.

### B12. How does `text[i] - '0'` turn a character into a number?
- **Short answer:** Characters are stored as numeric codes, and '0' to '9' have consecutive codes, so subtracting the code of '0' gives the digit's value.
- **Example:** In ASCII, '7' is 55 and '0' is 48, and 55 − 48 = 7. `digitsToNumber("2026-10-28", 0, 4)` computes 0×10+2 = 2, then 20, then 202, then 2026.

---

## C. Object-Oriented Programming

### C1. Which classes and objects are used?
- **Short answer:** `MainWindow`, `Sidebar`, `PageHeader`, `PlaceholderPage` and `DatabaseManager`. `main()` creates one `DatabaseManager` and one `MainWindow`.
- **Detailed explanation:** `Transaction` is a `struct`: a simple group of public data with no behaviour of its own. Its rules are separate functions such as `validateTransaction`. `Budget` and `SavingsGoal` are **Planned — Not Yet Implemented** (Milestones 4 and 5).

### C2. What does each class represent?

| Class or structure | Represents | File |
|---|---|---|
| `MainWindow` | The whole window: sidebar plus page area | `src/ui/MainWindow.h/.cpp` |
| `Sidebar` | The navigation bar with logo and 7 buttons | `src/ui/Sidebar.h/.cpp` |
| `PageHeader` | The title and subtitle at the top of a page | `src/ui/PageHeader.h/.cpp` |
| `PlaceholderPage` | A page whose feature is not built yet | `src/ui/PlaceholderPage.h/.cpp` |
| `DatabaseManager` | One open connection to the SQLite database file | `src/database/DatabaseManager.h/.cpp` |
| `Transaction` (struct) | One income or expense record | `src/core/Transaction.h` |
| `CurrencyInfo` (struct) | One supported currency: code, name, symbol, decimals | `src/core/Transaction.h` |

### C3. What are constructors, and where are they used?
- **Short answer:** A constructor runs automatically when an object is created and sets it up.
- **Detailed explanation:** `DatabaseManager(const QString &name = "finora") : connectionName(name)` stores the connection name through the initializer list. It does not open the file. Opening can fail, and a constructor cannot return `false`, so opening is done by `open()`, which can.

### C4. How does encapsulation apply to the project?
- **Short answer:** Data members are private; other code must use public member functions that check things first.
- **Detailed explanation:** In `DatabaseManager`, `opened`, `connectionName` and `errorMessage` are private. Nobody can set `opened = true` without really opening the file; they must call `open()`, which validates the path, the connection name and the driver.

### C5. Which data members and member functions does each class have?

| Class | Private data members | Member functions |
|---|---|---|
| `MainWindow` | `sidebar`, `pageStack` | Constructor, `showPage` (slot), `createLayout`, `createPages`, `setStartingSize` |
| `Sidebar` | `navButtonGroup` | Constructor, `setCurrentPage`, `pageSelected` (signal), `onNavButtonClicked` (slot), `createBrandArea`, `createNavButtons`, `createNavIcon` |
| `PageHeader` | `titleLabel`, `subtitleLabel` | Constructor |
| `PlaceholderPage` | none | Constructor, `createComingSoonCard` |
| `DatabaseManager` | `connectionName`, `currentPath`, `errorMessage`, `opened` | Constructor, destructor, `defaultDatabasePath` (static), `open`, `close`, `isOpen`, `databasePath`, `lastError`, `schemaVersion`, `tableNames`, `addTransaction`, `findTransactionById`; private `enableForeignKeys`, `upgradeSchema`, `createVersion1Tables`, `insertSupportedCurrencies`, `runStatement`, `fail` |

### C6. How do the classes interact?
- **Short answer:** `main()` owns the `DatabaseManager` and the `MainWindow`. The `Sidebar` tells `MainWindow` which page was chosen with a signal. `DatabaseManager` uses the core's `validateTransaction`.
- **Detailed explanation:** `main()` passes the `DatabaseManager` to `MainWindow`, which passes it to `TransactionsPage`. The page calls `loadTransactions`, `addTransaction`, `updateTransaction` and `deleteTransaction`; it never contains SQL. `TransactionsPage` opens a `TransactionDialog` for the form. See section K.

### C7. Did you use inheritance?
- **Short answer:** Yes, in two ways. Every window part extends a Qt class (`Sidebar : public QFrame`). And since Phase 4A, Finora has its own base class `Page`, from which `DashboardPage`, `TransactionsPage`, `BudgetsPage`, `SettingsPage` and `PlaceholderPage` inherit. `DatabaseManager` and `BudgetStore` inherit nothing.
- **Detailed explanation:** `Page` declares `virtual void refresh()`. Each page overrides it with its own version. `MainWindow` keeps all pages in a `std::vector<Page *>` and calls `pages[n]->refresh()`; C++ runs the right version at run time. That is polymorphism. See L9.

### C8. You create widgets with `new`. Who deletes them?
- **Short answer:** Qt does. A parent widget deletes its children when it is destroyed.

### C9. What is a destructor, and what is RAII?
- **Short answer:** A destructor runs automatically when an object is destroyed. RAII means a resource is tied to an object's lifetime, so it is released automatically.
- **Detailed explanation:** `~DatabaseManager()` calls `close()`. In `main()`, `database` is a local variable, so when `main()` ends, the destructor closes the database even if the code returns early. The tests use the same idea with `QTemporaryDir`, which deletes its folder when destroyed.
- **Example:** In `testPersistence`, `writer` lives inside `{ }`. At the closing brace its destructor closes the file, and a new `reader` object then reopens it.

### C10. Why is copying a `DatabaseManager` disabled with `= delete`?
- **Short answer:** A connection must have exactly one owner.
- **Detailed explanation:** A copy would hold the same connection name. When the copy was destroyed, its destructor would close the original's connection. Writing `DatabaseManager(const DatabaseManager &) = delete;` makes the compiler refuse to copy it at all.

### C11. What is a static member function?
- **Short answer:** A member function that belongs to the class, not to one object, so it can be called without creating an object.
- **Example:** `DatabaseManager::defaultDatabasePath()` is called in `main()` to get the path.

---

## D. Algorithms and Financial Logic

### D1. How is the dashboard greeting chosen?
- **Short answer:** `greetingForHour` checks the hour with an if-else chain: 5 to 11 morning, 12 to 16 afternoon, otherwise evening; invalid hours give "Hello".
- **Where in the code:** `src/core/Greeting.cpp`; flowchart in `docs/DESIGN.md`, section 4.

### D2. How does page switching work?
- **Short answer:** Button click, then signal, then slot, then the page changes.
- **Where in the code:** `Sidebar::createNavButtons`, `Sidebar::onNavButtonClicked`, `MainWindow::showPage`.

### D3. How is the starting window size calculated?
- **Short answer:** 1280 × 800, reduced to 90% of the screen if needed, then centred. See `MainWindow::setStartingSize`.

### D4. How do you know whether a year is a leap year?
- **Short answer:** Divisible by 400: yes. Otherwise divisible by 100: no. Otherwise divisible by 4: yes.
- **Where in the code:** `isLeapYear` in `src/core/Transaction.cpp`.
- **Example:** 2000 % 400 is 0, so 2000 is a leap year. 1900 % 400 is 300 but 1900 % 100 is 0, so it is not. 2024 % 4 is 0, so it is. 2026 % 4 is 2, so it is not.

### D5. How does date validation work?
- **Short answer:** `isValidIsoDate` checks the shape YYYY-MM-DD, checks every other character is a digit, converts the numbers, and checks the year range, the month range, and the day against `daysInMonth`.
- **Example:** "2026-02-29": the shape and digits are fine; year 2026, month 2, day 29; `daysInMonth(2026, 2)` is 28; 29 > 28, so it is rejected. "2024-02-29" passes because 2024 is a leap year.
- **Where in the code:** `src/core/Transaction.cpp`; pseudocode in `docs/DESIGN.md`, section 8.

### D6. How does `validateTransaction` decide whether a transaction is valid?
- **Short answer:** It checks type, amount, maximum, currency, category and date in that order, and returns the message for the first problem, or an empty string.
- **Example:** An amount of 0 returns "Amount must be greater than zero."

### D7. How is total income calculated?
- **Short answer:** `summarizeByCurrency` loops over the transactions and adds each income to the total of its own currency with `addChecked`, once for all time and once more if it is in the chosen month.
- **Where in the code:** `src/core/Totals.cpp`; pseudocode in `docs/DESIGN.md`, section 10.4.
- **Example:** a salary of PKR 120,000.00 on 1 October gives `monthIncomeMinor = 12000000` for PKR in October.

### D8. How are total expenses calculated?
- **Short answer:** In the same loop, each expense is added to `totalExpenseMinor` and, if it is in the chosen month, to `monthExpenseMinor` of its own currency, with checked addition.

### D9. How is the net balance calculated?
- **Short answer:** `balanceOf` returns total income minus total expenses for one currency; `monthNetOf` does the same for the chosen month. Each currency has its own wallet card.
- **Detailed explanation:** Both totals are 0 or more, so subtracting one from the other can never overflow. A negative balance is allowed and shown with a minus sign.

### D10. How are transactions categorised and filtered?
- **Short answer:** Each transaction has a category. The page loads all transactions into a vector, and `filterTransactions` keeps those for which `matchesFilter` is true.
- **Detailed explanation:** `matchesFilter` checks type, currency, category, a date range and a search text, returning `false` at the first rule that fails. Empty filter fields mean "any". Dates are compared as YYYY-MM-DD text. The search ignores case for A to Z.
- **Where in the code:** `src/core/TransactionFilter.cpp`; pseudocode in `docs/DESIGN.md`, section 9.3.
- **Example:** Type "Expense" plus currency "PKR" plus search "bus" keeps only "Bus fare".

### D11. How does the budget alert system work?
- **Short answer:** An expense counts toward a budget when its category (ignoring case), currency and month match. `budgetStatusFor` then gives "On track", "Nearing limit" (80% or more), "Limit reached" (exactly 100%) or "Over budget". Every status other than "On track" appears under "Budget alerts" on the dashboard.
- **Where in the code:** `src/core/Budget.cpp`; `DashboardPage::showBudgetAlerts`. See L4 and L5.

### D12. How is savings-goal progress calculated?
**Planned — Not Yet Implemented** (Milestone 5).

### D13. How are currencies converted, and how do you avoid mixing currencies?
- **Short answer:** Conversion is **Planned — Not Yet Implemented** (Milestone 5). Mixing is prevented: every total belongs to one currency. `summarizeByCurrency` looks up the summary with the same currency code and skips a transaction whose currency it does not know. The chart and the spending card show one currency at a time.

### D14. How are invalid amounts and rounding handled?
- **Short answer:** Invalid amounts are rejected, never rounded. Typed text is read exactly with `parseAmount`, without any `double`.
- **Detailed explanation:** `parseAmount` refuses letters, a minus sign, more than two digits after the point, two points, misplaced commas, zero and anything above 9,999,999,999.99, each with its own message. `validateTransaction` and SQLite's CHECK rule refuse the same values again. Rounding is only needed for currency conversion: **Planned — Not Yet Implemented** (Milestone 5).
- **Example:** "12.345" gives "Use at most two digits after the decimal point." and the form stays open.

### D15. Can totals overflow?
- **Short answer:** One transaction cannot, but a total of many could, so every total uses checked addition.
- **Detailed explanation:** The maximum single amount fits easily in a 64-bit `long long`. Adding enough of them could still pass 9,223,372,036,854,775,807 and silently wrap around to a wrong number. `addChecked` tests `amount > LLONG_MAX - total` before adding. If the amount does not fit, nothing is added and the total is marked as overflow; the wallet card then says "Too large to show" instead of a wrong number.
- **Where in the code:** `addChecked` in `src/core/Totals.cpp`; pseudocode in `docs/DESIGN.md`, section 10.4. The core tests force an overflow with two huge amounts.

---

## E. Database and File Handling

### E1. Why was SQLite selected?
- **Short answer:** It is a complete SQL database in one file, needs no server or internet, and Qt supports it directly.
- **Detailed explanation:** A server database such as MySQL would need installing and running a separate program. SQLite is a library inside Finora; Qt loads it from `sqldrivers\qsqlite.dll`.

### E2. Where is the database stored, and why there?
- **Short answer:** `%LOCALAPPDATA%\Finora\finora.db`, which is `C:\Users\<user name>\AppData\Local\Finora\finora.db`.
- **Detailed explanation:** `DatabaseManager::defaultDatabasePath()` asks Qt for `QStandardPaths::AppLocalDataLocation`. That is the standard Windows folder for a program's own data. The program folder may not be writable, and the source folder must never hold personal data. `open()` creates the folder if it is missing.

### E3. What information is stored in the database?
- **Short answer:** Since schema version 2: `currencies`, `transactions`, `budgets`, `app_settings` (theme and reduce motion), `audit_events`, and four account tables for Phase 4B: `users`, `user_settings`, `recovery_codes` and `login_sessions`.
- **Where in the code:** `createVersion1Tables` and `upgradeToVersion2` in `DatabaseManager.cpp`. Explained in `docs/DESIGN.md`, sections 7.2 and 10.2.

### E4. What is a primary key? Why `AUTOINCREMENT`?
- **Short answer:** A column whose value is unique for every row, used to find one exact row.
- **Detailed explanation:** `transactions.id` is `INTEGER PRIMARY KEY AUTOINCREMENT`: SQLite gives each new row the next number, and AUTOINCREMENT guarantees a number is never reused, even after deletion. `currencies.code` is a text primary key, so "PKR" can exist only once.

### E5. What is a foreign key?
- **Short answer:** A column that must match the primary key of another table.
- **Detailed explanation:** `transactions.currency_code REFERENCES currencies (code)` means a transaction can only use a currency that exists. SQLite enforces this only after `PRAGMA foreign_keys = ON`, which `enableForeignKeys()` runs and checks at every open. The test proves SQLite refuses currency "XYZ" and refuses to delete PKR while a transaction uses it.

### E6. What are CHECK constraints, and why have them if C++ already validates?
- **Short answer:** Rules stored in the table. They protect the data even from SQL that bypasses the C++ code.
- **Detailed explanation:** `amount_minor` must be between 1 and 999999999999, `type` must be 'income' or 'expense', and `transaction_date IS date(transaction_date)` rejects impossible dates such as 2026-02-30. This is defence in depth: the C++ check gives a friendly message, and the database rule is a safety net.

### E7. What is a prepared statement, and what is SQL injection?
- **Short answer:** A prepared statement sends the SQL text and the values separately. SQL injection is when typed text changes the SQL command itself.
- **Detailed explanation:** `addTransaction` prepares `INSERT ... VALUES (:type, :amount, ...)` and then calls `bindValue` for each placeholder. If the text were glued into the SQL instead, a description containing `'); DROP TABLE transactions; --` could end the string and run a second command. With placeholders it is stored as plain text.
- **Example:** The test saves `Ali's café, 50% off; "special" -- DROP TABLE transactions; --`, reads back exactly the same text, and the table still exists.

### E8. How does the application save and read a transaction?
- **Short answer:** `addTransaction` validates, then runs a prepared INSERT and stores the new id. `findTransactionById` runs a prepared SELECT and copies each column into a `Transaction`.
- **Detailed explanation:** See `docs/DESIGN.md`, section 7.7. Since Milestone 3, `loadTransactions` reads every row with a `while (query.next())` loop, `updateTransaction` runs a prepared `UPDATE ... WHERE id = :id`, and `deleteTransaction` runs a prepared `DELETE ... WHERE id = :id`. Both check `numRowsAffected()`: 0 means no row had that id, which is reported as "No transaction with id N was found."

### E9. How is the schema created, and why is it safe to repeat?
- **Short answer:** `upgradeSchema()` reads the version number stored in the file and brings it up to version 2 one step at a time, inside one database transaction.
- **Detailed explanation:** `PRAGMA user_version` is 0 in a new file: Finora creates the version 1 tables, adds the currencies, upgrades them to version 2 and sets the version to 2, all between `transaction()` and `commit()`. A version 1 file (from Milestone 2 or 3) is first backed up, then upgraded the same way (see L3). A version of 2 means nothing to do. A higher version means a newer Finora made the file, so it is refused untouched.

### E10. What is a database transaction?
- **Short answer:** A group of statements that is saved completely or not at all.
- **Detailed explanation:** If any step of creating or upgrading the tables fails, `rollback()` undoes every earlier step too, so a half-built or half-upgraded database can never exist. The database tests prove it: after two deliberate upgrade failures the file is byte-for-byte unchanged. Do not confuse it with a Finora *money* transaction.

### E11. What happens when the database cannot be opened?
- **Short answer:** Finora shows an error dialog with the file path and the reason, then exits with code 1. It never runs without saving.
- **Where in the code:** `showDatabaseError` and `main()` in `src/main.cpp`.
- **Verified on 28 October 2026:** with a file placed where the folder should be, the real program showed "Reason: Could not create the folder C:\Users\<user name>\AppData\Local\Finora." and exited with code 1.

### E12. How is user data preserved after closing the application?
- **Short answer:** Every saved transaction is written into `finora.db` on disk immediately, and the file is reopened at the next start.
- **Detailed explanation:** The test `testPersistence` saves a record, destroys the `DatabaseManager`, creates a new one, reopens the file and finds the same record with every field equal.

### E13. How can the data be backed up and restored?
- **Short answer:** Close Finora and copy `finora.db`. To restore, close Finora and copy the backup back. In addition, before an upgrade changes the file, Finora itself saves and checks a copy in the `backups` folder next to the database.
- **Where:** "Data storage, backup and reset" in `README.md`; `backupBeforeUpgrade` in `DatabaseManager.cpp`.

### E14. Why must Qt SQL handles be destroyed before removing a connection?
- **Short answer:** Qt refuses to remove a connection that a `QSqlDatabase` or `QSqlQuery` object still uses, and prints "connection is still in use".
- **Detailed explanation:** `DatabaseManager` never stores those objects as members. Each function creates them as local variables. In `open()` and `close()`, extra braces `{ }` end the handle's life before `QSqlDatabase::removeDatabase()` runs. The database test run printed no such warning.

### E15. Does the program read or write other files?
- **Short answer:** It reads its style sheet with `QFile` in `loadStyleSheet`. Writing CSV files is **Planned — Not Yet Implemented** (Milestone 6).

### E16. How did you test the database without damaging real data?
- **Short answer:** The tests use real SQLite files inside a `QTemporaryDir`, a new temporary folder that is deleted automatically.
- **Detailed explanation:** `open()` takes the file path as a parameter, so the tests pass a temporary path instead of the real one. After the test run, the real `%LOCALAPPDATA%\Finora` folder was confirmed untouched.

---

## F. Qt and Desktop UI

### F1. What is Qt?
- **Short answer:** A C++ library for graphical programs. Finora uses its Widgets module for windows and its SQL module for SQLite.

### F2. Why did you use Qt Widgets?
- **Short answer:** Widgets are normal C++ classes for desktop windows, so the whole program stays in C++.

### F3. What is the purpose of `QApplication` and `QMainWindow`?
- **Short answer:** `QApplication` runs the program and its event loop; `QMainWindow` is the main window.

### F4. How do signals and slots connect the UI to the program logic?
- **Short answer:** A signal is a message an object sends; a slot is a function that runs when the message arrives; `connect()` links them.

### F5. What does `Q_OBJECT` do?
- **Short answer:** It lets a class have its own signals and slots. `DatabaseManager` does not need it, because it has none.

### F6. How is the interface organised?
- **Short answer:** Sidebar on the left, pages on the right in a `QStackedWidget`, one page visible at a time.

### F7. How is the appearance controlled?
- **Short answer:** One style sheet template, `resources/styles/finora.qss`, for both themes. Colours are written as names such as `{{surface}}`; `applyTheme()` replaces them with the light or dark values from `ThemeColors`, sets Qt's Fusion style, a matching `QPalette` and the Inter font. See L1 and L2.

### F8. How do the GUI and the database interact?
- **Short answer:** `main()` opens the database before creating the window. The window does not use it yet.
- **Detailed explanation:** The Transactions page calls `DatabaseManager` functions and reloads the list after every change. The interface never contains SQL; Qt SQL is linked privately to the database library. The full path from a button click to SQLite is in section K3.

### F9. How does `Finora.exe` run without Qt installed?
- **Short answer:** After each build, `windeployqt` copies the Qt DLLs, plugins and MinGW runtime next to `Finora.exe`.
- **Detailed explanation:** Because Finora uses Qt SQL, `windeployqt` would copy all six SQL drivers. `CMakeLists.txt` excludes five of them, so only `Qt6Sql.dll` and `sqldrivers\qsqlite.dll` are copied.

---

## G. Validation, Errors and Testing

### G1. How does the program validate input?
- **Short answer:** Three times: in the form with `parseAmount` and `validateTransaction`, again in `DatabaseManager`, and finally inside SQLite with CHECK and FOREIGN KEY rules.
- **Detailed explanation:** The form shows a red message and stays open until the input is valid. The date field only allows real dates between 1900 and 2100, and the currency list only offers PKR, USD and EUR.

### G2. What happens if a transaction has zero, a negative amount or invalid data?
- **Short answer:** `addTransaction` returns `false`, nothing is saved, and `lastError()` explains why.
- **Example:** The test tries 10 invalid transactions and gets messages such as "Amount must be greater than zero." and "Date must be a real date written as YYYY-MM-DD."; the table stays empty.

### G3. What happens if a transaction is edited or deleted?
- **Short answer:** Editing opens the same form filled with the old values, and Save runs an UPDATE. Deleting asks "Delete this transaction?" with "No" as the default, and only "Yes" runs the DELETE.
- **Detailed explanation:** After either change, the page reloads every row from SQLite and shows a green message. If the database refuses, a warning explains why and the table does not change. See K5 and K6.

### G4. How did you test the program?
- **Short answer:** Three automated test programs, run with `ctest --preset debug` or `build.bat`.

| Program | Checks | Result on 30 Oct 2026, Windows |
|---|---|---|
| `finora_tests` | Greeting, leap years, days per month, 17 dates, currencies, validation, 22 typed amounts, formatting, search and filters; checked addition, months, totals per currency, money flow, spending per category, chart scale, every edge of the budget rules | 199 of 199 pass |
| `finora_ui_tests` | The real window and all 7 buttons; the Transactions page on a real database; the form's validation; the dashboard's wallet cards, chart and alerts; budgets created, refused, edited and deleted through the real forms; theme switching, reduce motion and the saved theme after a restart | 131 of 131 pass |
| `finora_db_tests` | Real SQLite files: creation, reopening, saving, reading, updating, deleting, punctuation, Urdu text, invalid data, errors, open failures, persistence, SQLite rules, ids never reused, single-copy lock; the upgrade from version 1 with its backup, two deliberate upgrade failures, settings, budgets, timestamps | 157 of 157 pass |

The single-copy guard was also tested with the real `Finora.exe`: a second copy showed "Finora is already open", and a copy killed on purpose left a lock file that the next start took over.

### G5. What edge cases did you consider?
- **Short answer:** Boundaries and unusual but valid input.
- **Examples:** leap days in 2024 and 2000 versus 2026 and 1900; 31 April; month 0 and 13; the maximum amount and one above it; lowercase "pkr"; apostrophes and SQL-like text; Urdu text; opening the same database twice; two objects with one connection name; a file where a folder should be; a folder where a file should be; a text file instead of a database; a database from a newer version.

### G6. What bugs did you encounter, and how were they fixed?
- **Wrong compiler.** An old GCC 6.3 cannot build Qt 6. **Fix:** `CMakePresets.json` names the Qt compiler's full path.
- **Interface test crashed at start.** The "offscreen" plugin was not in the build folder. **Fix:** the tests also look in Qt's plugin folder.
- **Description text cut off.** A wrapped, centred label with a maximum width got too little height. **Fix:** the label spans the card's width.
- **Build failed with "Permission denied".** Finora.exe was still running, and Windows cannot replace a running program. **Fix:** close Finora before building.
- **Too many SQL drivers.** Adding Qt SQL made `windeployqt` copy six drivers, five needing software that is not installed. **Fix:** `--exclude-plugins` keeps only SQLite.
- **A member hid a Qt function.** In `TransactionDialog`, a member variable named `result` hid `QDialog::result()`, so the build failed. **Fix:** it is now called `savedTransaction`.
- **Amount header out of line.** The "Amount" header was left-aligned above right-aligned numbers. **Fix:** the header is right-aligned too.
- **Two date styles.** The delete question showed "2026-09-02" while the table showed "02 Sep 2026". **Fix:** both use the table's style, and the question also shows the description.

---

## H. Code Walkthrough

### H1. What is the purpose of each major source file?

| File | Purpose |
|---|---|
| `src/main.cpp` | Starts the program, loads the fonts, applies the theme, asks before upgrading an older database, opens it, shows the window |
| `src/core/Greeting.h/.cpp` | Time-of-day greeting |
| `src/core/Transaction.h/.cpp` | Transaction structure, supported currencies, suggested categories, leap years, dates, validation |
| `src/core/Money.h/.cpp` | Reading typed amounts exactly; writing amounts as "1,200.50" |
| `src/core/TransactionFilter.h/.cpp` | Search and filters |
| `src/core/Totals.h/.cpp` | Checked totals per currency, money flow per month, spending per category, chart scale |
| `src/core/Budget.h/.cpp` | Budget structure, status enum, the 80% / 100% / over rules |
| `src/database/DatabaseManager.h/.cpp` | Open, schema and safe upgrade with backup, transactions, settings, audit history, errors |
| `src/database/BudgetStore.h/.cpp` | All budget SQL |
| `src/database/InstanceLock.h/.cpp` | Lets only one copy of Finora run at a time |
| `src/ui/TransactionsPage.h/.cpp` | The Transactions page |
| `src/ui/TransactionDialog.h/.cpp` | The Add / Edit Transaction form |
| `src/ui/MainWindow.h/.cpp` | Window, pages, page switching, window size |
| `src/ui/Sidebar.h/.cpp` | Logo, navigation buttons; defines the page numbers |
| `src/ui/PageHeader.h/.cpp` | Reusable title and subtitle |
| `src/ui/PlaceholderPage.h/.cpp` | "Coming in Milestone N" page |
| `src/ui/Page.h` | Base class of every page, with `virtual void refresh()` |
| `src/ui/DashboardPage.h/.cpp` | The dashboard |
| `src/ui/BudgetsPage.h/.cpp`, `BudgetDialog.h/.cpp` | The Budgets page and its form |
| `src/ui/SettingsPage.h/.cpp` | Theme, reduce motion, data location, font licences |
| `src/ui/components/` | `WalletCard`, `MoneyFlowChart`, `ProgressRing`, `MonthSwitcher`, `Toast` |
| `src/ui/Theme.h/.cpp` | Colour tokens of both themes, fonts, style sheet template, recoloured icons, reduce motion |
| `tests/finora_tests.cpp` | Core logic tests |
| `tests/finora_ui_tests.cpp` | Interface tests |
| `tests/finora_db_tests.cpp` | Database tests on real SQLite files |
| `CMakeLists.txt` | How to build the libraries, programs and tests |

### H2. Explain `isLeapYear` and `daysInMonth`.
```cpp
bool isLeapYear(int year)
{
    if (year % 400 == 0) { return true; }    // 2000: leap year
    if (year % 100 == 0) { return false; }   // 1900, 2100: not
    return year % 4 == 0;                    // 2024: yes, 2026: no
}

int daysInMonth(int year, int month)
{
    switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
        return 31;
    case 4: case 6: case 9: case 11:
        return 30;
    case 2:
        if (isLeapYear(year)) { return 29; }
        return 28;
    default:
        return 0;                            // no such month
    }
}
```

### H3. Explain `DatabaseManager::addTransaction` step by step.
1. If the database is not open, return `false` with "The database is not open."
2. Call `validateTransaction`. If it returns a message, return `false` with that message. No SQL runs.
3. Prepare an INSERT with placeholders `:type`, `:amount`, `:currency`, `:category`, `:description` and `:date`.
4. Bind each placeholder to the matching field, converting `std::string` to `QString`.
5. Execute. If SQLite refuses, return `false` with "Could not save the transaction:" and SQLite's reason.
6. Store `lastInsertId()` in `transaction.id` and return `true`.

### H4. Explain `DatabaseManager::open` step by step.
1. Refuse if already open, if the path is empty, if the connection name is already in use, or if the SQLite driver is missing.
2. Create the folder with `QDir().mkpath`.
3. Inside braces: add the connection, set the file name and open it; remember any reason for failure. The handle dies at the closing brace.
4. If SQLite could not open the file, remove the connection and return `false`.
5. Switch on foreign keys and check them, then run `upgradeSchema`.
6. If either step failed, `close()` and return `false`; the message stays in `lastError()`.

### H5. Trace complete examples.
**Start-up:**
1. `main()` creates `QApplication` and applies the theme.
2. `defaultDatabasePath()` returns `...\AppData\Local\Finora\finora.db`.
3. `database.open(path)` creates the folder and the file. `user_version` is 0, so the tables and currencies are created and the version is set to 1.
4. The window is created and shown.
5. When the window closes, `main()` returns and the destructor closes the database.

**Saving, as done in `finora_db_tests`:**
1. A `Transaction` is filled: expense, 120050, PKR, Food, Groceries, 2026-10-28.
2. `addTransaction` validates it: type, amount, currency, category and date are all fine.
3. The prepared INSERT runs, and SQLite's CHECK and FOREIGN KEY rules pass.
4. SQLite gives the row id 1, which is copied into `transaction.id`.
5. `findTransactionById(1, loaded)` reads it back, and every field equals the original.

**Saving from the screen:** see section K3.

### H6. Where does each syllabus concept appear?
See `docs/CPP_CONCEPTS_MAP.md`.

---

## I. Design Decisions and Limitations

### I1. Why did you choose this technology stack?
- **Short answer:** C++17 is the course language; Qt is a C++ GUI library named in the brief; SQLite needs no server; CMake builds it all. Everything was installed and verified on this computer.

### I2. Why a local database rather than a cloud backend?
- **Short answer:** Simpler, private, and works without internet. The whole database is one file the user can back up by copying.

### I3. Why is internet access not required?
- **Short answer:** Everything runs and is stored on the user's computer. Exchange rates will be typed in by the user and never claimed to be live.

### I4. Why is all SQL inside one class?
- **Short answer:** So the interface never depends on SQL, and a database change affects only `DatabaseManager`.

### I5. Why were budgets added as a new schema version instead of in version 1?
- **Short answer:** Nothing used them before Milestone 4. Adding them as schema version 2 also proved that the upgrade path works on real Milestone 3 files, with a backup and a rollback test.

### I6. Why not use Qt Charts?
- **Short answer:** Qt Charts is deprecated since Qt 6.10. Finora's money-flow chart is drawn by Finora itself with `QPainter` in `MoneyFlowChart::paintEvent`: a few loops, rounded rectangles and text. See L7. Report charts are **Planned — Not Yet Implemented** (Milestone 6).

### I7. What limitations remain?
No accounts yet (Phase 4B), so all records belong to "this computer"; no exchange rates, so there is no single total across currencies; filters work in memory, which suits one person's data; search and budget matching ignore case only for A to Z; the table cannot be re-sorted by clicking a header; a budget has the same limit every month; no savings goals or reports yet; build paths assume `C:\Qt`. Only one copy of Finora can run at a time; a second copy says Finora is already open and closes.

### I8. What would you improve in a future version?
Answer this after the project is complete.

---

## J. Demonstration and Unexpected Questions

### J1. Suggested live demonstration, Milestone 3
The full step-by-step script is in `docs/DEMO_MILESTONE3.md`. In short:
1. Run `build.bat` and show "100% tests passed, 0 tests failed out of 3".
2. Start `Finora.exe`, open Transactions, and add an expense of "1,200.50" PKR; point at "Will be saved as PKR 1,200.50".
3. Try "12.345" and show the red message.
4. Add an income in USD, then use search and the filters.
5. Edit a transaction, then delete one, answering No first and then Yes.
6. Close Finora, start it again, and show the data is still there.
7. Start a second copy and show "Finora is already open".
8. Open `src/core/Money.cpp` and explain `parseAmount` with K4.

### J2. Most likely difficult follow-up questions
- **"What happens if the power fails while the tables are being created?"** Creation runs inside one database transaction, so SQLite keeps either all of it or none of it.
- **"Why store the date as text?"** The ISO form YYYY-MM-DD sorts correctly as text, and SQLite's `date()` function understands it, which the CHECK rule uses.
- **"Why does `open()` not happen in the constructor?"** Opening can fail. A constructor cannot return `false`, but `open()` can.
- **"What if two copies of Finora run at once?"** It cannot happen. Before opening the database, `main()` locks `finora.lock` with Qt's `QLockFile` through my `InstanceLock` class. A second copy finds the lock taken, says Finora is already open, and closes. I added this because SQLite allows only one program to write at a time: in my tests, two programs saving at once became very slow and some saves were rejected with "database is locked". If Finora crashes, the lock file stays, but `QLockFile` sees that the program which wrote it is no longer running and the next start takes it over. I set the stale lock time to 0, because Qt's default would treat a lock older than 30 seconds as abandoned, even while Finora is still open.
- **"Why `AUTOINCREMENT`?"** So an id is never reused after a deletion.
- **"Can an apostrophe break your SQL?"** No. Values are bound to placeholders, never glued into the SQL text; the test proves it.

### J3. Questions to check your own understanding
1. Which statement in `main()` would you change to store the database elsewhere?
2. Why does `close()` check `opened` before removing the connection?
3. What would happen if `PRAGMA foreign_keys = ON` were removed?
4. Why does `upgradeSchema()` refuse version 99 instead of trying to use it?
5. What does `query.next()` return after the last row?

### J4. Challenge questions on edge cases and trade-offs
- **"Is 2100 a leap year?"** No. It divides by 100 but not by 400, and the test checks it.
- **"What is the largest amount one transaction can have, and why that limit?"** 9,999,999,999.99. It is a sensible business limit and far below the `long long` maximum, but totals will still need checked addition.
- **"Why reject lowercase pkr instead of converting it?"** Codes are stored exactly as the foreign key requires. Accepting variants would risk two spellings of one currency.

### J5. Did you use any outside help or tools?
- **How to answer:** Be fully honest. Name every tool and resource you used, for example the Qt documentation, course notes, and any AI assistant, and follow your university's AI-assistance policy. Explain which parts you studied, changed or tested yourself.
- **Why it matters:** The academic integrity criterion, R4, rewards acknowledging external resources and being able to explain the work.

---

## K. Milestone 3: The Transactions Page

### K1. What can the user do on the Transactions page?
- **Short answer:** See all transactions, newest first; add, edit and delete them; search by description or category; filter by type, currency, category and period.
- **Where in the code:** `src/ui/TransactionsPage.cpp`.

### K2. Which classes take part, and what does each do?
- **Short answer:** `TransactionsPage` is the page, `TransactionDialog` is the form, `DatabaseManager` does the SQL, and plain C++ functions in `src/core` hold the rules.
- **Detailed explanation:**

| Class or function | Job | OOP idea it shows |
|---|---|---|
| `TransactionsPage` | Table, filters, buttons, messages | Inherits `QWidget`; keeps a **reference member** `DatabaseManager &database` |
| `TransactionDialog` | The Add / Edit form | Inherits `QDialog`; **two constructors** (overloading); private widgets and a private `readForm` |
| `DatabaseManager` | `loadTransactions`, `addTransaction`, `updateTransaction`, `deleteTransaction` | Encapsulation: the SQL and the connection stay private |
| `parseAmount`, `formatAmount` | Read and write amounts | Plain functions with parameters and return values |
| `filterTransactions`, `matchesFilter` | Search and filters | Loops and conditions over a `std::vector` |

### K3. Trace adding a transaction from the click to SQLite and back.
1. The user clicks **+ Add Transaction**. The `clicked` signal calls the slot `onAddClicked`.
2. `onAddClicked` creates a `TransactionDialog` and calls `exec()`, which shows the form and waits.
3. The user fills the form and clicks **Save Transaction**. That calls `onSaveClicked`, which calls `readForm`.
4. `readForm` turns the amount text into minor units with `parseAmount`, then checks everything with `validateTransaction`. If something is wrong, a red message appears and the form stays open.
5. If all is valid, `accept()` closes the form and `exec()` returns `QDialog::Accepted`.
6. The page reads the result with `dialog.transaction()` and calls `saveNewTransaction`, which calls `database.addTransaction`.
7. `DatabaseManager` validates again and runs a prepared `INSERT`, binding each value to a placeholder. SQLite stores the row and gives it an id.
8. The page calls `reloadTransactions`. `loadTransactions` reads every row back with `SELECT ... ORDER BY transaction_date DESC, id DESC` into the vector.
9. `applyFilters` runs `filterTransactions` on the vector, and `showTransactions` fills the table.
10. A "Transaction added." success banner appears and hides itself after 4 seconds.
- **Example:** typing "1,200.50" with PKR stores `amount_minor = 120050`, and the table shows "PKR 1,200.50".

### K4. Explain `parseAmount` step by step.
- **Short answer:** It reads the text one character at a time and builds the whole number and the paisa separately, using only `long long` and `int`.
- **Detailed explanation:** Spaces at both ends are removed. A digit before the point updates `whole = whole * 10 + digit`; a digit after the point updates `fraction`, and a third digit after the point is an error. A comma is allowed only between two digits of the whole number. Any other character is an error. After the loop, one digit after the point counts as tens of paisa ("0.5" is 50). The result is `whole * 100 + fraction`, which must be above 0 and at most the maximum.
- **Where in the code:** `src/core/Money.cpp`; pseudocode in `docs/DESIGN.md`, section 9.2.
- **Example:** "1,20,000" gives `whole = 120000`, `fraction = 0`, result `12000000`, shown as "120,000.00".

### K5. How does editing work?
- **Short answer:** Select a row and click **Edit**, or double-click the row. The same form opens with the old values, and Save runs an `UPDATE`.
- **Detailed explanation:** Each row keeps its transaction id hidden in its first cell, set with `setData(Qt::UserRole, id)`. `selectedTransactionId` reads it back, and `findLoadedTransaction` finds that transaction in the vector with a linear search. `TransactionDialog(existing)` fills the form. On Save, `saveEditedTransaction` calls `updateTransaction`, which uses `WHERE id = :id`, so only that row changes.

### K6. How does deleting work, and why ask first?
- **Short answer:** **Delete** shows "Delete this transaction?" with the date, category, amount and description. "No" is the default button; only "Yes" deletes.
- **Detailed explanation:** A deleted transaction cannot be recovered, so the program asks. `QMessageBox::question` returns the button the user clicked. On "Yes", `deleteTransaction` runs `DELETE ... WHERE id = :id` and checks `numRowsAffected()`, so deleting a row that no longer exists is reported, not shown as a success.

### K7. How do the Qt widgets talk to the C++ logic?
- **Short answer:** Through signals and slots. Each `connect` links a widget's signal to one of the page's functions.
- **Examples from `TransactionsPage.cpp`:** `clicked` of the Add button calls `onAddClicked`; `textChanged` of the search box calls `applyFilters`; `currentIndexChanged` of each filter calls `applyFilters`; `itemSelectionChanged` of the table calls `onSelectionChanged`, which enables Edit and Delete; `cellDoubleClicked` calls `onEditClicked`.
- **Note:** `cellDoubleClicked` sends a row and a column, but `onEditClicked` takes no parameters. Qt allows a slot to ignore the extra values.

### K8. Why does the page keep a reference to the database instead of its own `DatabaseManager`?
- **Short answer:** There must be exactly one connection, created in `main()`. The page only uses it, so it holds a reference, written `DatabaseManager &database`.
- **Detailed explanation:** A reference is another name for an existing object. It must be set in the constructor's initializer list, `database(sharedDatabase)`, and it can never point to anything else afterwards. Because the page does not own the database, it never opens or closes it.

### K9. Why do you reload everything after each change instead of editing the table directly?
- **Short answer:** So the screen always shows exactly what SQLite stored, including ids and the order.
- **Detailed explanation:** It costs one `SELECT` per change, which is instant for one person's data, and it removes a whole class of bugs where the table and the database disagree.

### K10. Why are Edit and Delete separate buttons that act on the selected row?
- **Short answer:** It is simpler and clearer: one Edit and one Delete button, enabled only when a row is selected.
- **Detailed explanation:** Buttons inside every row would need one widget per row and a way to know which row's button was pressed. With a selection, the page reads the selected row's hidden id.

### K11. How was the page tested automatically, when the form waits for the user?
- **Short answer:** Just before clicking the button that opens a pop-up, the test starts a 0 ms `QTimer`. The timer fires inside the pop-up's own event loop and plays the user: it fills the form, or clicks Yes or No.
- **Where in the code:** `fillAddDialogAndSave`, `answerNo` and `answerYes` in `tests/finora_ui_tests.cpp`. The test has a 120-second timeout in CTest, so a stuck pop-up cannot hang it.

### K12. What happens if the database fails while the page is open?
- **Short answer:** A warning box says what could not be done and why, and the table keeps showing the last good data.
- **Example:** The interface test closes the database and tries to save. The warning reads "Could not save the transaction. Reason: The database is not open.", and the table still shows 4 rows.

---

## L. Milestone 4, Phase 4A: Dashboard, Budgets, Themes and the Safe Upgrade

### L1. What did Phase 4A add?
- **Short answer:**
  - A **dashboard**: one wallet card per currency, a money-flow chart, budget alerts, recent transactions and spending by category.
  - A **Budgets** page for monthly limits per category.
  - A **light and a dark theme**, and a **Settings** page.
  - **Schema version 2**, with a safe upgrade of older database files.
- **Where in the code:** `src/ui/DashboardPage.cpp`, `src/ui/BudgetsPage.cpp`, `src/ui/SettingsPage.cpp`, `src/ui/Theme.cpp`, `src/core/Totals.cpp`, `src/core/Budget.cpp`, `src/database/BudgetStore.cpp`, `DatabaseManager::upgradeSchema`.

### L2. How does switching between the light and the dark theme work?
- **Short answer:**
  - Every colour has a name, such as `surface` or `income`, in the structure `ThemeColors`. The light and dark themes give each name a different value.
  - The style sheet template `finora.qss` writes colours as `{{surface}}`.
  - `applyTheme()` replaces every name with the current theme's colour and applies the result to the whole program.
- **Detailed explanation:** `MainWindow::setDarkTheme` then:
  1. calls `applyTheme(*qApp, mode)`;
  2. saves `"dark"` or `"light"` in the `app_settings` table;
  3. recolours the sidebar icons;
  4. refreshes the current page;
  5. shows a toast.
  At the next start, `MainWindow` reads the saved value and applies it before any page is built.
- **Where in the code:** `fillStyleTemplate` and `applyTheme` in `src/ui/Theme.cpp`; `MainWindow::setDarkTheme`.
- **Example:** `background-color: {{surface}};` becomes `#ffffff` in the light theme and `#181430` in the dark theme.

### L3. How do you upgrade an old database without losing data?
- **Short answer:**
  1. Ask first.
  2. Save a backup and check it.
  3. Do every change inside one database transaction.
  4. Prove that every row was copied unchanged.
  5. Roll back everything if any step fails.
- **Detailed explanation:**
  - `main()` reads the version without changing the file (`storedSchemaVersion` opens it read-only) and asks "Upgrade and continue" or "Quit without changes".
  - `backupBeforeUpgrade` writes a full copy with `VACUUM INTO`. A second, read-only connection then checks the copy: SQLite's integrity check, the version number and the number of transactions.
  - Only then does `upgradeToVersion2` run, between `BEGIN` and `COMMIT`. It compares the row count and the sum of all amounts before and after copying, keeps the highest id ever used, and runs `PRAGMA foreign_key_check`.
  - Any failure calls `rollback()`, and the message says where the backup is.
- **Where in the code:** `upgradeSchema`, `backupBeforeUpgrade`, `verifyBackup`, `upgradeToVersion2` in `DatabaseManager.cpp`; `askToUpgrade` in `main.cpp`. Flowchart in `docs/DESIGN.md`, section 10.3.
- **Example:** the test stores a transaction in the currency `GBP`, which is not allowed, and then upgrades. The upgrade fails while copying, and the file stays byte-for-byte the same.

### L4. How is a budget's progress calculated?
- **Short answer:** `budgetProgressFor` loops over the transactions. It adds, with `addChecked`, every expense with the budget's currency, the same category (ignoring upper and lower case) and a date in the chosen month. Then:
  - `remaining = limit - spent`;
  - `percentUsed` is rounded down;
  - `budgetStatusFor` gives the status.
- **Where in the code:** `src/core/Budget.cpp`; pseudocode in `docs/DESIGN.md`, section 10.5.
- **Example:** a Food budget of PKR 20,000.00 with PKR 16,500.00 and PKR 1,580.50 spent in October gives 18,080.50 spent and 1,919.50 left. That is 90%, so the status is "Nearing limit".

### L5. Why `5 * spent >= 4 * limit` instead of `spent / limit >= 0.8`?
- **Short answer:** It means the same thing but uses only whole numbers, so there is no rounding and no `double`.
- **Detailed explanation:**
  - Integer division `spent / limit` would give 0 for anything below 100%.
  - A `double` can be slightly off.
  - Multiplying both sides by 5 removes the division.
  - The line runs only when `spent < limit`, and the limit is at most 999,999,999,999, so `5 * spent` stays far below the `long long` limit.
- **Example:** 7,999 of 10,000 gives 39,995 < 40,000, so the status is "On track". 8,000 of 10,000 gives 40,000 >= 40,000, so it is "Nearing limit".

### L6. Why is there no single "total balance" for all currencies?
- **Short answer:** Adding rupees to dollars gives a meaningless number. Without exchange rates, every total belongs to one currency, so each currency has its own wallet card.
- **Detailed explanation:** Exchange rates entered by the user, with clearly labelled converted totals, are **Planned — Not Yet Implemented** (Milestone 5).

### L7. How did you draw the money-flow chart?
- **Short answer:** `MoneyFlowChart` overrides `paintEvent` and draws with `QPainter`: grid lines, labels, one teal and one rose bar per month, and the month names. No chart library is used.
- **Detailed explanation:**
  - `chartScaleMaximum` picks a round top value with four equal steps.
  - Each bar's height is the plot height × value ÷ top value.
  - Moving the mouse or pressing Left and Right highlights a month and draws a bubble with its exact amounts. The same text appears under the chart for keyboard and screen-reader users.
  - A `QVariantAnimation` grows the bars from 0 to full height, unless "Reduce motion" is on.
- **Where in the code:** `src/ui/components/MoneyFlowChart.cpp`.

### L8. How does the dashboard stay up to date when you add a transaction on another page?
- **Short answer:** `MainWindow::showPage` calls the page's `refresh()` every time a page is shown. `DashboardPage::refresh()` reloads the transactions and budgets from SQLite and redraws every card, so it always shows what is really saved.

### L9. Explain polymorphism in Finora.
- **Short answer:** `Page` has `virtual void refresh()`. Each page overrides it. `MainWindow` stores all pages as `Page *` and calls `pages[n]->refresh()`, and C++ runs the version of the page's real class at run time.
- **Detailed explanation:** `override` asks the compiler to check that the function really replaces one in the base class. Qt uses the same idea: `MoneyFlowChart` and `ProgressRing` override `paintEvent`, and Qt calls them whenever they must be drawn.
- **Where in the code:** `src/ui/Page.h`, `MainWindow::showPage`, `DashboardPage::refresh`, `BudgetsPage::refresh`.

### L10. Where do you use inheritance and where composition?
- **Short answer:**
  - **Inheritance means "is a".** A `DashboardPage` *is a* `Page`, and a `WalletCard` *is a* `QFrame`.
  - **Composition means "has a".** `MainWindow` *has a* `BudgetStore` member, and `BudgetStore` *has* a reference to the `DatabaseManager` it works with.
- **Detailed explanation:** A budget store is not a kind of database, so inheriting from `DatabaseManager` would be wrong. It only uses the database's connection.

### L11. How did you make the interface accessible?
- **Short answer:**
  - Keyboard use everywhere, with visible focus rings.
  - Text contrast of at least 4.5 : 1 in both themes, checked with the WCAG formula.
  - Status always in words, not only in colour.
  - Toasts announced to screen readers.
  - A "Reduce motion" setting.
- **Example:** an over-budget card shows a rose ring, but also the words "Over budget" and "Over by USD 35.00". Amounts carry a + or - sign.

### L12. Why does schema version 2 have tables for users and sessions when sign-in does not exist yet?
- **Short answer:** The rebuilt `transactions` table has an owner column, `user_id`, with a foreign key to `users`, and a foreign key needs its table. Creating all Milestone 4 tables in one upgrade copies the user's records only once.
- **Detailed explanation:**
  - Existing records keep `user_id = NULL`, which means "made before accounts existed".
  - Sign-up and sign-in are **Planned — Not Yet Implemented** (Phase 4B).
  - Finora will ask before moving existing records into a new account.

### L13. Where do the fonts come from, and may you include them?
- **Short answer:** Inter 4.1 (from its official release) and Manrope (from its official repository) are both under the SIL Open Font License 1.1. The licence allows bundling them with a program, even a commercial one, as long as the licence text goes with them.
- **Detailed explanation:** The licence files are built into Finora.exe and shown under Settings > View font licences. `THIRD_PARTY_NOTICES.md` lists every outside font and library.

### L14. How was Phase 4A tested?
- **Short answer:** 487 automated checks, all passing: 199 core, 131 interface and 157 database checks. Previously there were 284.
- **Detailed explanation:**
  - The core tests check every edge of the budget rules (79.99%, 80%, 100%, 100.01%), checked addition with an overflow, month arithmetic across a new year, and totals that never mix currencies.
  - The database tests upgrade a real version 1 file and make it fail twice on purpose. Both failures leave the file byte-for-byte unchanged.
  - The interface tests click the real buttons:
    - create a budget through the form;
    - try a duplicate;
    - edit and delete with a confirmation;
    - check the wallet card amounts and the chart data;
    - press arrow keys on the chart;
    - switch themes;
    - check that the theme is remembered after a restart.
  - They also save screenshots of every page in both themes.

### L15. Trace adding an expense on the dashboard when a budget exists.
1. Click **+ Add Transaction** on the dashboard. The same `TransactionDialog` as on the Transactions page opens.
2. Save PKR 1,200.50 for Food. `DashboardPage::saveNewTransaction` calls `database.addTransaction`, which validates the transaction and runs a prepared `INSERT`.
3. `refresh()` reloads everything from SQLite.
   - `summarizeByCurrency` adds 1,200.50 to the PKR card's "Spent in October".
   - `monthlyFlow` makes the October spending bar taller.
   - `allBudgetProgress` recalculates the Food budget. If it reaches 80%, the budget appears under "Budget alerts".
4. A toast says "Transaction added.".

---

## M. Milestone 4, Phase 4B: Accounts and Security

### M1. What did Phase 4B add?
- **Short answer:** Local accounts.
  - Sign-up, sign-in and sign-out, a profile, and a password change.
  - Recovery codes for a forgotten password.
  - Sessions that end by themselves.
  - Most important: every transaction, budget and setting belongs to one account, and nobody sees another person's records.
- **Where in the code:** `src/auth/`, `src/database/UserStore.cpp`, `src/ui/AuthWindow.cpp`, `src/ui/AppController.cpp`.

### M2. How is a password stored? Why not encrypt it?
- **Short answer:** Finora stores a one-way **hash**, made with **Argon2id** by libsodium, never the password. Encryption can be reversed with a key; a hash cannot be turned back into the password.
- **Detailed explanation:** To sign in, Finora hashes the typed password again with the salt and settings saved in the stored string, and compares the result. If the database file were stolen, the attacker would still have to guess passwords one by one. Each guess costs 256 MiB of memory and three passes over it.
- **Example:** `$argon2id$v=19$m=262144,t=3,p=1$<salt>$<hash>` means Argon2id version 19, 262,144 KiB (256 MiB), 3 passes, 1 lane.

### M3. What is a salt, and why must each one be different?
- **Short answer:** A random value mixed into the hash. With a different salt for every hash, two people with the same password get different hashes, and an attacker cannot use one precomputed table for everybody.
- **Where in the code:** `crypto_pwhash_str` in `PasswordHasher::hash` makes a new 16-byte salt each time. A test checks that two accounts with the same password have different hashes.

### M4. Why the MODERATE setting? How do you know it really uses 256 MiB?
- **Short answer:** MODERATE makes each guess expensive, about 0.65 s and 256 MiB, but is still quick enough to sign in.
- **Detailed explanation:** The test `finora_auth_tests` reads Windows' memory counters for its own process. Peak memory went from about 9 MiB to about 265 MiB during one hash and fell back afterwards.
  - If a computer cannot provide the memory, `hash()` returns `false`. Finora then shows a clear message and creates no account.
  - The test forces this with a 512 GiB setting.
  - Recovery codes use the lighter INTERACTIVE setting (64 MiB), because they are already random.

### M5. How does sign-in avoid telling an attacker which emails have accounts?
- **Short answer:** A wrong password and an unknown email give exactly the same message ("The email or password is not correct."). Both have the same waiting times after 3 failures. For an unknown email, Finora still checks the password against a dummy hash, so the answer takes about as long.
- **Where in the code:** `AuthService::signIn`, `failForUnknownEmail`, `dummyPasswordHash`.

### M6. How do the waiting times after wrong passwords work?
- **Short answer:** `signInDelaySeconds(failures)` is 0 for the first two failures, 30 seconds after the third, then doubles each time up to 15 minutes.
- **Detailed explanation:** `users.locked_until` stores the end of the waiting time. While it lasts, not even the right password is checked. A successful sign-in sets the counter back to 0.

### M7. How do you make sure that a person sees only their own records?
- **Short answer:** Every transaction and budget row has an owner, `user_id`. Every SQL statement says `WHERE ... AND user_id IS :owner`. The owner always comes from the `Session`, never from the screen.
- **Detailed explanation:** `DatabaseManager::attachSession` connects the database to the program's one `Session`.
  - Signed in: the owner is the person's id.
  - Signed out: every data function refuses with "Please sign in first."
- **Example:** Bilal asks for Amna's transaction by its id. He gets "No transaction with id 5 was found.", exactly as for an id that does not exist.

### M8. What is a friend class, and why did you use one?
- **Short answer:** A friend class may use another class's private members. `Session` declares `friend class AuthService;`, so only `AuthService` can call `start()` and `end()`.
- **Detailed explanation:** Every page can read who is signed in, but no page can make itself someone else. This is encapsulation with one controlled exception.

### M9. What are recovery codes, and why are they single-use and hashed?
- **Short answer:** Ten random codes such as `7KQ4-M2TX-9RHC`, shown once at sign-up. Each code resets a forgotten password once.
- **Detailed explanation:**
  - They are hashed like passwords, so a stolen database does not reveal them.
  - When a code is used, `used_at` is set in the same transaction that stores the new password, so the code can never work again.
  - The alphabet has no 0, 1, I or O. That gives 32 characters, and 32^12 = 2^60 possible codes.

### M10. How do sessions end?
- **Short answer:** Six ways:
  - "Sign out";
  - 15 minutes without a key press or mouse action;
  - 12 hours after signing in;
  - closing Finora;
  - resetting the password;
  - a crash (closed at the next start).
- **Detailed explanation:** Each one writes its own reason into `login_sessions`. For the idle time, `AppController` uses an **event filter**: it overrides the virtual function `eventFilter`, which Qt calls for every event of the program.

### M11. What happened to the records made before accounts existed?
- **Short answer:** Nothing, until a person agrees. They keep `user_id = NULL`, so nobody sees them.
- **Detailed explanation:**
  - After sign-up, Finora asks once whether to move them. The default button is "Not now". Settings offers the same move later.
  - The move runs in one transaction: it checks that every transaction moved, and keeps out budgets that would duplicate the person's own.

### M12. Why did you add an outside library, and how did you check it?
- **Short answer:** Secure password hashing should never be written yourself, so I used libsodium, a widely used cryptography library.
- **Detailed explanation:**
  1. I downloaded the official MinGW package from two official sources and they matched.
  2. I verified its minisign signature with the official key from libsodium's documentation, and a copy with one changed byte was correctly rejected.
  3. I tested it with this exact compiler and Qt version, and an independent Argon2id implementation (OpenSSL) produced the same results.
  4. Everything is recorded in `third_party/libsodium-1.0.22/VERIFICATION.md`.
- **Licence:** libsodium uses the ISC licence, which allows this use when the notice is kept; the notice is in `THIRD_PARTY_NOTICES.md` and in Settings.

### M13. Why recovery codes instead of a "reset password" email?
- **Short answer:** Finora is offline and has no server. Sending email would need an email service and its secret passwords, and secrets must never be put inside a desktop program.
- **Detailed explanation:** A production online version would email a time-limited reset link to a verified address, and keep recovery codes as a backup. The README has a table comparing the two.

### M14. How was Phase 4B tested?
- **Short answer:** `finora_auth_tests` has 106 checks on real SQLite files, the interface tests have 33 new checks through the real windows, and the core tests have 42 new checks for the rules.
- **Detailed explanation:** Among other things, the tests prove that:
  - the password appears nowhere in the database file;
  - the general messages are identical;
  - two accounts cannot touch each other's records;
  - a recovery code works once;
  - signing out blocks all data access.

### M15. Trace a sign-in from the button to the dashboard.
1. **The window.** `AuthWindow::onSignInClicked` reads the password with `takePassword()`, which empties the box. It shows "Signing in..." and a busy pointer.
2. **The check.** `AuthService::signIn` finds the account and checks the waiting time. It verifies the password with Argon2id, then wipes the password.
3. **The session.** It starts a session row, and `Session::start` stores the user id; only `AuthService`, as a friend, can do this.
4. **The main window.** `AuthWindow` emits `signedIn`. `AppController` creates `MainWindow(database, &auth)`, which shows the name and greets the person.
5. **The data.** `DashboardPage::refresh` loads transactions. `DatabaseManager` adds `user_id IS :owner` with the id from the `Session`, so only this person's records appear.

---

## Quick Revision

- **Finora:** offline Windows finance app in C++17, Qt 6 Widgets and SQLite; Milestones 1 to 3 and Phase 4A of Milestone 4 are done.
- **Start-up:** fonts and theme, then the single-copy lock, then (for an older file) the upgrade question, then `DatabaseManager::open`, then the window, which applies the saved theme. A failure shows the path and reason, then exits with code 1.
- **Transactions page:** list, add, edit, delete with confirmation, search, and filters for type, currency, category and period.
- **Typed amounts:** `parseAmount` reads "1,200.50" as 120050 without `double`; more than two decimals are refused.
- **Filters:** a `std::vector` of transactions and a `for` loop with `matchesFilter`; dates compare as YYYY-MM-DD text.
- **Edit and delete:** the row's hidden id, then `UPDATE` or `DELETE ... WHERE id = :id`; delete asks first, with No as the default.
- **OOP:** constructor overloading in `TransactionDialog` and `BudgetDialog`; reference members; inheritance from Qt classes and from Finora's own `Page`; polymorphism through `virtual void refresh()` and the overridden `paintEvent`; composition in `MainWindow` (it has a `BudgetStore`).
- **Database file:** `%LOCALAPPDATA%\Finora\finora.db`; back up by copying it while Finora is closed.
- **Tables (version 2):** `currencies`, `transactions` (now with `user_id`, `uuid`, `created_at`, `updated_at`), `budgets` (one per category and currency), `app_settings`, `audit_events`, and the Phase 4B account tables.
- **Money:** `long long` minor units; 120050 = 1,200.50; per-transaction maximum 999,999,999,999; every total uses `addChecked`; currencies are never added together.
- **Validation:** `validateTransaction` in C++, plus CHECK and FOREIGN KEY rules in SQLite.
- **Dates:** YYYY-MM-DD, real calendar dates, leap years via `%`; `daysInMonth` uses `switch`.
- **Versioning:** `PRAGMA user_version`; 0 creates the tables, 1 is backed up and upgraded after the user agrees, 2 is up to date, higher is refused.
- **Budgets:** 80% rule `5 x spent >= 4 x limit`; exactly the limit is "Limit reached"; more is "Over budget"; a budget repeats every month.
- **Themes:** `ThemeColors` tokens, one `finora.qss` template with `{{name}}` colours, icons recoloured from one SVG, Inter and Manrope under the OFL 1.1.
- **Prepared statements:** placeholders plus `bindValue`, so there is no SQL injection.
- **RAII:** the destructor closes the database; braces end handle lifetimes before `removeDatabase`; copying is disabled.
- **Loops:** `for` for fixed counts, `while (query.next())` for database rows.
- **Accounts:** passwords stored only as Argon2id hashes (libsodium, MODERATE: 256 MiB, 3 passes) with unique salts; identical messages for a wrong password and an unknown email; waiting times after 3 failures; 10 hashed, single-use recovery codes; sessions end after sign-out, 15 idle minutes or 12 hours.
- **Isolation:** `user_id IS :owner` in every query; the owner comes only from the `Session`, which only `AuthService` (a friend class) can change.
- **Tests:** 241 core, 164 interface, 157 database and 106 account checks (668 in total), all passing on Windows on 30 October 2026.
- **Not built yet:** subscription plans (Phase 4C), goals, exchange rates and reports. Say "planned for Phase 4C" or "Milestone N".
