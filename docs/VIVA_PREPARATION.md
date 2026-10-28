# Finora Viva Preparation

Questions your Programming Fundamentals instructor may ask about Finora, with answers that match the real code. Every answer names the file or function it comes from, so you can open the code and check it yourself.

Features that are not built yet are marked **Planned — Not Yet Implemented**. Never present them as finished.

Last updated: Milestone 2, 28 October 2026.

**How to use this file.** Read the short answer first. Then open the file named under "Where in the code" and find the lines yourself. If you cannot explain a line, study it before the viva.

---

## A. Project Overview

### A1. What is Finora?
- **Short answer:** A Windows desktop program for personal finance and budgeting, written in C++17 with Qt 6 Widgets and a SQLite database.
- **Detailed explanation:** Finora will record income and expenses, track monthly budgets and savings goals, and handle PKR, USD and EUR without mixing them. At Milestone 2 the window, navigation, theme, database and tests work. The finance screens are planned, and every page says which milestone adds it.
- **Where in the code:** `src/main.cpp` starts the program; `src/database/DatabaseManager.cpp` handles the data.

### A2. What problem does it solve?
- **Short answer:** People lose track of what they earn and spend, overspend without warning, and mix currencies in their totals.
- **Detailed explanation:** A notebook cannot warn you at 80% of a budget, and adding dollars to rupees gives a meaningless total. Finora keeps each currency separate, will warn about budgets and show savings progress, and works offline.

### A3. Who are the intended users?
- **Short answer:** Students, young earners and freelancers who want a simple offline money tracker.
- **Detailed explanation:** Freelancers often earn in USD or EUR and spend in PKR, which is why the database already stores a currency with every transaction.

### A4. What are the main features?
- **Short answer:** Done: navigation, theme, database, validation and tests. Planned: the transaction, budget, goal, currency and report screens.
- **Detailed explanation:** See the feature table in `README.md`. The database can already save and read transactions, which the tests prove, but there is no screen to enter them until Milestone 3.

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
- **Detailed explanation:** `struct Transaction` holds one record. `SUPPORTED_CURRENCIES` is an array of 3 `CurrencyInfo` structures and is used both to validate codes and to fill the `currencies` table. `std::vector` is **Planned — Not Yet Implemented** (Milestone 3, the list of transactions).

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
- **Detailed explanation:** The window does not use the database yet. In Milestone 3 the transactions page will call `DatabaseManager`, never SQL directly.

### C7. Did you use inheritance?
- **Short answer:** Only to extend Qt classes, which is how Qt programs are written. `DatabaseManager` inherits nothing.

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
**Planned — Not Yet Implemented** (Milestone 4). Planned approach: a loop adds the income amounts of one currency and month, using checked addition.

### D8. How are total expenses calculated?
**Planned — Not Yet Implemented** (Milestone 4).

### D9. How is the net balance calculated?
**Planned — Not Yet Implemented** (Milestone 4). Planned rule: income − expenses, separately for each currency.

### D10. How are transactions categorised and filtered?
**Planned — Not Yet Implemented** (Milestone 3). Each transaction already stores a category in the database.

### D11. How does the budget alert system work?
**Planned — Not Yet Implemented** (Milestone 4). Agreed rule: an expense counts toward a budget when its category, currency and month match; a warning at 80% and an alert at 100%. Budgets will be added to the database as schema version 2.

### D12. How is savings-goal progress calculated?
**Planned — Not Yet Implemented** (Milestone 5).

### D13. How are currencies converted, and how do you avoid mixing currencies?
**Planned — Not Yet Implemented** (Milestones 4 and 5). Already in place: every transaction stores its own currency code, and nothing in the code adds amounts of different currencies.

### D14. How are invalid amounts and rounding handled?
- **Short answer:** Invalid amounts are rejected now. Rounding is not needed yet, because nothing is converted or typed as text yet.
- **Detailed explanation:** `validateTransaction` rejects 0, negative amounts and amounts above `MAX_AMOUNT_MINOR`, which is 999,999,999,999 paisa or 9,999,999,999.99. SQLite's CHECK rule blocks the same values. Rejecting typed amounts with more than two decimals and rounding conversions are **Planned — Not Yet Implemented** (Milestones 3 and 5).

### D15. Can totals overflow?
- **Short answer:** One transaction cannot, but a total of many could, so future totals must use checked addition.
- **Detailed explanation:** The maximum single amount fits easily in a 64-bit `long long`. Adding enough of them could still pass 9,223,372,036,854,775,807 and silently wrap around to a wrong number. Before each addition, a checked total will test `amount > LONG_LONG_MAX - total` and report an error instead. **Planned — Not Yet Implemented** (Milestone 4). Pseudocode in `docs/DESIGN.md`, section 6.

---

## E. Database and File Handling

### E1. Why was SQLite selected?
- **Short answer:** It is a complete SQL database in one file, needs no server or internet, and Qt supports it directly.
- **Detailed explanation:** A server database such as MySQL would need installing and running a separate program. SQLite is a library inside Finora; Qt loads it from `sqldrivers\qsqlite.dll`.

