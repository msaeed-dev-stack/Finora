// =====================================================================
//  DatabaseManager.cpp
// =====================================================================
#include "DatabaseManager.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

DatabaseManager::DatabaseManager(const QString &name)
    : connectionName(name)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

QString DatabaseManager::defaultDatabasePath()
{
    QString folder = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (folder.isEmpty()) {
        return QString();
    }
    return folder + "/finora.db";
}

bool DatabaseManager::open(const QString &databasePath)
{
    errorMessage.clear();

    if (opened) {
        return fail("This database is already open.");
    }
    if (databasePath.isEmpty()) {
        return fail("No database file was given.");
    }
    if (QSqlDatabase::contains(connectionName)) {
        return fail("Another database connection named \"" + connectionName + "\" is already open.");
    }
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        return fail("The SQLite driver (sqldrivers/qsqlite.dll) could not be loaded.");
    }

    // SQLite can create the file, but the folder around it must exist first.
    QString folder = QFileInfo(databasePath).absolutePath();
    if (!QDir().mkpath(folder)) {
        return fail("Could not create the folder " + QDir::toNativeSeparators(folder) + ".");
    }

    // The braces limit the life of the handle `db`. It is destroyed at the
    // closing brace, so removeDatabase() below never finds it still in use.
    bool fileOpened = false;
    QString reason;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(databasePath);
        fileOpened = db.open();
        if (!fileOpened) {
            reason = db.lastError().text();
        }
    }

    if (!fileOpened) {
        QSqlDatabase::removeDatabase(connectionName);
        return fail("Could not open the database file: " + reason);
    }

    opened = true;
    currentPath = databasePath;

    // Prepare the connection and the tables. If a step fails, the
    // connection is closed again; the error message is kept for the caller.
    if (!enableForeignKeys() || !upgradeSchema()) {
        close();
        return false;
    }
    return true;
}

void DatabaseManager::close()
{
    // Only remove a connection that this object opened. Another object may
    // own a connection with the same name, and it must not be disturbed.
    if (!opened) {
        return;
    }

    {
        QSqlDatabase db = QSqlDatabase::database(connectionName, false);
        db.close();
    }   // the handle is destroyed here, before the connection is removed

    QSqlDatabase::removeDatabase(connectionName);
    opened = false;
    currentPath.clear();
}

bool DatabaseManager::isOpen() const
{
    return opened;
}

QString DatabaseManager::databasePath() const
{
    return currentPath;
}

QString DatabaseManager::lastError() const
{
    return errorMessage;
}

int DatabaseManager::schemaVersion()
{
    if (!opened) {
        fail("The database is not open.");
        return -1;
    }

    // user_version is a whole number that SQLite keeps inside the file
    // for the program's own use. Finora uses it as the schema version.
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.exec("PRAGMA user_version") || !query.next()) {
        fail("Could not read the schema version: " + query.lastError().text());
        return -1;
    }
    return query.value(0).toInt();
}

QStringList DatabaseManager::tableNames()
{
    QStringList names;
    if (!opened) {
        fail("The database is not open.");
        return names;
    }

    // sqlite_master lists every table. Tables whose names start with
    // "sqlite_" belong to SQLite itself and are skipped.
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.exec("SELECT name FROM sqlite_master "
                    "WHERE type = 'table' AND name NOT LIKE 'sqlite_%' ORDER BY name")) {
        fail("Could not list the tables: " + query.lastError().text());
        return names;
    }

    // next() moves to the following row and returns false after the last row.
    while (query.next()) {
        names.append(query.value(0).toString());
    }
    return names;
}

bool DatabaseManager::addTransaction(Transaction &transaction)
{
    errorMessage.clear();
    if (!opened) {
        return fail("The database is not open.");
    }

    // Check the data in C++ first, so the user gets a clear message.
    std::string problem = validateTransaction(transaction);
    if (!problem.empty()) {
        return fail(QString::fromStdString(problem));
    }

    // A prepared statement: the SQL text contains only placeholders such
    // as :description, and the values are sent separately. A description
    // like  Ali's shop  therefore can never change the SQL command.
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    bool prepared = query.prepare(
        "INSERT INTO transactions "
        "(type, amount_minor, currency_code, category, description, transaction_date) "
        "VALUES (:type, :amount, :currency, :category, :description, :date)");
    if (!prepared) {
        return fail("Could not prepare to save the transaction: " + query.lastError().text());
    }

    query.bindValue(":type", QString::fromStdString(transaction.type));
    query.bindValue(":amount", transaction.amountMinor);
    query.bindValue(":currency", QString::fromStdString(transaction.currencyCode));
    query.bindValue(":category", QString::fromStdString(transaction.category));
    query.bindValue(":description", QString::fromStdString(transaction.description));
    query.bindValue(":date", QString::fromStdString(transaction.date));

    if (!query.exec()) {
        return fail("Could not save the transaction: " + query.lastError().text());
    }

    transaction.id = query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::findTransactionById(int id, Transaction &result)
{
    errorMessage.clear();
    if (!opened) {
        return fail("The database is not open.");
    }
    if (id <= 0) {
        return fail("A transaction id must be a number above 0.");
    }

    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    bool prepared = query.prepare(
        "SELECT id, type, amount_minor, currency_code, category, description, transaction_date "
        "FROM transactions WHERE id = :id");
    if (!prepared) {
        return fail("Could not prepare to read the transaction: " + query.lastError().text());
    }
    query.bindValue(":id", id);

    if (!query.exec()) {
        return fail("Could not read the transaction: " + query.lastError().text());
    }
    if (!query.next()) {
        return fail("No transaction with id " + QString::number(id) + " was found.");
    }

    // The column numbers follow the order of the SELECT list above.
    result.id = query.value(0).toInt();
    result.type = query.value(1).toString().toStdString();
    result.amountMinor = query.value(2).toLongLong();
    result.currencyCode = query.value(3).toString().toStdString();
    result.category = query.value(4).toString().toStdString();
    result.description = query.value(5).toString().toStdString();
    result.date = query.value(6).toString().toStdString();
    return true;
}

// SQLite checks FOREIGN KEY rules only when this setting is on, and every
// new connection starts with it off, so it is switched on at every open.
bool DatabaseManager::enableForeignKeys()
{
    if (!runStatement("PRAGMA foreign_keys = ON")) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.exec("PRAGMA foreign_keys")) {
        return fail("Could not check the foreign key setting: " + query.lastError().text());
    }
    if (!query.next() || query.value(0).toInt() != 1) {
        return fail("This SQLite library cannot enforce foreign keys.");
    }
    return true;
}

