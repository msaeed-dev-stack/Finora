// =====================================================================
//  finora_db_tests.cpp
//  Automated checks for DatabaseManager and BudgetStore using REAL
//  SQLite files, including the upgrade from schema version 1 to 2.
//
//  Every database is created inside a new temporary folder that
//  QTemporaryDir deletes at the end, so the tests never touch the real
//  Finora database in AppData. Every check prints PASS or FAIL.
// =====================================================================
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QVariant>

#include <iostream>
#include <string>

#include "core/Budget.h"
#include "core/Transaction.h"
#include "database/BudgetStore.h"
#include "database/DatabaseManager.h"
#include "database/InstanceLock.h"

// Counts how many checks ran and how many of them failed.
struct TestCounter {
    int checksRun = 0;
    int checksFailed = 0;
};

// Records one check. When it fails, `detail` (for example the database's
// error message) is printed to help find the problem.
void check(TestCounter &counter, const std::string &testName, bool passed,
           const QString &detail = QString())
{
    counter.checksRun++;

    if (passed) {
        std::cout << "PASS  " << testName << "\n";
    } else {
        counter.checksFailed++;
        std::cout << "FAIL  " << testName << "\n";
        if (!detail.isEmpty()) {
            std::cout << "        detail: " << detail.toStdString() << "\n";
        }
    }
}

// Prints the message a refused operation returned, so the output shows
// that every error message is meaningful.
void showMessage(const QString &message)
{
    std::cout << "        message: " << message.toStdString() << "\n";
}

// ---- Helpers ---------------------------------------------------------

// Every table of schema version 2, in the order tableNames() returns them.
const QStringList VERSION_2_TABLES = {
    "app_settings", "audit_events", "budgets", "currencies", "login_sessions",
    "recovery_codes", "transactions", "user_settings", "users"
};

// A valid transaction that tests copy and then change.
Transaction makeSampleTransaction()
{
    Transaction transaction;
    transaction.type = TYPE_EXPENSE;
    transaction.amountMinor = 120050;   // 1,200.50
    transaction.currencyCode = "PKR";
    transaction.category = "Food";
    transaction.description = "Groceries";
    transaction.date = "2026-10-28";
    return transaction;
}

// True if every field of the two transactions is the same.
bool sameTransaction(const Transaction &a, const Transaction &b)
{
    return a.id == b.id && a.type == b.type && a.amountMinor == b.amountMinor
           && a.currencyCode == b.currencyCode && a.category == b.category
           && a.description == b.description && a.date == b.date;
}

// The helpers below run SQL directly on an open connection, bypassing
// DatabaseManager, to prove that SQLite's own rules also protect the data.
// Each QSqlQuery is a local variable, destroyed when the helper returns.
// Only fixed test values are written into this SQL text; real user data
// always goes through prepared statements in DatabaseManager.
bool runRawSql(const QString &connectionName, const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    return query.exec(sql);
}

// Returns the number in the first column of the first row, or -1 on error.
int queryNumber(const QString &connectionName, const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.exec(sql) || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

// Builds a raw INSERT for the transactions table from fixed test values.
QString rawInsert(const QString &type, const QString &amount,
                  const QString &currency, const QString &date)
{
    return "INSERT INTO transactions "
           "(type, amount_minor, currency_code, category, description, transaction_date) "
           "VALUES ('" + type + "', " + amount + ", '" + currency + "', 'Food', '', '" + date + "')";
}

// Reads user_version from a file through a separate, short-lived connection.
int readSchemaVersionDirectly(const QString &path)
{
    int version = -1;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "direct_check");
        db.setDatabaseName(path);
        if (db.open()) {
            QSqlQuery query(db);
            if (query.exec("PRAGMA user_version") && query.next()) {
                version = query.value(0).toInt();
            }
        }   // `query` is destroyed here
        db.close();
    }   // `db` is destroyed here
    QSqlDatabase::removeDatabase("direct_check");
    return version;
}

bool writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    bool written = (file.write(content) == content.size());
    file.close();
    return written;
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    QByteArray content = file.readAll();
    file.close();
    return content;
}

// ---- Tests -----------------------------------------------------------

void testDefaultLocation(TestCounter &counter)
{
    QString path = DatabaseManager::defaultDatabasePath();
    check(counter, "default path is ...\\AppData\\Local\\Finora\\finora.db",
          path.endsWith("/AppData/Local/Finora/finora.db"), path);
    std::cout << "        path: " << QDir::toNativeSeparators(path).toStdString() << "\n";
}

void testFirstOpen(TestCounter &counter, const QString &folder)
{
    QString path = folder + "/first.db";
    DatabaseManager database("test_first");

    bool opened = database.open(path);
    check(counter, "open() creates a new database", opened, database.lastError());
    if (!opened) {
        return;
    }

    QStringList tables = database.tableNames();
    check(counter, "the database file now exists", QFile::exists(path));
    check(counter, "schema version is 2", database.schemaVersion() == 2 && CURRENT_SCHEMA_VERSION == 2);
    check(counter, "tables are exactly the 9 tables of version 2",
          tables == VERSION_2_TABLES, tables.join(", "));
    check(counter, "PKR, USD and EUR are stored",
          queryNumber("test_first", "SELECT COUNT(*) FROM currencies") == 3);
    check(counter, "foreign keys are switched on",
          queryNumber("test_first", "PRAGMA foreign_keys") == 1);
    check(counter, "a brand-new database needs no backup",
          database.lastBackupPath().isEmpty() && !QFileInfo::exists(folder + "/backups"));
}

