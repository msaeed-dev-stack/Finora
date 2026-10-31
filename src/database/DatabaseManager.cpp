// =====================================================================
//  DatabaseManager.cpp
// =====================================================================
#include "DatabaseManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

#include "SubscriptionStore.h"
#include "core/Subscription.h"

// ---- Helpers used only in this file ----------------------------------

// Fills the placeholders :type, :amount, :currency, :category,
// :description and :date of a prepared INSERT or UPDATE statement.
void bindTransactionValues(QSqlQuery &query, const Transaction &transaction)
{
    query.bindValue(":type", QString::fromStdString(transaction.type));
    query.bindValue(":amount", transaction.amountMinor);
    query.bindValue(":currency", QString::fromStdString(transaction.currencyCode));
    query.bindValue(":category", QString::fromStdString(transaction.category));
    query.bindValue(":description", QString::fromStdString(transaction.description));
    query.bindValue(":date", QString::fromStdString(transaction.date));
}

// Copies the current row of a query into a Transaction. The query must
// select: id, type, amount_minor, currency_code, category, description,
// transaction_date, in that order.
Transaction readTransactionRow(const QSqlQuery &query)
{
    Transaction transaction;
    transaction.id = query.value(0).toInt();
    transaction.type = query.value(1).toString().toStdString();
    transaction.amountMinor = query.value(2).toLongLong();
    transaction.currencyCode = query.value(3).toString().toStdString();
    transaction.category = query.value(4).toString().toStdString();
    transaction.description = query.value(5).toString().toStdString();
    transaction.date = query.value(6).toString().toStdString();
    return transaction;
}

// ---- DatabaseManager --------------------------------------------------

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

int DatabaseManager::storedSchemaVersion(const QString &databasePath)
{
    // Opening a missing file would create it, so check first.
    if (databasePath.isEmpty() || !QFileInfo::exists(databasePath)) {
        return -1;
    }
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        return -1;
    }

    int version = -1;
    const QString checkName = "finora_version_check";
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", checkName);
        db.setDatabaseName(databasePath);
        db.setConnectOptions("QSQLITE_OPEN_READONLY");   // this connection can never change the file
        if (db.open()) {
            QSqlQuery query(db);
            if (query.exec("PRAGMA user_version") && query.next()) {
                version = query.value(0).toInt();
            }
        }   // `query` is destroyed here
        db.close();
    }   // `db` is destroyed here, before the connection is removed
    QSqlDatabase::removeDatabase(checkName);
    return version;
}

