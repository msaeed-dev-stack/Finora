// =====================================================================
//  finora_db_tests.cpp
//  Automated checks for DatabaseManager using REAL SQLite files.
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

#include "core/Transaction.h"
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
    check(counter, "schema version is 1", database.schemaVersion() == 1);
    check(counter, "tables are exactly currencies and transactions",
          tables.size() == 2 && tables.contains("currencies") && tables.contains("transactions"),
          tables.join(", "));
    check(counter, "PKR, USD and EUR are stored",
          queryNumber("test_first", "SELECT COUNT(*) FROM currencies") == 3);
    check(counter, "foreign keys are switched on",
          queryNumber("test_first", "PRAGMA foreign_keys") == 1);
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
    check(counter, "schema version is still 1", database.schemaVersion() == 1);
    check(counter, "still exactly 2 tables", database.tableNames().size() == 2);
    check(counter, "still exactly 3 currencies (no duplicates)",
          queryNumber("test_reopen", "SELECT COUNT(*) FROM currencies") == 3);

    bool openedTwice = database.open(path);
    check(counter, "open() on an already open database is refused", !openedTwice);
    showMessage(database.lastError());
    check(counter, "the open database is still usable afterwards",
          database.isOpen() && database.schemaVersion() == 1);
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

    std::cout << "---------------------\n";
    std::cout << counter.checksRun << " checks, " << counter.checksFailed << " failed\n";

    if (counter.checksFailed > 0) {
        return 1;
    }
    return 0;
}