void testReopen(TestCounter &counter, const QString &folder)
{
    QString path = folder + "/reopen.db";
    DatabaseManager database("test_reopen");

    bool firstOpen = database.open(path);
    database.close();
    bool secondOpen = database.open(path);

    check(counter, "closing and opening the same file again works", firstOpen && secondOpen,
          database.lastError());
    check(counter, "schema version is still 2", database.schemaVersion() == 2);
    check(counter, "still exactly 9 tables", database.tableNames() == VERSION_2_TABLES);
    check(counter, "still exactly 3 currencies (no duplicates)",
          queryNumber("test_reopen", "SELECT COUNT(*) FROM currencies") == 3);
    check(counter, "reopening an up-to-date database makes no backup", database.lastBackupPath().isEmpty());

    bool openedTwice = database.open(path);
    check(counter, "open() on an already open database is refused", !openedTwice);
    showMessage(database.lastError());
    check(counter, "the open database is still usable afterwards",
          database.isOpen() && database.schemaVersion() == 2);
}

void testInsertAndRead(TestCounter &counter, const QString &folder)
{
    DatabaseManager database("test_insert");
    database.open(folder + "/insert.db");

    Transaction original = makeSampleTransaction();
    bool added = database.addTransaction(original);
    check(counter, "a valid transaction is saved", added, database.lastError());
    check(counter, "the new record gets an id above 0", original.id > 0);

    Transaction loaded;
    bool found = database.findTransactionById(original.id, loaded);
    check(counter, "the saved record can be read back", found, database.lastError());
    check(counter, "every field read back equals what was saved", sameTransaction(original, loaded));

    Transaction second = makeSampleTransaction();
    second.type = TYPE_INCOME;
    second.amountMinor = 500000;   // 5,000.00
    second.currencyCode = "USD";
    second.category = "Freelance";
    bool secondAdded = database.addTransaction(second);
    check(counter, "a second record gets a different id",
          secondAdded && second.id > 0 && second.id != original.id, database.lastError());
}

void testPunctuation(TestCounter &counter, const QString &folder)
{
    DatabaseManager database("test_punctuation");
    database.open(folder + "/punctuation.db");

    Transaction tricky = makeSampleTransaction();
    tricky.category = "Kids' toys";
    tricky.description = "Ali's café, 50% off; \"special\" -- DROP TABLE transactions; --";
    bool added = database.addTransaction(tricky);

    Transaction loaded;
    bool found = database.findTransactionById(tricky.id, loaded);
    check(counter, "apostrophes, quotes, % and ; are saved exactly",
          added && found && sameTransaction(tricky, loaded), database.lastError());
    check(counter, "the SQL-like text did not delete the table",
          database.tableNames().contains("transactions")
              && queryNumber("test_punctuation", "SELECT COUNT(*) FROM transactions") == 1);

    Transaction urdu = makeSampleTransaction();
    urdu.description = "چائے اور سموسے";
    bool urduAdded = database.addTransaction(urdu);
    Transaction urduLoaded;
    bool urduFound = database.findTransactionById(urdu.id, urduLoaded);
    check(counter, "Urdu text is saved and read back exactly",
          urduAdded && urduFound && sameTransaction(urdu, urduLoaded), database.lastError());
}

// One invalid transaction and a short description of what is wrong with it.
struct InvalidCase {
    std::string name;
    Transaction transaction;
};

void testInvalidInputs(TestCounter &counter, const QString &folder)
{
    DatabaseManager database("test_invalid");
    database.open(folder + "/invalid.db");

    const int CASE_COUNT = 10;
    InvalidCase cases[CASE_COUNT];
    for (int i = 0; i < CASE_COUNT; i++) {
        cases[i].transaction = makeSampleTransaction();
    }
    cases[0].name = "amount 0";                    cases[0].transaction.amountMinor = 0;
    cases[1].name = "a negative amount";           cases[1].transaction.amountMinor = -100;
    cases[2].name = "an amount above the maximum"; cases[2].transaction.amountMinor = MAX_AMOUNT_MINOR + 1;
    cases[3].name = "currency XYZ";                cases[3].transaction.currencyCode = "XYZ";
    cases[4].name = "lowercase currency pkr";      cases[4].transaction.currencyCode = "pkr";
    cases[5].name = "type 'transfer'";             cases[5].transaction.type = "transfer";
    cases[6].name = "an empty category";           cases[6].transaction.category = "";
    cases[7].name = "date 2026-02-29";             cases[7].transaction.date = "2026-02-29";
    cases[8].name = "date 2026-13-01";             cases[8].transaction.date = "2026-13-01";
    cases[9].name = "date written as 28/10/2026";  cases[9].transaction.date = "28/10/2026";

    for (int i = 0; i < CASE_COUNT; i++) {
        bool added = database.addTransaction(cases[i].transaction);
        check(counter, "rejects " + cases[i].name,
              !added && !database.lastError().isEmpty() && cases[i].transaction.id == 0);
        showMessage(database.lastError());
    }

    check(counter, "none of the rejected transactions was stored",
          queryNumber("test_invalid", "SELECT COUNT(*) FROM transactions") == 0);
}

void testErrorMessages(TestCounter &counter, const QString &folder)
{
    // 1. Using a database that was never opened.
    {
        DatabaseManager closedDatabase("test_closed");
        Transaction sample = makeSampleTransaction();
        Transaction result;
        check(counter, "adding to a database that is not open fails", !closedDatabase.addTransaction(sample));
        showMessage(closedDatabase.lastError());
        check(counter, "reading from a database that is not open fails",
              !closedDatabase.findTransactionById(1, result));
        check(counter, "schemaVersion() of a database that is not open is -1",
              closedDatabase.schemaVersion() == -1);
    }

    // 2. Asking for records that do not exist.
    {
        DatabaseManager database("test_missing");
        database.open(folder + "/missing.db");
        Transaction result;
        check(counter, "reading id 999 fails because it does not exist",
              !database.findTransactionById(999, result));
        showMessage(database.lastError());
        check(counter, "reading id 0 is refused", !database.findTransactionById(0, result));
        showMessage(database.lastError());
    }

    // 3. An empty file path.
    {
        DatabaseManager database("test_empty_path");
        check(counter, "an empty file path is refused", !database.open(""));
        showMessage(database.lastError());
    }

    // 4. Two objects that try to use the same connection name.
    {
        DatabaseManager first("test_duplicate");
        bool firstOpened = first.open(folder + "/duplicate.db");
        {
            DatabaseManager second("test_duplicate");
            check(counter, "a second connection with the same name is refused",
                  firstOpened && !second.open(folder + "/other.db"));
            showMessage(second.lastError());
        }   // `second` is destroyed here and must not close `first`'s connection

        Transaction sample = makeSampleTransaction();
        check(counter, "the first connection still works afterwards",
              first.isOpen() && first.addTransaction(sample), first.lastError());
    }
}