bool DatabaseManager::open(const QString &databasePath)
{
    errorMessage.clear();
    backupPath.clear();
    needsUpgrade = false;

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

QString DatabaseManager::lastBackupPath() const
{
    return backupPath;
}

QString DatabaseManager::sqlConnectionName() const
{
    return connectionName;
}

void DatabaseManager::setUpgradeAllowed(bool allowed)
{
    upgradeAllowed = allowed;
}

bool DatabaseManager::upgradeNeeded() const
{
    return needsUpgrade;
}

void DatabaseManager::attachSession(const Session &session)
{
    attachedSession = &session;
}

bool DatabaseManager::currentOwner(QVariant &ownerId)
{
    // No Session attached (the database tests of Milestones 2 to 4A):
    // the records that belong to no account. A null QVariant becomes SQL
    // NULL, and "user_id IS NULL" finds them.
    if (attachedSession == nullptr) {
        ownerId = QVariant();
        return true;
    }
    // A Session is attached but nobody is signed in: no records at all.
    if (!attachedSession->isSignedIn()) {
        return fail("Please sign in first.");
    }
    ownerId = attachedSession->userId();
    return true;
}

bool DatabaseManager::signedInUserId(int &userId) const
{
    userId = 0;
    if (attachedSession == nullptr || !attachedSession->isSignedIn()) {
        return false;
    }
    userId = attachedSession->userId();
    return true;
}

bool DatabaseManager::accountsInUse() const
{
    return attachedSession != nullptr;
}

PlanId DatabaseManager::currentPlan()
{
    // No account (no Session, or nobody signed in): the Free rules apply.
    int userId = 0;
    if (!opened || !signedInUserId(userId)) {
        return PLAN_FREE;
    }

    // The signed-in person's own subscription row. If it cannot be read,
    // the answer is Free, so a paid feature is refused, never allowed.
    SubscriptionStore store(*this);
    Subscription subscription;
    if (!store.loadSubscription(subscription)) {
        return PLAN_FREE;
    }

    // planAt() also applies every deadline that has passed: a paid period
    // that ended a minute ago no longer counts, even before its row is updated.
    std::string now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
    return planAt(subscription, now);
}

bool DatabaseManager::requireFeature(Feature feature)
{
    errorMessage.clear();
    PlanId plan = currentPlan();
    if (!featureAvailable(feature) || !planIncludes(plan, feature)) {
        return fail(QString::fromStdString(lockedFeatureMessage(feature, plan)));
    }
    return true;
}

int DatabaseManager::budgetLimit()
{
    if (!accountsInUse()) {
        return UNLIMITED_BUDGETS;   // no accounts, so no plans (the Milestone 2 to 4A tests)
    }
    return activeBudgetLimit(currentPlan());
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

    // The new row belongs to the signed-in person (see currentOwner).
    QVariant owner;
    if (!currentOwner(owner)) {
        return false;
    }

    // A prepared statement: the SQL text contains only placeholders such
    // as :description, and the values are sent separately. A description
    // like  Ali's shop  therefore can never change the SQL command.
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    bool prepared = query.prepare(
        "INSERT INTO transactions "
        "(user_id, type, amount_minor, currency_code, category, description, transaction_date) "
        "VALUES (:owner, :type, :amount, :currency, :category, :description, :date)");
    if (!prepared) {
        return fail("Could not prepare to save the transaction: " + query.lastError().text());
    }

    bindTransactionValues(query, transaction);
    query.bindValue(":owner", owner);

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

    QVariant owner;
    if (!currentOwner(owner)) {
        return false;
    }

    // "user_id IS :owner" finds only the owner's rows: another person's
    // id gives "not found", exactly like an id that does not exist.
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    bool prepared = query.prepare(
        "SELECT id, type, amount_minor, currency_code, category, description, transaction_date "
        "FROM transactions WHERE id = :id AND user_id IS :owner");
    if (!prepared) {
        return fail("Could not prepare to read the transaction: " + query.lastError().text());
    }
    query.bindValue(":id", id);
    query.bindValue(":owner", owner);

    if (!query.exec()) {
        return fail("Could not read the transaction: " + query.lastError().text());
    }
    if (!query.next()) {
        return fail("No transaction with id " + QString::number(id) + " was found.");
    }

    result = readTransactionRow(query);
    return true;
}

bool DatabaseManager::loadTransactions(std::vector<Transaction> &results)
{
    errorMessage.clear();
    results.clear();
    if (!opened) {
        return fail("The database is not open.");
    }

    QVariant owner;
    if (!currentOwner(owner)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    query.prepare("SELECT id, type, amount_minor, currency_code, category, description, transaction_date "
                  "FROM transactions WHERE user_id IS :owner ORDER BY transaction_date DESC, id DESC");
    query.bindValue(":owner", owner);
    if (!query.exec()) {
        return fail("Could not load the transactions: " + query.lastError().text());
    }

    // One row at a time until next() returns false after the last row.
    while (query.next()) {
        results.push_back(readTransactionRow(query));
    }
    return true;
}

bool DatabaseManager::updateTransaction(const Transaction &transaction)
{
    errorMessage.clear();
    if (!opened) {
        return fail("The database is not open.");
    }
    if (transaction.id <= 0) {
        return fail("A transaction id must be a number above 0.");
    }

    std::string problem = validateTransaction(transaction);
    if (!problem.empty()) {
        return fail(QString::fromStdString(problem));
    }

    QVariant owner;
    if (!currentOwner(owner)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    bool prepared = query.prepare(
        "UPDATE transactions SET type = :type, amount_minor = :amount, currency_code = :currency, "
        "category = :category, description = :description, transaction_date = :date, "
        "updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') "
        "WHERE id = :id AND user_id IS :owner");
    if (!prepared) {
        return fail("Could not prepare to update the transaction: " + query.lastError().text());
    }
    bindTransactionValues(query, transaction);
    query.bindValue(":id", transaction.id);
    query.bindValue(":owner", owner);

    if (!query.exec()) {
        return fail("Could not update the transaction: " + query.lastError().text());
    }
    // numRowsAffected() tells how many rows the UPDATE changed: 0 means no such id.
    if (query.numRowsAffected() == 0) {
        return fail("No transaction with id " + QString::number(transaction.id) + " was found.");
    }
    return true;
}

bool DatabaseManager::deleteTransaction(int id)
{
    errorMessage.clear();
    if (!opened) {
        return fail("The database is not open.");
    }
    if (id <= 0) {
        return fail("A transaction id must be a number above 0.");
    }

    QVariant owner;
    if (!currentOwner(owner)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    bool prepared = query.prepare("DELETE FROM transactions WHERE id = :id AND user_id IS :owner");
    if (!prepared) {
        return fail("Could not prepare to delete the transaction: " + query.lastError().text());
    }
    query.bindValue(":id", id);
    query.bindValue(":owner", owner);

    if (!query.exec()) {
        return fail("Could not delete the transaction: " + query.lastError().text());
    }
    if (query.numRowsAffected() == 0) {
        return fail("No transaction with id " + QString::number(id) + " was found.");
    }
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

// Brings the tables up to CURRENT_SCHEMA_VERSION, one version at a time,
// all inside one database transaction:
//   version 0       a new, empty file: create the version 1 tables, then
//                   upgrade them step by step to the newest version
//   version 1       a file from Milestone 2 or 3: save and check a backup,
//                   then upgrade it to version 2, 3 and 4
//   version 2       a file from Phase 4A or 4B: save and check a backup,
//                   then upgrade it to version 3 and 4
//   version 3       a file from Phase 4C: save and check a backup, then
//                   upgrade it to version 4
//   version 4       already up to date: nothing to do
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

    // A file that already has tables holds the user's records. It is
    // upgraded only when that is allowed: Finora asks the person first.
    // Reading the version above may have let SQLite finish undoing a
    // change that was interrupted earlier (for example by a power cut);
    // that only restores the last saved state.
    bool hasUserData = (version >= 1);
    if (hasUserData && !upgradeAllowed) {
        needsUpgrade = true;
        return fail("This data file uses layout " + QString::number(version) + " and must be upgraded to layout "
                    + QString::number(CURRENT_SCHEMA_VERSION) + " first. It was not changed.");
    }

    // A copy is saved and checked BEFORE anything in the file changes.
    if (hasUserData && !backupBeforeUpgrade(version)) {
        return false;
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
    if (ok && version < 2) {
        ok = upgradeToVersion2();
    }
    if (ok && version < 3) {
        ok = upgradeToVersion3();
    }
    if (ok && version < 4) {
        ok = upgradeToVersion4();
    }

    // Every foreign key must still point to an existing row.
    if (ok) {
        ok = checkForeignKeys();
    }
    if (ok && hasUserData) {
        ok = recordAuditEvent("database_upgraded",
                              "Schema version " + QString::number(version) + " to "
                                  + QString::number(CURRENT_SCHEMA_VERSION) + ". Backup: "
                                  + QFileInfo(backupPath).fileName());
    }

    // PRAGMA cannot use placeholders, so the number is written into the
    // text. It is a constant from this program, never user input.
    if (ok) {
        ok = runStatement("PRAGMA user_version = " + QString::number(CURRENT_SCHEMA_VERSION));
    }

    if (!ok) {
        QString reason = errorMessage;
        db.rollback();   // undo every step since db.transaction()
        if (hasUserData) {
            return fail("Finora could not upgrade the database, so nothing in it was changed. "
                        "A backup copy is kept at " + QDir::toNativeSeparators(backupPath)
                        + ". Reason: " + reason);
        }
        return false;
    }
    if (!db.commit()) {
        QString reason = db.lastError().text();
        db.rollback();
        return fail("Could not save the new tables, so nothing was changed: " + reason);
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

// Saves a complete copy of the database in a "backups" folder next to it
// and checks the copy, before an upgrade changes anything.
bool DatabaseManager::backupBeforeUpgrade(int fromVersion)
{
    QString folder = QFileInfo(currentPath).absolutePath() + "/backups";
    if (!QDir().mkpath(folder)) {
        return fail("Could not create the backup folder " + QDir::toNativeSeparators(folder)
                    + ". The database was not changed.");
    }

    // For example  finora-v1-backup-2026-10-30_15-04-59.db . If that name
    // is already taken, a number is added:  ...-15-04-59-2.db
    QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString baseName = folder + "/finora-v" + QString::number(fromVersion) + "-backup-" + stamp;
    QString path = baseName + ".db";
    int number = 2;
    while (QFile::exists(path)) {
        path = baseName + "-" + QString::number(number) + ".db";
        number++;
    }

    // What the copy must contain: every transaction and, from version 2
    // on, every account.
    long long transactionCount = 0;
    long long accountCount = 0;
    if (!readNumber("SELECT COUNT(*) FROM transactions", transactionCount)) {
        return false;
    }
    if (fromVersion >= 2 && !readNumber("SELECT COUNT(*) FROM users", accountCount)) {
        return false;
    }

    // VACUUM INTO writes a complete and consistent copy into a new file.
    // The file name is bound as a value, like any other value.
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.prepare("VACUUM INTO :file")) {
        return fail("Could not prepare the backup: " + query.lastError().text()
                    + " The database was not changed.");
    }
    query.bindValue(":file", path);
    if (!query.exec()) {
        return fail("Could not save a backup copy: " + query.lastError().text()
                    + " The database was not changed.");
    }

    if (!verifyBackup(path, fromVersion, transactionCount, accountCount)) {
        return false;
    }
    backupPath = path;
    return true;
}

// Opens the backup through a second, read-only connection and checks it:
// SQLite's own integrity check, the schema version, the number of
// transactions and (from version 2 on) the number of accounts must all
// be right before the upgrade may start.
bool DatabaseManager::verifyBackup(const QString &backupFile, int expectedVersion,
                                   long long expectedTransactions, long long expectedAccounts)
{
    QString problem;
    QString checkName = connectionName + "_backup_check";
    {
        QSqlDatabase backup = QSqlDatabase::addDatabase("QSQLITE", checkName);
        backup.setDatabaseName(backupFile);
        backup.setConnectOptions("QSQLITE_OPEN_READONLY");
        if (!backup.open()) {
            problem = "the copy could not be opened";
        } else {
            QSqlQuery query(backup);
            if (!query.exec("PRAGMA integrity_check") || !query.next()
                || query.value(0).toString() != "ok") {
                problem = "SQLite's integrity check of the copy failed";
            } else if (!query.exec("PRAGMA user_version") || !query.next()
                       || query.value(0).toInt() != expectedVersion) {
                problem = "the copy has a different schema version";
            } else if (!query.exec("SELECT COUNT(*) FROM transactions") || !query.next()
                       || query.value(0).toLongLong() != expectedTransactions) {
                problem = "the copy does not hold every transaction";
            } else if (expectedVersion >= 2
                       && (!query.exec("SELECT COUNT(*) FROM users") || !query.next()
                           || query.value(0).toLongLong() != expectedAccounts)) {
                problem = "the copy does not hold every account";
            }
        }   // `query` is destroyed here
        backup.close();
    }   // `backup` is destroyed here, before the connection is removed
    QSqlDatabase::removeDatabase(checkName);

    if (!problem.isEmpty()) {
        return fail("The backup copy could not be verified (" + problem + "). "
                    "The database was not changed.");
    }
    return true;
}

// Version 1 -> 2. Runs inside the transaction started by upgradeSchema(),
// so if any step fails, rollback() undoes all of them.
bool DatabaseManager::upgradeToVersion2()
{
    // ---- Remember what is there now, to compare after copying ----
    long long rowsBefore = 0;
    long long amountBefore = 0;
    long long lastIdBefore = 0;
    if (!readNumber("SELECT COUNT(*) FROM transactions", rowsBefore)
        || !readNumber("SELECT COALESCE(SUM(amount_minor), 0) FROM transactions", amountBefore)
        || !readNumber("SELECT COALESCE(MAX(seq), 0) FROM sqlite_sequence WHERE name = 'transactions'",
                       lastIdBefore)) {
        return false;
    }

    // ---- 1. Settings of this Finora installation (theme, reduced motion) ----
    bool ok = runStatement(
        "CREATE TABLE app_settings ("
        "  key   TEXT PRIMARY KEY CHECK (length(key) BETWEEN 1 AND 64),"
        "  value TEXT NOT NULL"
        ")");

    // ---- 2. Accounts. The sign-in screens that use these tables are
    //         Phase 4B; the tables are created now so that the
    //         transactions table below can refer to an owner. ----
    // password_hash holds a one-way Argon2id hash with its own salt and
    // settings, never the password itself.
    if (ok) {
        ok = runStatement(
            "CREATE TABLE users ("
            "  id                 INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  email              TEXT NOT NULL UNIQUE COLLATE NOCASE CHECK (length(email) BETWEEN 3 AND 254),"
            "  display_name       TEXT NOT NULL CHECK (length(trim(display_name)) BETWEEN 1 AND 60),"
            "  password_hash      TEXT NOT NULL CHECK (length(password_hash) > 0),"
            "  failed_login_count INTEGER NOT NULL DEFAULT 0 CHECK (failed_login_count >= 0),"
            "  locked_until       TEXT,"
            "  created_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
            "  updated_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
            "  last_login_at      TEXT"
            ")");
    }
    if (ok) {
        ok = runStatement(
            "CREATE TABLE user_settings ("
            "  user_id INTEGER NOT NULL REFERENCES users (id) ON DELETE CASCADE,"
            "  key     TEXT NOT NULL CHECK (length(key) BETWEEN 1 AND 64),"
            "  value   TEXT NOT NULL,"
            "  PRIMARY KEY (user_id, key)"
            ")");
    }
    if (ok) {
        ok = runStatement(
            "CREATE TABLE recovery_codes ("
            "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  user_id    INTEGER NOT NULL REFERENCES users (id) ON DELETE CASCADE,"
            "  code_hash  TEXT NOT NULL CHECK (length(code_hash) > 0),"
            "  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
            "  used_at    TEXT"
            ")");
    }
    if (ok) {
        ok = runStatement(
            "CREATE TABLE login_sessions ("
            "  id             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  user_id        INTEGER NOT NULL REFERENCES users (id) ON DELETE CASCADE,"
            "  started_at     TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
            "  last_active_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
            "  expires_at     TEXT NOT NULL,"
            "  ended_at       TEXT,"
            "  end_reason     TEXT"
            ")");
    }
    if (ok) {
        ok = runStatement(
            "CREATE TABLE audit_events ("
            "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  user_id    INTEGER REFERENCES users (id) ON DELETE CASCADE,"
            "  event_type TEXT NOT NULL CHECK (length(event_type) BETWEEN 1 AND 64),"
            "  details    TEXT NOT NULL DEFAULT '',"
            "  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))"
            ")");
    }

    // ---- 3. Rebuild transactions with four new columns ----
    // SQLite cannot add a UNIQUE column to an existing table, so a new
    // table is made, the rows are copied, and the old table is replaced.
    //   user_id   the owner; NULL = a record made before accounts existed
    //   uuid      a random id that stays the same on every computer, for a future cloud sync
    //   created_at, updated_at   when the row was saved and last changed (UTC)
    if (ok) {
        ok = runStatement(
            "CREATE TABLE transactions_v2 ("
            "  id               INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  uuid             TEXT    NOT NULL UNIQUE DEFAULT (lower(hex(randomblob(16)))),"
            "  user_id          INTEGER REFERENCES users (id) ON DELETE CASCADE,"
            "  type             TEXT    NOT NULL CHECK (type IN ('income', 'expense')),"
            "  amount_minor     INTEGER NOT NULL CHECK (amount_minor > 0 AND amount_minor <= 999999999999),"
            "  currency_code    TEXT    NOT NULL REFERENCES currencies (code),"
            "  category         TEXT    NOT NULL CHECK (length(trim(category)) > 0),"
            "  description      TEXT    NOT NULL DEFAULT '',"
            "  transaction_date TEXT    NOT NULL CHECK (transaction_date IS date(transaction_date)),"
            "  created_at       TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
            "  updated_at       TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))"
            ")");
    }
    // The ids are copied unchanged. uuid and the times are filled by the
    // DEFAULT expressions, separately for every row.
    if (ok) {
        ok = runStatement(
            "INSERT INTO transactions_v2 "
            "(id, type, amount_minor, currency_code, category, description, transaction_date) "
            "SELECT id, type, amount_minor, currency_code, category, description, transaction_date "
            "FROM transactions");
    }

    // ---- 4. Prove that every row arrived unchanged ----
    long long rowsAfter = -1;
    long long amountAfter = -1;
    if (ok) {
        ok = readNumber("SELECT COUNT(*) FROM transactions_v2", rowsAfter)
             && readNumber("SELECT COALESCE(SUM(amount_minor), 0) FROM transactions_v2", amountAfter);
    }
    if (ok && (rowsAfter != rowsBefore || amountAfter != amountBefore)) {
        return fail("The copied transactions do not match the original ones ("
                    + QString::number(rowsAfter) + " of " + QString::number(rowsBefore) + " rows).");
    }

    // ---- 5. Swap the tables ----
    if (ok) {
        ok = runStatement("DROP TABLE transactions");
    }
    if (ok) {
        ok = runStatement("ALTER TABLE transactions_v2 RENAME TO transactions");
    }
    // AUTOINCREMENT never gives out an id twice. The highest id ever used
    // (also by deleted rows) is carried over, so that promise still holds.
    // If every old row had been deleted, the new table has no entry in
    // sqlite_sequence at all, so the entry is written fresh. The numbers
    // come from the database itself, never from the user.
    long long lastIdNow = 0;
    if (ok && lastIdBefore > 0) {
        ok = readNumber("SELECT COALESCE(MAX(seq), 0) FROM sqlite_sequence WHERE name = 'transactions'",
                        lastIdNow);
    }
    if (ok && lastIdNow < lastIdBefore) {
        ok = runStatement("DELETE FROM sqlite_sequence WHERE name = 'transactions'")
             && runStatement("INSERT INTO sqlite_sequence (name, seq) VALUES ('transactions', "
                             + QString::number(lastIdBefore) + ")");
    }
    // An index is a sorted list that lets SQLite find one owner's
    // transactions by date without reading the whole table.
    if (ok) {
        ok = runStatement("CREATE INDEX transactions_by_owner_and_date "
                          "ON transactions (user_id, transaction_date)");
    }

    // ---- 6. Budgets: one monthly limit per category and currency ----
    if (ok) {
        ok = runStatement(
            "CREATE TABLE budgets ("
            "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  user_id       INTEGER REFERENCES users (id) ON DELETE CASCADE,"
            "  category      TEXT    NOT NULL CHECK (length(trim(category)) BETWEEN 1 AND 40),"
            "  currency_code TEXT    NOT NULL REFERENCES currencies (code),"
            "  limit_minor   INTEGER NOT NULL CHECK (limit_minor > 0 AND limit_minor <= 999999999999),"
            "  created_at    TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
            "  updated_at    TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))"
            ")");
    }
    // A UNIQUE index refuses a second Food budget in PKR for the same
    // owner. lower() makes "Food" and "food" the same; ifnull() treats
    // records without an owner as owner 0, because SQLite never counts
    // two NULLs as equal.
    if (ok) {
        ok = runStatement("CREATE UNIQUE INDEX budgets_one_per_category "
                          "ON budgets (ifnull(user_id, 0), lower(category), currency_code)");
    }
    return ok;
}

// Version 2 -> 3 (Phase 4C). Runs inside the transaction started by
// upgradeSchema(). It only ADDS two tables and fills one of them: no
// existing table, row or password hash is changed.
bool DatabaseManager::upgradeToVersion3()
{
    long long accountsBefore = 0;
    if (!readNumber("SELECT COUNT(*) FROM users", accountsBefore)) {
        return false;
    }

    // ---- 1. One subscription per account: its plan, status and dates ----
    // The CHECK rules are a second line of defence behind the C++ rules:
    // the plan and the status must agree, an active plan needs a renewal
    // date, and every paid status needs the time when its access ends.
    // UNIQUE (id, user_id) lets a payment name its subscription AND owner.
    bool ok = runStatement(
        "CREATE TABLE subscriptions ("
        "  id                     INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id                INTEGER NOT NULL UNIQUE REFERENCES users (id) ON DELETE CASCADE,"
        "  plan                   TEXT NOT NULL CHECK (plan IN ('free', 'plus', 'pro', 'business')),"
        "  status                 TEXT NOT NULL CHECK (status IN ('free', 'trial', 'active', 'past_due', "
        "                                                         'cancelled', 'expired')),"
        "  started_at             TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
        "  renews_at              TEXT,"
        "  expires_at             TEXT,"
        "  cancelled_at           TEXT,"
        "  scheduled_plan         TEXT CHECK (scheduled_plan IN ('plus', 'pro')),"
        "  trial_used             INTEGER NOT NULL DEFAULT 0 CHECK (trial_used IN (0, 1)),"
        "  last_payment_reference TEXT,"
        "  last_payment_status    TEXT CHECK (last_payment_status IN ('succeeded', 'failed')),"
        "  updated_at             TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
        "  UNIQUE (id, user_id),"
        "  CHECK ((plan = 'free') = (status IN ('free', 'expired'))),"
        "  CHECK (status <> 'active' OR renews_at IS NOT NULL),"
        "  CHECK (status NOT IN ('trial', 'active', 'past_due', 'cancelled') OR expires_at IS NOT NULL)"
        ")");

    // ---- 2. The billing history: every payment attempt ----
    // There is no column for card details, because Finora never collects
    // them. 'simulated' is the only provider: a real one needs a server.
    // The two-column foreign key makes sure that a payment's subscription
    // belongs to the same person as the payment.
    if (ok) {
        ok = runStatement(
            "CREATE TABLE payments ("
            "  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  user_id         INTEGER NOT NULL REFERENCES users (id) ON DELETE CASCADE,"
            "  subscription_id INTEGER NOT NULL,"
            "  plan            TEXT NOT NULL CHECK (plan IN ('plus', 'pro', 'business')),"
            "  reason          TEXT NOT NULL CHECK (reason IN ('new', 'upgrade', 'renewal')),"
            "  amount_minor    INTEGER NOT NULL CHECK (amount_minor > 0 AND amount_minor <= 1000000),"
            "  currency_code   TEXT NOT NULL DEFAULT 'USD' REFERENCES currencies (code),"
            "  status          TEXT NOT NULL DEFAULT 'pending' CHECK (status IN ('pending', 'succeeded', 'failed')),"
            "  provider        TEXT NOT NULL DEFAULT 'simulated' CHECK (provider IN ('simulated')),"
            "  reference       TEXT NOT NULL UNIQUE CHECK (length(reference) BETWEEN 8 AND 40),"
            "  failure_reason  TEXT,"
            "  period_start    TEXT,"
            "  period_end      TEXT,"
            "  created_at      TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
            "  completed_at    TEXT,"
            "  FOREIGN KEY (subscription_id, user_id) REFERENCES subscriptions (id, user_id) ON DELETE CASCADE,"
            "  CHECK (status <> 'succeeded' OR (period_start IS NOT NULL AND period_end IS NOT NULL"
            "                                   AND completed_at IS NOT NULL)),"
            "  CHECK (status <> 'failed' OR (failure_reason IS NOT NULL AND completed_at IS NOT NULL))"
            ")");
    }
    if (ok) {
        ok = runStatement("CREATE INDEX payments_by_owner ON payments (user_id, created_at)");
    }

    // ---- 3. Every existing account starts on Finora Free, from the day
    //         the account was created ----
    if (ok) {
        ok = runStatement("INSERT INTO subscriptions (user_id, plan, status, started_at) "
                          "SELECT id, 'free', 'free', created_at FROM users");
    }

    // ---- 4. Prove that every account received exactly one subscription ----
    long long subscriptionsAfter = -1;
    if (ok) {
        ok = readNumber("SELECT COUNT(*) FROM subscriptions", subscriptionsAfter);
    }
    if (ok && subscriptionsAfter != accountsBefore) {
        return fail("Not every account received the Free plan (" + QString::number(subscriptionsAfter)
                    + " of " + QString::number(accountsBefore) + ").");
    }
    return ok;
}

// Version 3 -> 4 (Milestone 5). Runs inside the transaction started by
// upgradeSchema(). It only ADDS two empty tables: no existing table or
// row is changed.
bool DatabaseManager::upgradeToVersion4()
{
    // ---- 1. Savings goals: one row per goal ----
    // Goals are a Finora Plus feature, so a goal always belongs to an
    // account (user_id NOT NULL). target_date is NULL for a goal without a date.
    bool ok = runStatement(
        "CREATE TABLE goals ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id       INTEGER NOT NULL REFERENCES users (id) ON DELETE CASCADE,"
        "  name          TEXT    NOT NULL CHECK (length(trim(name)) BETWEEN 1 AND 40),"
        "  currency_code TEXT    NOT NULL REFERENCES currencies (code),"
        "  target_minor  INTEGER NOT NULL CHECK (target_minor > 0 AND target_minor <= 999999999999),"
        "  target_date   TEXT    CHECK (target_date IS NULL OR target_date IS date(target_date)),"
        "  created_at    TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
        "  updated_at    TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))"
        ")");
    // Two goals of the same person may not have the same name ("Laptop"
    // and "laptop" count as the same).
    if (ok) {
        ok = runStatement("CREATE UNIQUE INDEX goals_one_name_per_owner ON goals (user_id, lower(name))");
    }

    // ---- 2. Money put into a goal (positive) or taken out (negative) ----
    // Contributions are never edited or deleted one by one: a mistake is
    // corrected with a new contribution, so the history stays complete.
    // Deleting a whole goal deletes its contributions with it (CASCADE).
    if (ok) {
        ok = runStatement(
            "CREATE TABLE goal_contributions ("
            "  id                INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  goal_id           INTEGER NOT NULL REFERENCES goals (id) ON DELETE CASCADE,"
            "  amount_minor      INTEGER NOT NULL CHECK (amount_minor <> 0"
            "                                            AND amount_minor BETWEEN -999999999999 AND 999999999999),"
            "  contribution_date TEXT    NOT NULL CHECK (contribution_date IS date(contribution_date)),"
            "  note              TEXT    NOT NULL DEFAULT '' CHECK (length(note) <= 100),"
            "  created_at        TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))"
            ")");
    }
    if (ok) {
        ok = runStatement("CREATE INDEX goal_contributions_by_goal ON goal_contributions (goal_id, id)");
    }

    // ---- 3. A second guard behind the C++ rules ----
    // A trigger is SQL that SQLite runs by itself before every new
    // contribution. It refuses one that would make the saved amount
    // negative or larger than the largest amount, even if some other
    // program wrote to this file without Finora's checks.
    if (ok) {
        ok = runStatement(
            "CREATE TRIGGER goal_saved_amount_stays_valid "
            "BEFORE INSERT ON goal_contributions "
            "WHEN (SELECT COALESCE(SUM(amount_minor), 0) FROM goal_contributions WHERE goal_id = NEW.goal_id)"
            "     + NEW.amount_minor NOT BETWEEN 0 AND 999999999999 "
            "BEGIN "
            "  SELECT RAISE(ABORT, 'The saved amount of a goal must stay between zero and the largest amount.'); "
            "END");
    }
    return ok;
}

// PRAGMA foreign_key_check lists every row whose foreign key points to a
// row that does not exist. An empty result means every link is valid.
bool DatabaseManager::checkForeignKeys()
{
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.exec("PRAGMA foreign_key_check")) {
        return fail("Could not check the foreign keys: " + query.lastError().text());
    }
    if (query.next()) {
        return fail("The table " + query.value(0).toString()
                    + " has a row that refers to a record that does not exist.");
    }
    return true;
}

QString DatabaseManager::setting(const QString &key, const QString &defaultValue)
{
    errorMessage.clear();
    if (!opened) {
        fail("The database is not open.");
        return defaultValue;
    }

    // Signed in: the person's own settings. Otherwise: the settings of
    // this Finora installation (used by the sign-in window).
    int userId = 0;
    bool personal = signedInUserId(userId);
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    QString sql = "SELECT value FROM app_settings WHERE key = :key";
    if (personal) {
        sql = "SELECT value FROM user_settings WHERE user_id = :user AND key = :key";
    }
    if (!query.prepare(sql)) {
        fail("Could not prepare to read a setting: " + query.lastError().text());
        return defaultValue;
    }
    query.bindValue(":key", key);
    if (personal) {
        query.bindValue(":user", userId);
    }
    if (!query.exec()) {
        fail("Could not read a setting: " + query.lastError().text());
        return defaultValue;
    }
    if (!query.next()) {
        return defaultValue;   // never saved
    }
    return query.value(0).toString();
}

bool DatabaseManager::saveSetting(const QString &key, const QString &value)
{
    errorMessage.clear();
    if (!opened) {
        return fail("The database is not open.");
    }

    // INSERT OR REPLACE adds the row, or replaces the row that already has
    // this key. Signed in, the setting is the person's own.
    int userId = 0;
    bool personal = signedInUserId(userId);
    QString sql = "INSERT OR REPLACE INTO app_settings (key, value) VALUES (:key, :value)";
    if (personal) {
        sql = "INSERT OR REPLACE INTO user_settings (user_id, key, value) VALUES (:user, :key, :value)";
    }
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.prepare(sql)) {
        return fail("Could not prepare to save a setting: " + query.lastError().text());
    }
    query.bindValue(":key", key);
    query.bindValue(":value", value);
    if (personal) {
        query.bindValue(":user", userId);
    }
    if (!query.exec()) {
        return fail("Could not save a setting: " + query.lastError().text());
    }
    return true;
}

// The rates are saved as text such as "278.5000" and read back through
// parseRate(), the same check that a typed rate goes through.
ExchangeRates DatabaseManager::exchangeRates()
{
    ExchangeRates rates;
    long long rate = 0;
    if (parseRate(setting("usd_in_pkr", "").toStdString(), rate).empty()) {
        rates.usdInPkr = rate;
    }
    if (parseRate(setting("eur_in_pkr", "").toStdString(), rate).empty()) {
        rates.eurInPkr = rate;
    }
    return rates;
}

bool DatabaseManager::saveExchangeRates(const ExchangeRates &rates)
{
    errorMessage.clear();
    if (!opened) {
        return fail("The database is not open.");
    }
    // 0 means "not entered". Any other value must be a real rate.
    if (rates.usdInPkr < 0 || rates.usdInPkr > MAX_RATE || rates.eurInPkr < 0 || rates.eurInPkr > MAX_RATE) {
        return fail("A rate must be more than zero and at most " + QString::fromStdString(formatRate(MAX_RATE))
                    + ".");
    }

    // "" for a rate that is not entered. (A QString made without a value
    // would be saved as SQL NULL, which the settings tables refuse.)
    QString usdText = "";
    QString eurText = "";
    if (rates.usdInPkr > 0) {
        usdText = QString::fromStdString(formatRate(rates.usdInPkr));
    }
    if (rates.eurInPkr > 0) {
        eurText = QString::fromStdString(formatRate(rates.eurInPkr));
    }

    // The three values are saved together or not at all.
    QSqlDatabase db = QSqlDatabase::database(connectionName, false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }
    bool ok = saveSetting("usd_in_pkr", usdText) && saveSetting("eur_in_pkr", eurText)
              && saveSetting("rates_saved_on", QDate::currentDate().toString(Qt::ISODate));
    if (!ok) {
        QString reason = errorMessage;
        db.rollback();
        return fail(reason);
    }
    if (!db.commit()) {
        QString reason = db.lastError().text();
        db.rollback();
        return fail("Could not save the rates: " + reason);
    }
    return true;
}

QString DatabaseManager::exchangeRatesSavedOn()
{
    return setting("rates_saved_on", "");
}

bool DatabaseManager::recordAuditEvent(const QString &eventType, const QString &details)
{
    if (!opened) {
        return fail("The database is not open.");
    }

    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.prepare("INSERT INTO audit_events (user_id, event_type, details) VALUES (:user, :type, :details)")) {
        return fail("Could not prepare to record an event: " + query.lastError().text());
    }
    int userId = 0;
    if (signedInUserId(userId)) {
        query.bindValue(":user", userId);
    } else {
        query.bindValue(":user", QVariant());   // NULL: no account
    }
    query.bindValue(":type", eventType);
    query.bindValue(":details", details);
    if (!query.exec()) {
        return fail("Could not record an event: " + query.lastError().text());
    }
    return true;
}

bool DatabaseManager::loadAuditHistory(std::vector<AuditEvent> &events, int limit)
{
    events.clear();
    if (!opened) {
        return fail("The database is not open.");
    }
    if (!requireFeature(FEATURE_AUDIT_HISTORY)) {
        return false;   // requireFeature() has explained which plan is needed
    }
    int userId = 0;
    if (!signedInUserId(userId)) {
        return fail("Please sign in first.");
    }

    // Only this person's own events: user_id comes from the Session.
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    query.prepare("SELECT event_type, details, created_at FROM audit_events "
                  "WHERE user_id = :owner ORDER BY id DESC LIMIT :limit");
    query.bindValue(":owner", userId);
    query.bindValue(":limit", limit);
    if (!query.exec()) {
        return fail("Could not load the audit history: " + query.lastError().text());
    }
    while (query.next()) {
        AuditEvent event;
        event.eventType = query.value(0).toString().toStdString();
        event.details = query.value(1).toString().toStdString();
        event.createdAt = query.value(2).toString().toStdString();
        events.push_back(event);
    }
    return true;
}

// Runs a query that returns one number, such as SELECT COUNT(*).
bool DatabaseManager::readNumber(const QString &sql, long long &number)
{
    QSqlQuery query(QSqlDatabase::database(connectionName, false));
    if (!query.exec(sql) || !query.next()) {
        return fail("A database check failed: " + query.lastError().text());
    }
    number = query.value(0).toLongLong();
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
