# C++ Concepts Map

Where each C++ concept appears in Finora's real source code. Only concepts that exist in the code are listed in the first table. Concepts that are not used yet are listed separately, with the milestone where they will appear naturally.

Last updated: Milestone 2, 28 October 2026.

## Concepts used in the code

| C++ Concept | Where It Is Used | Example Function or File | Why It Is Used | How I Can Explain It |
|---|---|---|---|---|
| Variables | `int hour`, `int year`, `int month`, `int day`, `bool fileOpened` | `MainWindow::createPages`, `isValidIsoDate`, `DatabaseManager::open` | To hold values worked out while the program runs | "A variable is a named box in memory. `year` holds the four digits of the date as a number." |
| Constants | `PAGE_COUNT = 7`, `MAX_AMOUNT_MINOR`, `SUPPORTED_CURRENCY_COUNT = 3`, `CURRENT_SCHEMA_VERSION = 1`, `TYPE_INCOME` | `Sidebar.h`, `Transaction.h`, `DatabaseManager.h` | Fixed values get one name, so a typo cannot change them | "The maximum amount is written once, in `MAX_AMOUNT_MINOR`, and every check uses that name." |
| `int` | Hours, page numbers, dates, ids, counters | `greetingForHour(int hour)`, `Transaction::id` | Whole numbers | "A record id or a month is a whole number, so `int` fits." |
| `long long` | Money amounts in paisa or cents | `Transaction::amountMinor`, `MAX_AMOUNT_MINOR` | 64-bit whole numbers hold large amounts exactly | "1,200.50 rupees is stored as 120050 in a `long long`. An `int` holds at most 2,147,483,647, which is only about 21.4 million rupees in paisa." |
| `bool` | Results such as success or failure, the `opened` flag | `DatabaseManager::open`, `isLeapYear`, `DatabaseManager::opened` | A yes or no answer | "`open()` returns `false` when anything goes wrong." |
| `char` and character arithmetic | Checking each character of a date; turning a digit character into a number | `isValidIsoDate`, `digitsToNumber` | Dates arrive as text | "`'7' - '0'` is 7, because digit characters have consecutive codes." |
| `std::string` | Transaction fields, messages | `struct Transaction`, `validateTransaction` | Text in the plain C++ core | "The core uses the standard `string`, so it does not depend on Qt." |
| C-strings (`const char *`) | Fixed labels and currency data | `NavItem`, `PageInfo`, `CurrencyInfo` | Text built into the program | "`const char *` points to text that never changes." |
| `QString` | All text in the window and in SQL | `PageHeader`, `DatabaseManager` | Qt classes need Qt's string class | "I convert between `std::string` and `QString` where the core meets Qt." |
| Arithmetic operators | `*`, `/`, `+`, `-` | `setStartingSize`, `digitsToNumber` | Sizes and building numbers from digits | "`number * 10 + digit` adds one more digit to the number." |
| Remainder operator `%` | Leap-year rule | `isLeapYear` | To test divisibility | "`year % 4 == 0` means the year divides exactly by 4." |
| Relational operators | `<`, `>`, `<=`, `>=`, `==`, `!=` | `isValidIsoDate`, `validateTransaction`, `showPage` | To compare values | "`day > daysInMonth(year, month)` rejects 30 February." |
| Logical operators | `&&`, `\|\|`, `!` | `greetingForHour`, `validateTransaction`, `DatabaseManager::open` | To combine or reverse conditions | "`type != TYPE_INCOME && type != TYPE_EXPENSE` is true only for an unknown type." |
| Assignment and increment | `=`, `++` | Every loop, `TestCounter` | To store values and count | "`counter.checksRun++` adds one after each check." |
| Type conversion and casting | `static_cast<int>(text.length())`, `QString::fromStdString`, `toStdString`, `toLongLong`, `QString::number` | `isBlank`, `DatabaseManager::addTransaction`, `findTransactionById` | To move values between types | "`length()` returns an unsigned size; `static_cast<int>` turns it into an `int` once." |
| `if / else if / else` | Choosing one of several results | `greetingForHour` | To pick exactly one outcome | "Only the first true condition runs." |
| `switch` | Days in each month | `daysInMonth` | One variable compared with many fixed values | "`case 4: case 6: case 9: case 11:` all return 30." |
| Guard `if` with early `return` | Rejecting bad input before using it | `showPage`, `addTransaction`, `findTransactionById` | To stop before bad data is used | "If the database is not open, `addTransaction` returns at once with a message." |
| `for` loops | Buttons, pages, currencies, date characters, test cases | `createNavButtons`, `isSupportedCurrency`, `isValidIsoDate`, `insertSupportedCurrencies` | Repeat a known number of times | "The loop checks each of the 10 date characters." |
| `continue` | Skipping the two dash positions in a date | `isValidIsoDate` | To skip one loop step | "At positions 4 and 7 the loop continues to the next character." |
| `while` loop | Reading database rows one by one | `DatabaseManager::tableNames` | The number of rows is not known in advance | "`while (query.next())` runs once per row and stops after the last row." |
| Functions | Each task has its own function | `isLeapYear`, `daysInMonth`, `validateTransaction`, `showDatabaseError` | One job per function: easy to read and test | "`isLeapYear` only answers one question, so I can test it alone." |
| Parameters and return values | `daysInMonth(int year, int month)` returns `int` | `Transaction.cpp` | Data in, result out | "I pass 2024 and 2 and get back 29." |
| Pass by reference | `Transaction &transaction`, `Transaction &result`, `const std::string &text` | `addTransaction`, `findTransactionById`, `isValidIsoDate` | To fill the caller's variable, or to avoid copying | "`findTransactionById` fills the `result` structure that the caller gave it." |
| Default arguments | `QWidget *parent = nullptr`, `const QString &name = "finora"` | Interface constructors, `DatabaseManager` constructor | The caller may leave them out | "Finora uses the default name; each test passes its own name." |
| Arrays | `NAV_ITEMS`, `PAGE_INFO`, `SUPPORTED_CURRENCIES`, test case arrays | `Sidebar.cpp`, `MainWindow.cpp`, `Transaction.h`, tests | A fixed list in order | "`SUPPORTED_CURRENCIES[1]` is the US Dollar entry." |
| Structures | `Transaction`, `CurrencyInfo`, `NavItem`, `PageInfo`, `TestCounter`, `DateCase`, `InvalidCase` | `Transaction.h`, tests | Group related values into one item | "A `Transaction` groups type, amount, currency, category, description and date." |
| Classes and objects | `MainWindow`, `Sidebar`, `PageHeader`, `PlaceholderPage`, `DatabaseManager` | `src/ui/`, `src/database/` | Each part of the program is one class | "`DatabaseManager database;` in `main()` creates one database object." |
| Constructors and initializer lists | `DatabaseManager::DatabaseManager(const QString &name) : connectionName(name)` | Every class | Set up the object when it is created | "The initializer list stores the connection name before the constructor body runs." |
| Destructor | `DatabaseManager::~DatabaseManager()` calls `close()` | `DatabaseManager.cpp` | Clean up automatically when the object is destroyed | "When `database` goes out of scope, its destructor closes the file." |
| RAII and scope blocks | Connection closed by the destructor; lock file released by the destructor; `{ ... }` blocks end handle lifetimes; `QTemporaryDir` deletes test files | `DatabaseManager::open`, `close`, `InstanceLock`, `main.cpp`, `finora_db_tests.cpp` | Resources are released at a predictable moment | "The braces destroy the handle before the connection is removed, so Qt never sees it still in use." |
| Copying disabled with `= delete` | `DatabaseManager(const DatabaseManager &) = delete;` | `DatabaseManager.h` | A connection must have exactly one owner | "If copying were allowed, destroying the copy would close the original's database." |
| Static member function | `DatabaseManager::defaultDatabasePath()` | `DatabaseManager.cpp` | Works without an object | "I can ask for the path before creating a `DatabaseManager`." |
| Encapsulation | Private `connectionName`, `opened`, `errorMessage`; public `open`, `addTransaction`, `lastError` | `DatabaseManager.h`, `Sidebar.h` | Other code cannot break the object's state | "Nobody outside the class can set `opened` to true without really opening the file." |
| Member functions | `DatabaseManager::addTransaction`, `Sidebar::setCurrentPage` | `src/` | Behaviour that belongs to one class | "A member function works on its own object's data." |
| Inheritance from Qt classes | `class Sidebar : public QFrame` | `src/ui/*.h` | Qt widgets are built by extending Qt's classes | "I do not build my own class hierarchy." |
| Pointers, `new` and `nullptr` | `new QPushButton(...)`, `Sidebar *sidebar = nullptr` | Interface constructors | Qt widgets are owned by their parent | "The parent deletes its child widgets, so I never call `delete`." |
| Header files and include guards | `#ifndef FINORA_DATABASEMANAGER_H` | Every `.h` file | Declarations in `.h`, code in `.cpp`, no double inclusion | "The guard stops a header being included twice." |
| Modular programming | Libraries `finora_core` and `finora_database`; folders `core`, `database`, `ui`, `tests` | `CMakeLists.txt` | Small parts with one purpose | "The tests use the same libraries as the program." |
| File handling | Reading the style sheet with `QFile`; the SQLite database file; writing test files in `finora_db_tests` | `loadStyleSheet`, `DatabaseManager`, `writeFile` in tests | Data must survive closing the program | "Every transaction is stored in `finora.db` and read back after a restart." |
| Database operations (SQL) | `CREATE TABLE`, `INSERT`, `SELECT`, `PRAGMA` | `DatabaseManager.cpp` | Store and find records | "`SELECT ... WHERE id = :id` reads one record." |
| Prepared statements | `query.prepare(...)` then `bindValue(...)` | `addTransaction`, `findTransactionById`, `insertSupportedCurrencies` | Values are sent separately from the SQL text | "An apostrophe in a description can never change the command." |
| Input validation | Type, amount, currency, category, real calendar date | `validateTransaction`, `isValidIsoDate` | Stop bad data before it is saved | "29 February 2026 is rejected because 2026 is not a leap year." |
| Error handling | `bool` results plus `lastError()`; `fail()` helper; error dialog and exit code 1 | `DatabaseManager`, `main.cpp` | The caller always knows what went wrong | "`open()` returns false and `lastError()` says why." |
| Signals and slots | `connect(sidebar, &Sidebar::pageSelected, this, &MainWindow::showPage)` | `MainWindow` constructor | Qt calls a function when something happens | "When the sidebar emits `pageSelected`, Qt calls `showPage`." |
| Program exit codes | `return 1` after a database failure; tests return 0 or 1 | `main.cpp`, `tests/*.cpp` | Report success or failure to Windows and CTest | "Exit code 1 means Finora stopped because of a problem." |

## Concepts not used yet

| C++ Concept | Status | Where it will appear naturally |
|---|---|---|
| `double` | **Planned — Not Yet Implemented** | Percentages such as budget usage, Milestone 4. Money itself stays in whole numbers |
| `std::vector` | **Planned — Not Yet Implemented** | The list of transactions loaded from the database, Milestone 3 |
| `do-while` loop | **Planned — Not Yet Implemented** | No natural place found yet. Please confirm whether the syllabus needs it before an example is added |
| File writing with `std::ofstream` | **Planned — Not Yet Implemented** | CSV export, Milestone 6 |
| Updating and deleting records (SQL `UPDATE`, `DELETE`) | **Planned — Not Yet Implemented** | Editing and deleting transactions, Milestone 3 |