// Real failures when opening a database, all reliable on Windows.
void testOpenFailures(TestCounter &counter, const QString &folder)
{
    // A. A FILE already exists where the database's folder should be,
    //    so the folder cannot be created.
    QString blocker = folder + "/blocker";
    writeFile(blocker, "This file sits where a folder is needed.");
    {
        DatabaseManager database("test_blocked");
        check(counter, "a file in place of the folder makes open() fail",
              !database.open(blocker + "/finora.db"));
        showMessage(database.lastError());
    }
    check(counter, "no connection is left behind after that failure",
          !QSqlDatabase::contains("test_blocked"));

    // B. The path names a folder, not a file. Here SQLite itself fails
    //    to open it: a real database-open failure.
    QString folderPath = folder + "/a_folder";
    QDir().mkpath(folderPath);
    {
        DatabaseManager database("test_folder");
        bool opened = database.open(folderPath);
        check(counter, "opening a folder fails inside SQLite's open()",
              !opened && database.lastError().startsWith("Could not open the database file"),
              database.lastError());
        showMessage(database.lastError());
    }
    check(counter, "no connection is left behind after the SQLite failure",
          !QSqlDatabase::contains("test_folder"));

    // C. A file that is not a SQLite database at all.
    QString textFile = folder + "/notes.txt";
    QByteArray notes;
    for (int i = 0; i < 20; i++) {
        notes.append("These are shopping notes, not a database.\n");
    }
    writeFile(textFile, notes);
    {
        DatabaseManager database("test_not_database");
        check(counter, "a file that is not a database is refused", !database.open(textFile));
        showMessage(database.lastError());
    }
    check(counter, "the refused file was not changed", readFile(textFile) == notes);
    check(counter, "no connection is left behind after that failure",
          !QSqlDatabase::contains("test_not_database"));

    // D. A database from a newer Finora (schema version 99).
    QString newerPath = folder + "/newer.db";
    {
        DatabaseManager setup("test_newer_setup");
        setup.open(newerPath);
        runRawSql("test_newer_setup", "PRAGMA user_version = 99");
    }
    {
        DatabaseManager database("test_newer");
        check(counter, "a database from a newer Finora is refused", !database.open(newerPath));
        showMessage(database.lastError());
    }
    check(counter, "its schema version was left unchanged at 99",
          readSchemaVersionDirectly(newerPath) == 99);
}

void testPersistence(TestCounter &counter, const QString &folder)
{
    QString path = folder + "/persist.db";
    Transaction saved = makeSampleTransaction();
    saved.description = "Saved before closing";
    bool added = false;

    {
        DatabaseManager writer("test_persist_write");
        writer.open(path);
        added = writer.addTransaction(saved);
    }   // `writer` is destroyed here, and its destructor closes the database

    check(counter, "a record was saved and the first object was destroyed", added);
    check(counter, "the database file is still on disk", QFile::exists(path));

    DatabaseManager reader("test_persist_read");
    bool reopened = reader.open(path);
    check(counter, "a new DatabaseManager reopens the file", reopened, reader.lastError());

    Transaction loaded;
    bool found = reader.findTransactionById(saved.id, loaded);
    check(counter, "the record saved before closing is still there",
          found && sameTransaction(saved, loaded), reader.lastError());
    check(counter, "reopening did not duplicate the currencies",
          queryNumber("test_persist_read", "SELECT COUNT(*) FROM currencies") == 3);
}

// SQLite's own rules, tested with raw SQL that bypasses the C++ checks.
void testDatabaseRules(TestCounter &counter, const QString &folder)
{
    QString name = "test_rules";
    DatabaseManager database(name);
    database.open(folder + "/rules.db");

    check(counter, "control: a correct raw INSERT is accepted",
          runRawSql(name, rawInsert("expense", "500", "PKR", "2026-10-28")));
    check(counter, "SQLite refuses currency XYZ (foreign key)",
          !runRawSql(name, rawInsert("expense", "500", "XYZ", "2026-10-28")));
    check(counter, "SQLite refuses amount 0 (CHECK rule)",
          !runRawSql(name, rawInsert("expense", "0", "PKR", "2026-10-28")));
    check(counter, "SQLite refuses type 'gift' (CHECK rule)",
          !runRawSql(name, rawInsert("gift", "500", "PKR", "2026-10-28")));
    check(counter, "SQLite refuses date 2026-02-30 (CHECK rule)",
          !runRawSql(name, rawInsert("expense", "500", "PKR", "2026-02-30")));
    check(counter, "SQLite refuses deleting PKR while a transaction uses it (foreign key)",
          !runRawSql(name, "DELETE FROM currencies WHERE code = 'PKR'"));
    check(counter, "only the correct row was stored",
          queryNumber(name, "SELECT COUNT(*) FROM transactions") == 1);
}

