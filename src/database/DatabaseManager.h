// =====================================================================
//  DatabaseManager.h
//  Opens Finora's SQLite database, creates and upgrades its tables, and
//  saves and loads transactions and settings. The other store classes
//  in this folder (BudgetStore) use the connection this class opens.
//  All SQL in Finora lives in src/database, so the user interface never
//  has to know any SQL.
// =====================================================================
#ifndef FINORA_DATABASEMANAGER_H
#define FINORA_DATABASEMANAGER_H

#include <QString>
#include <QStringList>

#include <vector>

#include "core/Transaction.h"

// The newest table layout this version of Finora understands.
//   1  Milestone 2: currencies and transactions
//   2  Milestone 4: budgets, settings, the account tables used from
//      Phase 4B, and an owner, a unique id and timestamps for every transaction
const int CURRENT_SCHEMA_VERSION = 2;

// Owns one connection to one SQLite database file.
//
// Lifetime (RAII): open() creates the connection, and close() or the
// destructor removes it, so a connection is never left behind.
// The class never keeps QSqlDatabase or QSqlQuery objects as members.
// Each function creates the handles it needs as local variables, which
// are destroyed when the function ends, before close() removes the
// connection.
class DatabaseManager
{
public:
    // `name` identifies this object's connection inside Qt SQL. Two
    // DatabaseManager objects that are open at the same time need different names.
    DatabaseManager(const QString &name = "finora");
    ~DatabaseManager();

    // A connection must have exactly one owner, so copying is not allowed.
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    // The normal location: <AppLocalDataLocation>/finora.db, which for
    // Finora on Windows is C:/Users/<user>/AppData/Local/Finora/finora.db.
    // Returns an empty string if Windows reports no data folder.
    static QString defaultDatabasePath();

    // Reads the schema version of a database file WITHOUT changing it
    // (the file is opened read-only). Returns -1 if the file does not
    // exist or cannot be read. main() uses this to ask the user before
    // an older file is upgraded.
    static int storedSchemaVersion(const QString &databasePath);

    // Opens the database file, creating the file and its tables if needed.
    // An older file is upgraded: first a backup copy is saved and checked,
    // then every change runs inside one database transaction. If any step
    // fails, the transaction is rolled back and the file stays as it was.
    // Finora passes defaultDatabasePath(); the tests pass a temporary file.
    // Returns false and sets lastError() if anything goes wrong.
    bool open(const QString &databasePath);
    void close();

    bool isOpen() const;
    QString databasePath() const;
    QString lastError() const;

    // The backup made by the last upgrade, or empty if open() did not
    // need to upgrade anything.
    QString lastBackupPath() const;

    // The name of the open connection inside Qt SQL. The other store
    // classes of finora_database use it to run their own SQL.
    QString sqlConnectionName() const;

    // The schema version stored in the file, or -1 on error.
    int schemaVersion();

    // Names of Finora's own tables, sorted alphabetically.
    QStringList tableNames();

    // ---- Transactions ----

    // Saves a valid transaction. On success, transaction.id is set to the
    // new record's number. Invalid data is refused before any SQL runs.
    bool addTransaction(Transaction &transaction);

    // Loads the transaction with the given id into `result`.
    bool findTransactionById(int id, Transaction &result);

    // Loads every transaction into `results`, newest date first. When two
    // transactions have the same date, the one saved later comes first.
    bool loadTransactions(std::vector<Transaction> &results);

    // Saves new values for an existing transaction, found by transaction.id.
    // Invalid data is refused before any SQL runs.
    bool updateTransaction(const Transaction &transaction);

    // Deletes the transaction with the given id.
    bool deleteTransaction(int id);

    // ---- Settings of this Finora installation, such as the theme ----

    // The saved value of `key`, or `defaultValue` if it was never saved.
    QString setting(const QString &key, const QString &defaultValue);

    // Saves `value` under `key`, replacing an older value.
    bool saveSetting(const QString &key, const QString &value);

    // ---- History of important events ----

    // Adds one line to the audit_events table, for example
    // ("database_upgraded", "Schema version 1 to 2").
    bool recordAuditEvent(const QString &eventType, const QString &details);

private:
    bool enableForeignKeys();
    bool upgradeSchema();
    bool createVersion1Tables();
    bool insertSupportedCurrencies();
    bool backupBeforeUpgrade(int fromVersion);
    bool verifyBackup(const QString &backupFile, int expectedVersion, long long expectedTransactions);
    bool upgradeToVersion2();
    bool checkForeignKeys();
    bool readNumber(const QString &sql, long long &number);
    bool runStatement(const QString &sql);
    bool fail(const QString &message);

    QString connectionName;   // this object's connection name inside Qt SQL
    QString currentPath;      // the open file, or empty
    QString backupPath;       // the copy saved by the last upgrade, or empty
    QString errorMessage;     // explains the most recent failure
    bool opened = false;      // true only while this object owns an open connection
};

#endif // FINORA_DATABASEMANAGER_H
