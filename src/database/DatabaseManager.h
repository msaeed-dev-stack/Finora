// =====================================================================
//  DatabaseManager.h
//  Opens Finora's SQLite database, creates its tables, and saves and
//  loads transactions. All SQL in Finora lives in this class, so the
//  user interface never has to know any SQL.
// =====================================================================
#ifndef FINORA_DATABASEMANAGER_H
#define FINORA_DATABASEMANAGER_H

#include <QString>
#include <QStringList>

#include "core/Transaction.h"

// The newest table layout this version of Finora understands.
// Milestone 4 will raise it to 2 when it adds the budgets table.
const int CURRENT_SCHEMA_VERSION = 1;

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

    // Opens the database file, creating the file and its tables if needed.
    // Finora passes defaultDatabasePath(); the tests pass a temporary file.
    // Returns false and sets lastError() if anything goes wrong.
    bool open(const QString &databasePath);
    void close();

    bool isOpen() const;
    QString databasePath() const;
    QString lastError() const;

    // The schema version stored in the file, or -1 on error.
    int schemaVersion();

    // Names of Finora's own tables, sorted alphabetically.
    QStringList tableNames();

    // Saves a valid transaction. On success, transaction.id is set to the
    // new record's number. Invalid data is refused before any SQL runs.
    bool addTransaction(Transaction &transaction);

    // Loads the transaction with the given id into `result`.
    bool findTransactionById(int id, Transaction &result);

private:
    bool enableForeignKeys();
    bool upgradeSchema();
    bool createVersion1Tables();
    bool insertSupportedCurrencies();
    bool runStatement(const QString &sql);
    bool fail(const QString &message);

    QString connectionName;   // this object's connection name inside Qt SQL
    QString currentPath;      // the open file, or empty
    QString errorMessage;     // explains the most recent failure
    bool opened = false;      // true only while this object owns an open connection
};

#endif // FINORA_DATABASEMANAGER_H