### E2. Where is the database stored, and why there?
- **Short answer:** `%LOCALAPPDATA%\Finora\finora.db`, which is `C:\Users\<user name>\AppData\Local\Finora\finora.db`.
- **Detailed explanation:** `DatabaseManager::defaultDatabasePath()` asks Qt for `QStandardPaths::AppLocalDataLocation`. That is the standard Windows folder for a program's own data. The program folder may not be writable, and the source folder must never hold personal data. `open()` creates the folder if it is missing.

### E3. What information is stored in the database?
- **Short answer:** Two tables: `currencies` (PKR, USD, EUR) and `transactions` (one row per income or expense).
- **Where in the code:** `createVersion1Tables` in `DatabaseManager.cpp`. Column-by-column explanation in `docs/DESIGN.md`, section 7.2.

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
- **Detailed explanation:** See `docs/DESIGN.md`, section 7.7. Updating and deleting are **Planned — Not Yet Implemented** (Milestone 3).

### E9. How is the schema created, and why is it safe to repeat?
- **Short answer:** `upgradeSchema()` reads the version number stored in the file and creates tables only if the file is new, inside one database transaction.
- **Detailed explanation:** `PRAGMA user_version` is 0 in a new file. Finora then creates both tables, adds the currencies and sets the version to 1, all between `transaction()` and `commit()`. A version of 1 means nothing to do. A higher version means a newer Finora made the file, so it is refused untouched. The statements also use `CREATE TABLE IF NOT EXISTS` and `INSERT OR IGNORE`, so nothing can be duplicated.

### E10. What is a database transaction?
- **Short answer:** A group of statements that is saved completely or not at all.
- **Detailed explanation:** If creating the second table failed, `rollback()` would undo the first one too, so a half-built database can never exist. Do not confuse it with a Finora *money* transaction.

### E11. What happens when the database cannot be opened?
- **Short answer:** Finora shows an error dialog with the file path and the reason, then exits with code 1. It never runs without saving.
- **Where in the code:** `showDatabaseError` and `main()` in `src/main.cpp`.
- **Verified on 28 October 2026:** with a file placed where the folder should be, the real program showed "Reason: Could not create the folder C:\Users\<user name>\AppData\Local\Finora." and exited with code 1.

### E12. How is user data preserved after closing the application?
- **Short answer:** Every saved transaction is written into `finora.db` on disk immediately, and the file is reopened at the next start.
- **Detailed explanation:** The test `testPersistence` saves a record, destroys the `DatabaseManager`, creates a new one, reopens the file and finds the same record with every field equal.

### E13. How can the data be backed up and restored?
- **Short answer:** Close Finora and copy `finora.db`. To restore, close Finora and copy the backup back.
- **Where:** "Data storage, backup and reset" in `README.md`.

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
- **Short answer:** The style sheet `resources/styles/light.qss`, plus Qt's Fusion style and light colours chosen in `applyLightTheme`.

### F8. How do the GUI and the database interact?
- **Short answer:** `main()` opens the database before creating the window. The window does not use it yet.
- **Detailed explanation:** In Milestone 3, the transactions page will call `DatabaseManager` functions. The interface will never contain SQL; Qt SQL is linked privately to the database library.

### F9. How does `Finora.exe` run without Qt installed?
- **Short answer:** After each build, `windeployqt` copies the Qt DLLs, plugins and MinGW runtime next to `Finora.exe`.
- **Detailed explanation:** Because Finora uses Qt SQL, `windeployqt` would copy all six SQL drivers. `CMakeLists.txt` excludes five of them, so only `Qt6Sql.dll` and `sqldrivers\qsqlite.dll` are copied.

---

## G. Validation, Errors and Testing

### G1. How does the program validate input?
- **Short answer:** Twice: in C++ with `validateTransaction`, and inside SQLite with CHECK and FOREIGN KEY rules.
- **Detailed explanation:** Validation of text typed into forms is **Planned — Not Yet Implemented** (Milestone 3).

### G2. What happens if a transaction has zero, a negative amount or invalid data?
- **Short answer:** `addTransaction` returns `false`, nothing is saved, and `lastError()` explains why.
- **Example:** The test tries 10 invalid transactions and gets messages such as "Amount must be greater than zero." and "Date must be a real date written as YYYY-MM-DD."; the table stays empty.

### G3. What happens if a transaction is edited or deleted?
**Planned — Not Yet Implemented** (Milestone 3). Deleting will ask for confirmation first.

### G4. How did you test the program?
- **Short answer:** Three automated test programs, run with `ctest --preset debug` or `build.bat`.

| Program | Checks | Result on 28 Oct 2026 |
|---|---|---|
| `finora_tests` | Greeting, leap years, days per month, 17 dates, currencies, validation | 59 of 59 pass |
| `finora_ui_tests` | The real window and all 7 buttons | 22 of 22 pass |
| `finora_db_tests` | Real SQLite files: creation, reopening, saving, reading, punctuation, Urdu text, invalid data, errors, open failures, persistence, SQLite rules, single-instance lock | 75 checks |