// The single-instance lock that main() takes before opening the database.
// Two InstanceLock objects for the same file behave like two copies of
// Finora: the second one must be refused while the first holds the lock.
void testInstanceLock(TestCounter &counter, const QString &folder)
{
    check(counter, "the lock file sits next to the database",
          InstanceLock::lockPathFor(folder + "/data/finora.db") == folder + "/data/finora.lock",
          InstanceLock::lockPathFor(folder + "/data/finora.db"));
    check(counter, "no database path gives no lock path", InstanceLock::lockPathFor("").isEmpty());

    // The data folder does not exist yet, as on the very first start.
    QString lockPath = folder + "/lock_test/finora.lock";
    {
        InstanceLock first(lockPath);
        check(counter, "the first copy gets the lock", first.tryLock(), first.lastError());
        check(counter, "the lock file now exists", QFileInfo::exists(lockPath));
        check(counter, "asking again from the same copy still succeeds",
              first.tryLock() && first.isLocked(), first.lastError());

        InstanceLock second(lockPath);
        check(counter, "a second copy is refused while the first one runs", !second.tryLock());
        check(counter, "the second copy knows another copy holds the lock",
              second.heldByAnotherCopy() && !second.isLocked());
        showMessage(second.lastError());
        check(counter, "the first copy still holds the lock", first.isLocked());
    }   // `first` is destroyed here and releases the lock (RAII)

    check(counter, "the lock file is deleted when the first copy closes",
          !QFileInfo::exists(lockPath));
    {
        InstanceLock later(lockPath);
        check(counter, "a new copy gets the lock after the first one closed",
              later.tryLock(), later.lastError());
    }

    // A lock file left behind by a crash: it names a program that is no
    // longer running. 999999 is not a running program's number (Windows
    // numbers are always multiples of 4). Qt reads this file and sees
    // that the lock was abandoned.
    QString crashedPath = folder + "/crashed.lock";
    writeFile(crashedPath, "999999\nFinora\n" + QSysInfo::machineHostName().toUtf8() + "\n");
    {
        InstanceLock afterCrash(crashedPath);
        check(counter, "a lock left behind by a crash is taken over",
              afterCrash.tryLock(), afterCrash.lastError());
    }

    // A FILE sits where the data folder should be, so no lock file can
    // be created. This is a real error, not "already running".
    QString blocker = folder + "/lock_blocker";
    writeFile(blocker, "This file sits where a folder is needed.");
    {
        InstanceLock blocked(blocker + "/finora.lock");
        check(counter, "a lock file that cannot be created is refused", !blocked.tryLock());
        check(counter, "that failure is not reported as \"already running\"",
              !blocked.heldByAnotherCopy());
        showMessage(blocked.lastError());
    }
}

// Milestone 3: loading every transaction, updating one, deleting one.
void testLoadUpdateDelete(TestCounter &counter, const QString &folder)
{
    QString path = folder + "/manage.db";
    DatabaseManager database("test_manage");
    database.open(path);

    Transaction october1 = makeSampleTransaction();
    october1.date = "2026-10-01";
    october1.description = "First";
    Transaction october15a = makeSampleTransaction();
    october15a.date = "2026-10-15";
    october15a.description = "Second";
    Transaction october15b = makeSampleTransaction();
    october15b.date = "2026-10-15";
    october15b.description = "Third";
    bool added = database.addTransaction(october1) && database.addTransaction(october15a)
                 && database.addTransaction(october15b);
    check(counter, "three transactions are saved for the management tests", added, database.lastError());

    // Loading: newest date first; for the same date, the later id first.
    std::vector<Transaction> loaded;
    bool loadedOk = database.loadTransactions(loaded);
    check(counter, "loadTransactions reads all 3 rows", loadedOk && loaded.size() == 3, database.lastError());
    check(counter, "rows come newest first, then the later id first",
          loaded.size() == 3 && loaded[0].description == "Third" && loaded[1].description == "Second"
              && loaded[2].description == "First");

    // Updating a transaction.
    Transaction changed = october15a;
    changed.type = TYPE_INCOME;
    changed.amountMinor = 999950;
    changed.currencyCode = "USD";
    changed.category = "Freelance";
    changed.description = "Logo design for Sana's shop";
    changed.date = "2026-10-20";
    check(counter, "a valid update is saved", database.updateTransaction(changed), database.lastError());
    Transaction afterUpdate;
    database.findTransactionById(changed.id, afterUpdate);
    check(counter, "every changed field is read back", sameTransaction(changed, afterUpdate));

    Transaction invalid = changed;
    invalid.amountMinor = 0;
    check(counter, "an update with amount 0 is refused", !database.updateTransaction(invalid));
    showMessage(database.lastError());
    Transaction afterRefusal;
    database.findTransactionById(changed.id, afterRefusal);
    check(counter, "the refused update changed nothing", sameTransaction(changed, afterRefusal));

    Transaction missing = changed;
    missing.id = 999;
    check(counter, "updating id 999 fails because it does not exist", !database.updateTransaction(missing));
    showMessage(database.lastError());
    missing.id = 0;
    check(counter, "updating id 0 is refused", !database.updateTransaction(missing));

    // Deleting a transaction.
    check(counter, "deleting an existing transaction works",
          database.deleteTransaction(october1.id), database.lastError());
    Transaction gone;
    check(counter, "the deleted transaction cannot be found", !database.findTransactionById(october1.id, gone));
    check(counter, "deleting it again fails with a clear message", !database.deleteTransaction(october1.id));
    showMessage(database.lastError());
    check(counter, "deleting id 0 is refused", !database.deleteTransaction(0));

    // AUTOINCREMENT: deleting the newest row must not free its id for reuse.
    int highestId = october15b.id;
    database.deleteTransaction(highestId);
    Transaction next = makeSampleTransaction();
    database.addTransaction(next);
    check(counter, "a new transaction never reuses a deleted id", next.id > highestId,
          "new id " + QString::number(next.id) + ", deleted id " + QString::number(highestId));

    // The changes are kept after closing and reopening the file.
    database.close();
    DatabaseManager reopened("test_manage_reopen");
    reopened.open(path);
    std::vector<Transaction> afterRestart;
    reopened.loadTransactions(afterRestart);
    Transaction updatedAfterRestart;
    bool updateKept = reopened.findTransactionById(changed.id, updatedAfterRestart)
                      && sameTransaction(changed, updatedAfterRestart);
    check(counter, "after reopening: 2 rows remain and the update is kept",
          afterRestart.size() == 2 && updateKept);

    // Every operation fails clearly on a database that is not open.
    DatabaseManager closedDatabase("test_manage_closed");
    std::vector<Transaction> none;
    check(counter, "loading from a closed database fails", !closedDatabase.loadTransactions(none));
    check(counter, "updating in a closed database fails", !closedDatabase.updateTransaction(changed));
    check(counter, "deleting in a closed database fails", !closedDatabase.deleteTransaction(1));
    showMessage(closedDatabase.lastError());
}