// Brings the tables up to CURRENT_SCHEMA_VERSION:
//   version 0       a new, empty file: create the version 1 tables
//   version 1       already up to date: nothing to do
//   higher version  made by a newer Finora: refuse and leave the file untouched
bool DatabaseManager::upgradeSchema()
{
    int version = schemaVersion();
    if (version < 0) {
        return false;   // schemaVersion() has already set the error message
    }
    if (version > CURRENT_SCHEMA_VERSION) {
        return fail("This database was created by a newer version of Finora (schema version "
                    + QString::number(version) + "). It was not changed.");
    }
    if (version == CURRENT_SCHEMA_VERSION) {
        return true;
    }

    // Everything happens inside one database transaction: either every
    // step is saved, or after rollback() none of them is.
    QSqlDatabase db = QSqlDatabase::database(connectionName, false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }

    bool ok = true;
    if (version < 1) {
        ok = createVersion1Tables();
    }
    // Milestone 4 will add:  if (ok && version < 2) { ok = createVersion2Tables(); }

    // PRAGMA cannot use placeholders, so the number is written into the
    // text. It is a constant from this program, never user input.
    if (ok) {
        ok = runStatement("PRAGMA user_version = " + QString::number(CURRENT_SCHEMA_VERSION));
    }

    if (!ok) {
        db.rollback();
        return false;
    }
    if (!db.commit()) {
        return fail("Could not save the new tables: " + db.lastError().text());
    }
    return true;
}

bool DatabaseManager::createVersion1Tables()
{
    // Every currency Finora supports. A transaction's currency must be listed here.
    bool ok = runStatement(
        "CREATE TABLE IF NOT EXISTS currencies ("
        "  code           TEXT PRIMARY KEY CHECK (length(code) = 3),"
        "  name           TEXT NOT NULL,"
        "  symbol         TEXT NOT NULL,"
        "  decimal_places INTEGER NOT NULL CHECK (decimal_places BETWEEN 0 AND 3)"
        ")");
    if (!ok) {
        return false;
    }

    // One row per income or expense. The CHECK rules repeat the C++
    // validation, so even SQL that bypasses this class cannot store bad data.
    ok = runStatement(
        "CREATE TABLE IF NOT EXISTS transactions ("
        "  id               INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  type             TEXT    NOT NULL CHECK (type IN ('income', 'expense')),"
        "  amount_minor     INTEGER NOT NULL CHECK (amount_minor > 0 AND amount_minor <= 999999999999),"
        "  currency_code    TEXT    NOT NULL REFERENCES currencies (code),"
        "  category         TEXT    NOT NULL CHECK (length(trim(category)) > 0),"
        "  description      TEXT    NOT NULL DEFAULT '',"
        "  transaction_date TEXT    NOT NULL CHECK (transaction_date IS date(transaction_date))"
        ")");
    if (!ok) {
        return false;
    }

    return insertSupportedCurrencies();
}

// Adds PKR, USD and EUR from SUPPORTED_CURRENCIES in core/Transaction.h.
// INSERT OR IGNORE skips a currency that is already in the table.
bool DatabaseManager::insertSupportedCurrencies()
{
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    bool prepared = query.prepare(
        "INSERT OR IGNORE INTO currencies (code, name, symbol, decimal_places) "
        "VALUES (:code, :name, :symbol, :decimals)");
    if (!prepared) {
        return fail("Could not prepare to add currencies: " + query.lastError().text());
    }

    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        query.bindValue(":code", QString::fromUtf8(SUPPORTED_CURRENCIES[i].code));
        query.bindValue(":name", QString::fromUtf8(SUPPORTED_CURRENCIES[i].name));
        query.bindValue(":symbol", QString::fromUtf8(SUPPORTED_CURRENCIES[i].symbol));
        query.bindValue(":decimals", SUPPORTED_CURRENCIES[i].decimalPlaces);

        if (!query.exec()) {
            return fail("Could not add the currency " + QString::fromUtf8(SUPPORTED_CURRENCIES[i].code)
                        + ": " + query.lastError().text());
        }
    }
    return true;
}

// Runs one SQL statement that returns no rows, such as CREATE TABLE.
bool DatabaseManager::runStatement(const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.exec(sql)) {
        return fail("A database command failed: " + query.lastError().text());
    }
    return true;
}

// Remembers the error message and returns false, so a function can
// report a problem in one line:   return fail("...");
bool DatabaseManager::fail(const QString &message)
{
    errorMessage = message;
    return false;
}