### G5. What edge cases did you consider?
- **Short answer:** Boundaries and unusual but valid input.
- **Examples:** leap days in 2024 and 2000 versus 2026 and 1900; 31 April; month 0 and 13; the maximum amount and one above it; lowercase "pkr"; apostrophes and SQL-like text; Urdu text; opening the same database twice; two objects with one connection name; a file where a folder should be; a folder where a file should be; a text file instead of a database; a database from a newer version.

### G6. What bugs did you encounter, and how were they fixed?
- **Wrong compiler.** An old GCC 6.3 cannot build Qt 6. **Fix:** `CMakePresets.json` names the Qt compiler's full path.
- **Interface test crashed at start.** The "offscreen" plugin was not in the build folder. **Fix:** the tests also look in Qt's plugin folder.
- **Description text cut off.** A wrapped, centred label with a maximum width got too little height. **Fix:** the label spans the card's width.
- **Build failed with "Permission denied".** Finora.exe was still running, and Windows cannot replace a running program. **Fix:** close Finora before building.
- **Too many SQL drivers.** Adding Qt SQL made `windeployqt` copy six drivers, five needing software that is not installed. **Fix:** `--exclude-plugins` keeps only SQLite.

---

## H. Code Walkthrough

### H1. What is the purpose of each major source file?

| File | Purpose |
|---|---|
| `src/main.cpp` | Starts the program, applies the theme, opens the database, shows the window |
| `src/core/Greeting.h/.cpp` | Time-of-day greeting |
| `src/core/Transaction.h/.cpp` | Transaction structure, supported currencies, leap years, dates, validation |
| `src/database/DatabaseManager.h/.cpp` | All SQL: open, schema, save, read, errors |
| `src/ui/MainWindow.h/.cpp` | Window, pages, page switching, window size |
| `src/ui/Sidebar.h/.cpp` | Logo, navigation buttons; defines the page numbers |
| `src/ui/PageHeader.h/.cpp` | Reusable title and subtitle |
| `src/ui/PlaceholderPage.h/.cpp` | "Coming in Milestone N" page |
| `src/ui/Theme.h/.cpp` | Style, font, style sheet loading |
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

Saving from a screen is **Planned — Not Yet Implemented** (Milestone 3).

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

### I5. Why are budgets not in the database yet?
- **Short answer:** Nothing uses them before Milestone 4. Adding them then, as schema version 2, also proves the upgrade path works.

### I6. Why not use Qt Charts?
- **Short answer:** Qt Charts is deprecated since Qt 6.10. Simple charts will be drawn with a few loops. **Planned — Not Yet Implemented** (Milestones 4 and 6).

### I7. What limitations remain?
No transaction screen yet; no budgets, goals or exchange rates in the database yet; light theme only; build paths assume `C:\Qt`. Only one copy of Finora can run at a time; a second copy says Finora is already open and closes.

### I8. What would you improve in a future version?
Answer this after the project is complete.

---

## J. Demonstration and Unexpected Questions

### J1. Suggested live demonstration, Milestone 2
1. Run `build.bat` and show "100% tests passed, 0 tests failed out of 3".
2. Run `build\debug\finora_db_tests.exe` and point at the error messages it prints.
3. Start `Finora.exe`. Press Win + R, type `%LOCALAPPDATA%\Finora`, and show `finora.db`.
4. Open `src/database/DatabaseManager.cpp` and explain `addTransaction` with H3.
5. Open `src/core/Transaction.cpp` and explain `isLeapYear` and `daysInMonth` with H2.

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

## Quick Revision

- **Finora:** offline Windows finance app in C++17, Qt 6 Widgets and SQLite; Milestone 2 of 7 is done.
- **Start-up:** theme, then `DatabaseManager::open`, then the window. A failure shows the path and reason, then exits with code 1.
- **Database file:** `%LOCALAPPDATA%\Finora\finora.db`; back up by copying it while Finora is closed.
- **Tables:** `currencies` (code primary key) and `transactions` (id primary key AUTOINCREMENT, currency_code foreign key).
- **Money:** `long long` minor units; 120050 = 1,200.50; per-transaction maximum 999,999,999,999; totals will need checked addition.
- **Validation:** `validateTransaction` in C++, plus CHECK and FOREIGN KEY rules in SQLite.
- **Dates:** YYYY-MM-DD, real calendar dates, leap years via `%`; `daysInMonth` uses `switch`.
- **Versioning:** `PRAGMA user_version`; 0 creates the tables, 1 is up to date, higher is refused.
- **Prepared statements:** placeholders plus `bindValue`, so there is no SQL injection.
- **RAII:** the destructor closes the database; braces end handle lifetimes before `removeDatabase`; copying is disabled.
- **Loops:** `for` for fixed counts, `while (query.next())` for database rows.
- **Tests:** 59 core, 22 interface and 62 database checks, all passing on 28 October 2026.
- **Not built yet:** the transaction, budget, goal, currency and report screens. Say "planned for Milestone N".