// =====================================================================
//  Milestone 4: schema version 2, the safe upgrade, settings and budgets
// =====================================================================

// Runs SQL statements on a file through a short-lived connection of its
// own. Foreign keys are OFF on a new connection, so a test can also
// store a broken row on purpose.
bool runSqlOnFile(const QString &path, const QStringList &statements)
{
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "file_setup");
        db.setDatabaseName(path);
        ok = db.open();
        if (ok) {
            QSqlQuery query(db);
            for (int i = 0; i < statements.size() && ok; i++) {
                ok = query.exec(statements[i]);
            }
        }   // `query` is destroyed here
        db.close();
    }
    QSqlDatabase::removeDatabase("file_setup");
    return ok;
}

// The number in the first column of the first row, read from a file
// through its own read-only connection, or -1 on error.
long long numberFromFile(const QString &path, const QString &sql)
{
    long long number = -1;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "file_check");
        db.setDatabaseName(path);
        db.setConnectOptions("QSQLITE_OPEN_READONLY");
        if (db.open()) {
            QSqlQuery query(db);
            if (query.exec(sql) && query.next()) {
                number = query.value(0).toLongLong();
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("file_check");
    return number;
}

// The text in the first column of the first row, or "" on error.
QString queryText(const QString &connectionName, const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.exec(sql) || !query.next()) {
        return QString();
    }
    return query.value(0).toString();
}

// Writes a database file exactly as Milestones 2 and 3 created it
// (schema version 1, the same CREATE TABLE statements), with four
// transactions, one of which is deleted again. Used to test the upgrade
// without needing an old copy of Finora.
bool createVersion1File(const QString &path)
{
    return runSqlOnFile(path, {
        "CREATE TABLE currencies ("
        "  code TEXT PRIMARY KEY CHECK (length(code) = 3), name TEXT NOT NULL,"
        "  symbol TEXT NOT NULL, decimal_places INTEGER NOT NULL CHECK (decimal_places BETWEEN 0 AND 3))",
        "CREATE TABLE transactions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  type TEXT NOT NULL CHECK (type IN ('income', 'expense')),"
        "  amount_minor INTEGER NOT NULL CHECK (amount_minor > 0 AND amount_minor <= 999999999999),"
        "  currency_code TEXT NOT NULL REFERENCES currencies (code),"
        "  category TEXT NOT NULL CHECK (length(trim(category)) > 0),"
        "  description TEXT NOT NULL DEFAULT '',"
        "  transaction_date TEXT NOT NULL CHECK (transaction_date IS date(transaction_date)))",
        "INSERT INTO currencies VALUES ('PKR', 'Pakistani Rupee', 'Rs', 2), "
        "('USD', 'US Dollar', '$', 2), ('EUR', 'Euro', 'E', 2)",
        "INSERT INTO transactions (type, amount_minor, currency_code, category, description, transaction_date) "
        "VALUES ('income', 12000000, 'PKR', 'Salary', 'October salary', '2026-10-01')",
        "INSERT INTO transactions (type, amount_minor, currency_code, category, description, transaction_date) "
        "VALUES ('expense', 120050, 'PKR', 'Food', 'Groceries from Ali''s shop', '2026-10-28')",
        "INSERT INTO transactions (type, amount_minor, currency_code, category, description, transaction_date) "
        "VALUES ('expense', 4599, 'USD', 'Shopping', 'Book', '2026-09-15')",
        "INSERT INTO transactions (type, amount_minor, currency_code, category, description, transaction_date) "
        "VALUES ('expense', 999, 'EUR', 'Food', 'Deleted later', '2026-10-02')",
        "DELETE FROM transactions WHERE id = 4",
        "PRAGMA user_version = 1"
    });
}

int countFilesIn(const QString &folder)
{
    return QDir(folder).entryList(QDir::Files).size();
}

void testStoredSchemaVersion(TestCounter &counter, const QString &folder)
{
    QString missing = folder + "/does_not_exist.db";
    check(counter, "storedSchemaVersion of a missing file is -1",
          DatabaseManager::storedSchemaVersion(missing) == -1);
    check(counter, "checking a missing file does not create it", !QFile::exists(missing));

    QString path = folder + "/version_check.db";
    {
        DatabaseManager database("test_version_check");
        database.open(path);
    }   // closed by the destructor
    check(counter, "storedSchemaVersion reads a new file as version 2",
          DatabaseManager::storedSchemaVersion(path) == 2);
}

void testUpgradeFromVersion1(TestCounter &counter, const QString &parentFolder)
{
    // A folder of its own, so the backups folder holds only this test's backup.
    QString folder = parentFolder + "/upgrade_ok";
    QDir().mkpath(folder);
    QString path = folder + "/finora.db";

    bool created = createVersion1File(path);
    check(counter, "a Milestone 3 (version 1) file is prepared", created);
    if (!created) {
        return;
    }

    QByteArray bytesBefore = readFile(path);
    check(counter, "storedSchemaVersion sees version 1 before the upgrade",
          DatabaseManager::storedSchemaVersion(path) == 1);
    check(counter, "reading the version did not change a single byte", readFile(path) == bytesBefore);

    DatabaseManager database("test_upgrade");
    bool opened = database.open(path);
    check(counter, "open() upgrades the version 1 file", opened, database.lastError());
    if (!opened) {
        return;
    }

    check(counter, "after the upgrade the schema version is 2", database.schemaVersion() == 2);
    check(counter, "the upgraded file has the 9 tables of version 2",
          database.tableNames() == VERSION_2_TABLES, database.tableNames().join(", "));

    std::vector<Transaction> rows;
    database.loadTransactions(rows);
    bool sameValues = rows.size() == 3
                      && rows[0].id == 2 && rows[0].amountMinor == 120050
                      && rows[0].description == "Groceries from Ali's shop" && rows[0].date == "2026-10-28"
                      && rows[1].id == 1 && rows[1].type == TYPE_INCOME && rows[1].amountMinor == 12000000
                      && rows[1].category == "Salary"
                      && rows[2].id == 3 && rows[2].currencyCode == "USD" && rows[2].amountMinor == 4599;
    check(counter, "all 3 transactions kept their ids and values", sameValues);
    check(counter, "every upgraded row got a different 32-character id",
          queryNumber("test_upgrade", "SELECT COUNT(DISTINCT uuid) FROM transactions WHERE length(uuid) = 32") == 3);
    check(counter, "upgraded rows belong to no account yet (user_id is NULL)",
          queryNumber("test_upgrade", "SELECT COUNT(*) FROM transactions WHERE user_id IS NULL") == 3);
    check(counter, "upgraded rows have a created and an updated time",
          queryNumber("test_upgrade", "SELECT COUNT(*) FROM transactions "
                                      "WHERE length(created_at) = 20 AND updated_at = created_at") == 3);

    Transaction added = makeSampleTransaction();
    database.addTransaction(added);
    check(counter, "the next transaction gets id 5: the deleted id 4 is never given out again", added.id == 5);

    check(counter, "the foreign key check finds no broken link",
          queryNumber("test_upgrade", "SELECT COUNT(*) FROM pragma_foreign_key_check") == 0);
    check(counter, "SQLite's integrity check says ok", queryText("test_upgrade", "PRAGMA integrity_check") == "ok");
    check(counter, "the upgrade is written to the audit history",
          queryNumber("test_upgrade", "SELECT COUNT(*) FROM audit_events WHERE event_type = 'database_upgraded'") == 1);

    // The backup: made before anything changed, so it is the old file.
    QString backup = database.lastBackupPath();
    check(counter, "a backup was saved in the backups folder next to the database",
          !backup.isEmpty() && QFile::exists(backup) && QFileInfo(backup).absolutePath() == folder + "/backups",
          backup);
    check(counter, "the backup is the untouched version 1 file with its 3 transactions",
          DatabaseManager::storedSchemaVersion(backup) == 1
              && numberFromFile(backup, "SELECT COUNT(*) FROM transactions") == 3
              && numberFromFile(backup, "SELECT COUNT(*) FROM pragma_table_info('transactions') "
                                        "WHERE name = 'uuid'") == 0);
    std::cout << "        backup: " << QFileInfo(backup).fileName().toStdString() << "\n";

    database.close();
    bool reopened = database.open(path);
    check(counter, "opening the upgraded file again makes no second backup",
          reopened && database.lastBackupPath().isEmpty() && countFilesIn(folder + "/backups") == 1);
}

// A version 1 file whose transactions were ALL deleted still remembers
// the highest id it gave out (4). After the upgrade the next id must be
// 5, not 1, even though no row was copied.
void testUpgradeKeepsIdsAfterDeletingEverything(TestCounter &counter, const QString &parentFolder)
{
    QString folder = parentFolder + "/upgrade_all_deleted";
    QDir().mkpath(folder);
    QString path = folder + "/finora.db";
    bool prepared = createVersion1File(path) && runSqlOnFile(path, {"DELETE FROM transactions"});
    check(counter, "a version 1 file with every transaction deleted is prepared",
          prepared && numberFromFile(path, "SELECT COUNT(*) FROM transactions") == 0);

    DatabaseManager database("test_upgrade_empty");
    bool opened = database.open(path);
    check(counter, "the file without transactions is upgraded", opened, database.lastError());

    Transaction added = makeSampleTransaction();
    database.addTransaction(added);
    check(counter, "the next id is 5: deleted ids are not given out again after the upgrade", added.id == 5);
}

void testUpgradeRollback(TestCounter &counter, const QString &parentFolder)
{
    // ---- A. A transaction that points to a currency that does not exist.
    //         Milestone 3 never allowed this; it is stored here on purpose
    //         (foreign keys are off on the setup connection) to make the
    //         copy step of the upgrade fail.
    QString folderA = parentFolder + "/upgrade_broken_link";
    QDir().mkpath(folderA);
    QString pathA = folderA + "/finora.db";
    bool preparedA = createVersion1File(pathA)
                     && runSqlOnFile(pathA, {"INSERT INTO transactions (type, amount_minor, currency_code, "
                                             "category, description, transaction_date) "
                                             "VALUES ('expense', 500, 'GBP', 'Food', 'Bad link', '2026-10-05')"});
    check(counter, "a version 1 file with a broken currency link is prepared", preparedA);

    QByteArray bytesBeforeA = readFile(pathA);
    {
        DatabaseManager database("test_rollback_a");
        bool opened = database.open(pathA);
        check(counter, "the upgrade of a file with a broken link is refused", !opened);
        showMessage(database.lastError());
        check(counter, "the message says nothing was changed and where the backup is",
              database.lastError().contains("nothing in it was changed")
                  && database.lastError().contains("backup"));
        check(counter, "the refused database is not left open", !database.isOpen());
        check(counter, "the backup copy from before the attempt is kept",
              !database.lastBackupPath().isEmpty() && QFile::exists(database.lastBackupPath()));
    }
    check(counter, "after the rollback the file is still version 1",
          DatabaseManager::storedSchemaVersion(pathA) == 1);
    check(counter, "after the rollback the file is byte-for-byte as before", readFile(pathA) == bytesBeforeA);

    // ---- B. A table named "budgets" already exists (made by another
    //         tool). The upgrade fails late, AFTER the transactions table
    //         was rebuilt, so the rollback must undo the rebuild too.
    QString folderB = parentFolder + "/upgrade_conflict";
    QDir().mkpath(folderB);
    QString pathB = folderB + "/finora.db";
    bool preparedB = createVersion1File(pathB)
                     && runSqlOnFile(pathB, {"CREATE TABLE budgets (note TEXT)",
                                             "INSERT INTO budgets VALUES ('made by another tool')"});
    check(counter, "a version 1 file with an unexpected budgets table is prepared", preparedB);

    QByteArray bytesBeforeB = readFile(pathB);
    {
        DatabaseManager database("test_rollback_b");
        check(counter, "the upgrade that meets an existing budgets table is refused", !database.open(pathB));
        showMessage(database.lastError());
    }
    check(counter, "the file is byte-for-byte as before: the rebuilt table was rolled back",
          readFile(pathB) == bytesBeforeB);
    check(counter, "the original transactions table is back, without the new columns",
          numberFromFile(pathB, "SELECT COUNT(*) FROM pragma_table_info('transactions') WHERE name = 'uuid'") == 0
              && numberFromFile(pathB, "SELECT COUNT(*) FROM transactions") == 3);
    check(counter, "no half-made version 2 table is left behind",
          numberFromFile(pathB, "SELECT COUNT(*) FROM sqlite_master "
                                "WHERE name IN ('transactions_v2', 'users', 'app_settings')") == 0);
    check(counter, "the other tool's budgets table is untouched",
          numberFromFile(pathB, "SELECT COUNT(*) FROM budgets WHERE note = 'made by another tool'") == 1);
}

void testSettings(TestCounter &counter, const QString &folder)
{
    QString path = folder + "/settings.db";
    DatabaseManager database("test_settings");
    database.open(path);

    check(counter, "a setting that was never saved gives the default",
          database.setting("theme", "light") == "light" && database.lastError().isEmpty());
    check(counter, "a saved setting can be read back",
          database.saveSetting("theme", "dark") && database.setting("theme", "light") == "dark");
    check(counter, "saving again replaces the value instead of adding a row",
          database.saveSetting("theme", "light") && database.setting("theme", "dark") == "light"
              && queryNumber("test_settings", "SELECT COUNT(*) FROM app_settings WHERE key = 'theme'") == 1);
    check(counter, "an apostrophe in a setting is stored exactly",
          database.saveSetting("note", "Ali's") && database.setting("note", "") == "Ali's");

    database.saveSetting("reduced_motion", "1");
    database.close();
    database.open(path);
    check(counter, "settings are kept after closing and reopening",
          database.setting("reduced_motion", "0") == "1" && database.setting("theme", "dark") == "light");

    DatabaseManager closedDatabase("test_settings_closed");
    check(counter, "a closed database gives the default and an error message",
          closedDatabase.setting("theme", "light") == "light" && !closedDatabase.lastError().isEmpty());
    check(counter, "saving a setting into a closed database fails", !closedDatabase.saveSetting("theme", "dark"));
}

void testBudgetStore(TestCounter &counter, const QString &folder)
{
    QString path = folder + "/budgets.db";
    DatabaseManager database("test_budgets");
    database.open(path);
    BudgetStore store(database);

    Budget food;
    food.category = "Food";
    food.currencyCode = "PKR";
    food.limitMinor = 2000000;   // PKR 20,000.00
    check(counter, "a valid budget is saved and gets an id", store.addBudget(food) && food.id > 0,
          store.lastError());

    Budget foodInDollars = food;
    foodInDollars.id = 0;
    foodInDollars.currencyCode = "USD";
    foodInDollars.limitMinor = 20000;   // USD 200.00
    check(counter, "the same category in another currency is allowed", store.addBudget(foodInDollars),
          store.lastError());

    Budget duplicate;
    duplicate.category = "food";
    duplicate.currencyCode = "PKR";
    duplicate.limitMinor = 100;
    check(counter, "a second Food budget in PKR is refused, also written in lower case",
          !store.addBudget(duplicate) && store.lastError().contains("already"));
    showMessage(store.lastError());

    Budget invalid = duplicate;
    invalid.category = "Rent";
    invalid.limitMinor = 0;
    bool zeroRefused = !store.addBudget(invalid);
    invalid.limitMinor = MAX_AMOUNT_MINOR + 1;
    bool hugeRefused = !store.addBudget(invalid);
    invalid.limitMinor = 100;
    invalid.currencyCode = "GBP";
    bool currencyRefused = !store.addBudget(invalid);
    invalid.currencyCode = "PKR";
    invalid.category = "   ";
    bool blankRefused = !store.addBudget(invalid);
    check(counter, "a limit of 0, a huge limit, GBP and a blank category are all refused",
          zeroRefused && hugeRefused && currencyRefused && blankRefused);

    std::vector<Budget> loaded;
    store.loadBudgets(loaded);
    check(counter, "loading returns the 2 budgets, PKR before USD",
          loaded.size() == 2 && loaded[0].currencyCode == "PKR" && loaded[1].currencyCode == "USD");
    check(counter, "the loaded values are exactly the saved ones",
          loaded.size() == 2 && loaded[0].id == food.id && loaded[0].category == "Food"
              && loaded[0].limitMinor == 2000000 && loaded[1].limitMinor == 20000);

    food.limitMinor = 2500000;
    store.updateBudget(food);
    store.loadBudgets(loaded);
    check(counter, "updating a budget changes its limit", loaded.size() == 2 && loaded[0].limitMinor == 2500000);

    Budget changedIntoDuplicate = foodInDollars;
    changedIntoDuplicate.currencyCode = "PKR";
    check(counter, "changing a budget into a duplicate is refused", !store.updateBudget(changedIntoDuplicate));

    Budget missing = food;
    missing.id = 9999;
    check(counter, "updating a budget that does not exist fails", !store.updateBudget(missing));

    // Deleting a budget must never delete the transactions of its category.
    Transaction groceries = makeSampleTransaction();
    database.addTransaction(groceries);
    check(counter, "deleting a budget removes only that budget",
          store.deleteBudget(foodInDollars.id) && store.loadBudgets(loaded) && loaded.size() == 1
              && queryNumber("test_budgets", "SELECT COUNT(*) FROM transactions") == 1);
    check(counter, "deleting a budget that does not exist fails", !store.deleteBudget(9999));

    // SQLite's own rules, tested by bypassing BudgetStore.
    check(counter, "SQLite refuses a limit of 0 even without BudgetStore",
          !runRawSql("test_budgets", "INSERT INTO budgets (category, currency_code, limit_minor) "
                                     "VALUES ('Rent', 'PKR', 0)"));
    check(counter, "SQLite refuses a budget in an unknown currency (foreign key)",
          !runRawSql("test_budgets", "INSERT INTO budgets (category, currency_code, limit_minor) "
                                     "VALUES ('Rent', 'GBP', 100)"));
    check(counter, "SQLite's unique index refuses FOOD in PKR next to Food in PKR",
          !runRawSql("test_budgets", "INSERT INTO budgets (category, currency_code, limit_minor) "
                                     "VALUES ('FOOD', 'PKR', 100)"));

    database.close();
    database.open(path);
    store.loadBudgets(loaded);
    check(counter, "budgets are kept after closing and reopening",
          loaded.size() == 1 && loaded[0].limitMinor == 2500000);

    DatabaseManager closedDatabase("test_budgets_closed");
    BudgetStore closedStore(closedDatabase);
    check(counter, "a budget store on a closed database refuses to work",
          !closedStore.addBudget(food) && !closedStore.loadBudgets(loaded));
}

void testTransactionTimestamps(TestCounter &counter, const QString &folder)
{
    DatabaseManager database("test_times");
    database.open(folder + "/times.db");

    Transaction first = makeSampleTransaction();
    Transaction second = makeSampleTransaction();
    database.addTransaction(first);
    database.addTransaction(second);
    check(counter, "a new transaction gets a 32-character id and a created time",
          queryNumber("test_times", "SELECT COUNT(*) FROM transactions WHERE id = " + QString::number(first.id)
                                    + " AND length(uuid) = 32 AND length(created_at) = 20") == 1);
    check(counter, "two transactions never share an id",
          queryNumber("test_times", "SELECT COUNT(DISTINCT uuid) FROM transactions") == 2);

    runRawSql("test_times", "UPDATE transactions SET created_at = '2000-01-01T00:00:00Z', "
                            "updated_at = '2000-01-01T00:00:00Z' WHERE id = " + QString::number(first.id));
    first.amountMinor = 5000;
    database.updateTransaction(first);
    check(counter, "editing refreshes updated_at but keeps created_at",
          queryNumber("test_times", "SELECT COUNT(*) FROM transactions WHERE id = " + QString::number(first.id)
                                    + " AND updated_at > '2000-01-01T00:00:00Z'"
                                      " AND created_at = '2000-01-01T00:00:00Z'") == 1);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("Finora");   // so defaultDatabasePath() matches the real program

    TestCounter counter;
    std::cout << "Finora database tests\n";
    std::cout << "---------------------\n";

    // A new, empty folder. QTemporaryDir deletes it and everything in it
    // when this object is destroyed at the end of main() (RAII).
    QTemporaryDir temporaryFolder;
    if (!temporaryFolder.isValid()) {
        std::cout << "Could not create a temporary folder.\n";
        return 1;
    }
    QString folder = temporaryFolder.path();
    std::cout << "Temporary folder: " << QDir::toNativeSeparators(folder).toStdString() << "\n";

    check(counter, "the SQLite driver can be loaded", QSqlDatabase::isDriverAvailable("QSQLITE"));
    testDefaultLocation(counter);
    testFirstOpen(counter, folder);
    testReopen(counter, folder);
    testInsertAndRead(counter, folder);
    testPunctuation(counter, folder);
    testInvalidInputs(counter, folder);
    testErrorMessages(counter, folder);
    testOpenFailures(counter, folder);
    testPersistence(counter, folder);
    testDatabaseRules(counter, folder);
    testInstanceLock(counter, folder);
    testLoadUpdateDelete(counter, folder);

    std::cout << "---- Milestone 4: version 2, upgrade, settings, budgets ----\n";
    testStoredSchemaVersion(counter, folder);
    testUpgradeFromVersion1(counter, folder);
    testUpgradeKeepsIdsAfterDeletingEverything(counter, folder);
    testUpgradeRollback(counter, folder);
    testSettings(counter, folder);
    testBudgetStore(counter, folder);
    testTransactionTimestamps(counter, folder);

    std::cout << "---------------------\n";
    std::cout << counter.checksRun << " checks, " << counter.checksFailed << " failed\n";

    if (counter.checksFailed > 0) {
        return 1;
    }
    return 0;
}
