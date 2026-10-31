// =====================================================================
//  finora_billing_tests.cpp
//  Automated checks for subscriptions and billing (Milestone 4, Phase
//  4C): the upgrade to schema version 3, the Free plan of every
//  account, and the database rules that protect subscriptions and
//  payments. Since Milestone 5 also savings goals (a Finora Plus
//  feature, so they need accounts and plans), the upgrade to schema
//  version 4 and the saved exchange rates.
//
//  Every database lives in a temporary folder that QTemporaryDir deletes
//  at the end, so the tests never touch the real Finora database.
// =====================================================================
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QTemporaryDir>
#include <QVariant>

#include <iostream>
#include <string>
#include <vector>

#include "auth/AuthService.h"
#include "auth/PasswordHasher.h"
#include "billing/SimulatedPaymentGateway.h"
#include "billing/SubscriptionService.h"
#include "core/AccountRules.h"
#include "core/Budget.h"
#include "core/ExchangeRates.h"
#include "core/Goal.h"
#include "core/Plans.h"
#include "core/Subscription.h"
#include "core/Totals.h"
#include "core/Transaction.h"
#include "database/BudgetStore.h"
#include "database/DatabaseManager.h"
#include "database/GoalStore.h"
#include "database/SubscriptionStore.h"
#include "reports/ReportExporter.h"

// Counts how many checks ran and how many of them failed.
struct TestCounter {
    int checksRun = 0;
    int checksFailed = 0;
};

// Records one check. When it fails, `detail` (for example an error
// message) is printed to help find the problem.
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

const std::string GOOD_PASSWORD = "Correct Horse 9!";

// ---- SQL helpers ------------------------------------------------------
// They run SQL directly, bypassing Finora's classes, to look inside the
// file or to prove that SQLite's own rules protect the data. Only fixed
// test values are written into this SQL text.

// Runs SQL on the open connection of a DatabaseManager (its name).
bool runSql(const QString &connection, const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connection, false));
    return query.exec(sql);
}

long long numberFromSql(const QString &connection, const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connection, false));
    if (!query.exec(sql) || !query.next()) {
        return -1;
    }
    return query.value(0).toLongLong();
}

QString textFromSql(const QString &connection, const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connection, false));
    if (!query.exec(sql) || !query.next()) {
        return "(error)";
    }
    return query.value(0).toString();
}

// Every row of a table in rowid order, each value as text, so that two
// snapshots taken before and after an upgrade can be compared exactly.
QString tableSnapshot(const QString &connection, const QString &table, const QString &condition)
{
    QSqlQuery query(QSqlDatabase::database(connection, false));
    if (!query.exec("SELECT * FROM " + table + " WHERE " + condition + " ORDER BY rowid")) {
        return "(error) " + query.lastError().text();
    }
    QString text;
    while (query.next()) {
        QSqlRecord record = query.record();
        for (int i = 0; i < record.count(); i++) {
            QString value = "NULL";
            if (!query.value(i).isNull()) {
                value = query.value(i).toString();
            }
            text += record.fieldName(i) + "=" + value + ";";
        }
        text += "\n";
    }
    return text;
}

// Opens a short-lived connection to a CLOSED file, runs the statements
// in order and closes it again. Returns false at the first failure.
bool runSqlOnFile(const QString &path, const QStringList &statements)
{
    bool ok = true;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "billing_file_setup");
        db.setDatabaseName(path);
        ok = db.open();
        for (int i = 0; ok && i < statements.size(); i++) {
            QSqlQuery query(db);
            ok = query.exec(statements[i]);
            if (!ok) {
                std::cout << "        setup failed: " << statements[i].toStdString() << ": "
                          << query.lastError().text().toStdString() << "\n";
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("billing_file_setup");
    return ok;
}

// Snapshots of the given tables of a CLOSED file, in the same format as
// tableSnapshot().
QStringList snapshotsOfFile(const QString &path, const QStringList &tables)
{
    QStringList snapshots;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "billing_file_snapshot");
        db.setDatabaseName(path);
        db.setConnectOptions("QSQLITE_OPEN_READONLY");
        if (db.open()) {
            for (int i = 0; i < tables.size(); i++) {
                snapshots.append(tableSnapshot("billing_file_snapshot", tables[i], "1 = 1"));
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("billing_file_snapshot");
    return snapshots;
}

long long numberFromFile(const QString &path, const QString &sql)
{
    long long number = -1;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "billing_file_number");
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
    QSqlDatabase::removeDatabase("billing_file_number");
    return number;
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

// ---- Test accounts ----------------------------------------------------

// A database for one test: its own file, a Session attached to it, an
// AuthService and the two stores used by the tests.
struct TestAccounts {
    DatabaseManager database;
    Session session;
    AuthService auth;
    BudgetStore budgets;
    SubscriptionStore subscriptions;
    QString connection;
    QString path;

    TestAccounts(const QString &name, const QString &folder, const PasswordHasher &hasher)
        : database(name), auth(database, session, hasher, hasher), budgets(database),
          subscriptions(database), connection(name), path(folder + "/" + name + ".db")
    {
        database.open(path);
        database.attachSession(session);
    }
};

// Creates an account and leaves it signed in.
bool signUp(AuthService &auth, const std::string &name, const std::string &email)
{
    std::vector<std::string> codes;
    std::string password = GOOD_PASSWORD;
    return auth.signUp(name, email, password, codes);
}

bool signIn(AuthService &auth, const std::string &email)
{
    std::string password = GOOD_PASSWORD;
    return auth.signIn(email, password);
}

Transaction makeExpense(const std::string &category, long long amountMinor, const std::string &date)
{
    Transaction transaction;
    transaction.type = TYPE_EXPENSE;
    transaction.amountMinor = amountMinor;
    transaction.currencyCode = "PKR";
    transaction.category = category;
    transaction.description = "Test";
    transaction.date = date;
    return transaction;
}

Budget makeBudget(const std::string &category, long long limitMinor)
{
    Budget budget;
    budget.category = category;
    budget.currencyCode = "PKR";
    budget.limitMinor = limitMinor;
    return budget;
}

// The tables of schema version 2, which version 3 must leave untouched.
const QStringList VERSION_2_TABLES = {
    "app_settings", "audit_events", "budgets", "currencies", "login_sessions",
    "recovery_codes", "transactions", "user_settings", "users"
};

// The tables of schema version 3, which version 4 must leave untouched.
const QStringList VERSION_3_TABLES = {
    "app_settings", "audit_events", "budgets", "currencies", "login_sessions", "payments",
    "recovery_codes", "subscriptions", "transactions", "user_settings", "users"
};

// ---- 4C.1: the subscription data model --------------------------------

void testNewDatabase(TestCounter &counter, const QString &folder)
{
    DatabaseManager database("billing_new");
    bool opened = database.open(folder + "/billing_new.db");
    check(counter, "a new database is created at schema version 4",
          opened && database.schemaVersion() == 4, database.lastError());
    QStringList tables = database.tableNames();
    check(counter, "it has the subscriptions and payments tables",
          tables.contains("subscriptions") && tables.contains("payments"), tables.join(", "));
    check(counter, "both new tables start empty",
          numberFromSql("billing_new", "SELECT COUNT(*) FROM subscriptions") == 0
              && numberFromSql("billing_new", "SELECT COUNT(*) FROM payments") == 0);
    check(counter, "no column of either table can hold card details",
          numberFromSql("billing_new",
                        "SELECT COUNT(*) FROM (SELECT name FROM pragma_table_info('payments') "
                        "UNION ALL SELECT name FROM pragma_table_info('subscriptions')) "
                        "WHERE name LIKE '%card%' OR name LIKE '%cvv%' OR name LIKE '%cvc%' "
                        "OR name LIKE '%expiry%' OR name LIKE '%iban%' OR name LIKE '%pan%'") == 0);
}

void testSignUpStartsOnFree(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_signup", folder, quick);
    bool created = signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    check(counter, "an account is created", created, accounts.auth.lastError());

    check(counter, "the new account has exactly one subscription: Free",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM subscriptions") == 1
              && textFromSql(accounts.connection, "SELECT plan || '/' || status FROM subscriptions") == "free/free");
    check(counter, "it starts now, has no renewal or end date and no trial used",
          numberFromSql(accounts.connection,
                        "SELECT COUNT(*) FROM subscriptions WHERE length(started_at) = 20 AND renews_at IS NULL "
                        "AND expires_at IS NULL AND trial_used = 0") == 1);
    check(counter, "the start of the Free plan is in the audit history",
          numberFromSql(accounts.connection,
                        "SELECT COUNT(*) FROM audit_events WHERE event_type = 'plan_started'") == 1);

    Subscription amnaPlan;
    bool loaded = accounts.subscriptions.loadSubscription(amnaPlan);
    check(counter, "SubscriptionStore loads Amna's subscription while she is signed in",
          loaded && amnaPlan.id > 0 && amnaPlan.plan == PLAN_FREE && amnaPlan.status == STATUS_FREE
              && !amnaPlan.trialUsed && !amnaPlan.hasScheduledPlan,
          accounts.subscriptions.lastError());

    std::vector<PaymentRecord> payments;
    check(counter, "a new account has an empty billing history",
          accounts.subscriptions.loadPayments(payments) && payments.empty());

    accounts.auth.signOut("signed_out");
    signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com");
    Subscription bilalPlan;
    accounts.subscriptions.loadSubscription(bilalPlan);
    long long bilalRow = numberFromSql(accounts.connection,
                                       "SELECT s.id FROM subscriptions s JOIN users u ON u.id = s.user_id "
                                       "WHERE u.email = 'bilal@example.com'");
    check(counter, "signed in as Bilal, the store loads Bilal's own subscription, not Amna's",
          bilalPlan.id == bilalRow && bilalPlan.id != amnaPlan.id);

    accounts.auth.signOut("signed_out");
    Subscription nobody;
    check(counter, "while nobody is signed in, no subscription can be loaded",
          !accounts.subscriptions.loadSubscription(nobody)
              && accounts.subscriptions.lastError() == "Please sign in first.");
    check(counter, "while nobody is signed in, no billing history can be loaded",
          !accounts.subscriptions.loadPayments(payments));

    // Without a Session at all (the tests of Milestones 2 to 4A) there is
    // no account, so there is no subscription to load either.
    DatabaseManager noSession("billing_no_session");
    noSession.open(folder + "/billing_no_session.db");
    SubscriptionStore store(noSession);
    check(counter, "without a Session, subscriptions cannot be reached",
          !store.loadSubscription(nobody));
}

void testSignUpIsAllOrNothing(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_atomic", folder, quick);
    // A trigger makes SQLite refuse every new subscription, so the Free
    // plan cannot be started. The whole sign-up must then be undone.
    runSql(accounts.connection, "CREATE TRIGGER refuse_plans BEFORE INSERT ON subscriptions "
                                "BEGIN SELECT RAISE(ABORT, 'no plans today'); END");
    bool created = signUp(accounts.auth, "Zara Malik", "zara@example.com");
    check(counter, "when the Free plan cannot be started, sign-up fails", !created);
    check(counter, "and nothing of the account is left: no user, codes, settings or events",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM users") == 0
              && numberFromSql(accounts.connection, "SELECT COUNT(*) FROM recovery_codes") == 0
              && numberFromSql(accounts.connection, "SELECT COUNT(*) FROM user_settings") == 0
              && numberFromSql(accounts.connection, "SELECT COUNT(*) FROM audit_events "
                                                    "WHERE event_type = 'account_created'") == 0);
    runSql(accounts.connection, "DROP TRIGGER refuse_plans");
    check(counter, "once plans can be stored again, the same sign-up works",
          signUp(accounts.auth, "Zara Malik", "zara@example.com"), accounts.auth.lastError());
}

// Builds a real version 2 file: accounts, sessions, recovery codes,
// settings, transactions and budgets made by Finora's own classes, then
// the tables of versions 3 and 4 removed again. Returns false on any problem.
bool createVersion2File(const QString &folder, const QString &name, const PasswordHasher &quick)
{
    {
        TestAccounts accounts(name, folder, quick);
        if (!signUp(accounts.auth, "Amna Siddiqui", "amna@example.com")) {
            return false;
        }
        Transaction food = makeExpense("Food", 250000, "2026-10-05");
        Transaction rent = makeExpense("Rent", 4500000, "2026-10-01");
        Budget foodBudget = makeBudget("Food", 2000000);
        Budget travelBudget = makeBudget("Transport", 800000);
        bool saved = accounts.database.addTransaction(food) && accounts.database.addTransaction(rent)
                     && accounts.budgets.addBudget(foodBudget) && accounts.budgets.addBudget(travelBudget);
        accounts.auth.signOut("signed_out");

        if (!saved || !signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com")) {
            return false;
        }
        Transaction books = makeExpense("Education", 120000, "2026-10-12");
        saved = accounts.database.addTransaction(books);
        accounts.auth.signOut("signed_out");
        if (!saved || !signIn(accounts.auth, "amna@example.com")) {
            return false;
        }
        accounts.auth.signOut("signed_out");
    }   // the DatabaseManager closes the file here

    // A record from before accounts existed, then back to version 2.
    return runSqlOnFile(folder + "/" + name + ".db", {
        "INSERT INTO transactions (user_id, type, amount_minor, currency_code, category, description, "
        "transaction_date) VALUES (NULL, 'income', 9000000, 'PKR', 'Salary', 'Before accounts', '2026-09-30')",
        "DROP TABLE goal_contributions",
        "DROP TABLE goals",
        "DROP TABLE payments",
        "DROP TABLE subscriptions",
        "DELETE FROM audit_events WHERE event_type = 'plan_started'",
        "PRAGMA user_version = 2"
    });
}

void testUpgradeFromVersion2(TestCounter &counter, const QString &parentFolder, const PasswordHasher &quick)
{
    QString folder = parentFolder + "/upgrade_v2";
    QDir().mkpath(folder);
    QString path = folder + "/billing_upgrade.db";
    bool prepared = createVersion2File(folder, "billing_upgrade", quick);
    check(counter, "a version 2 file with 2 accounts, records and a session history is prepared",
          prepared && DatabaseManager::storedSchemaVersion(path) == 2
              && numberFromFile(path, "SELECT COUNT(*) FROM users") == 2
              && numberFromFile(path, "SELECT COUNT(*) FROM transactions") == 4);
    if (!prepared) {
        return;
    }

    QStringList before = snapshotsOfFile(path, VERSION_2_TABLES);
    long long lastEventBefore = numberFromFile(path, "SELECT MAX(id) FROM audit_events");

    TestAccounts accounts("billing_upgrade", folder, quick);   // opens and upgrades the file
    check(counter, "open() upgrades the version 2 file to version 4 (through version 3)",
          accounts.database.isOpen() && accounts.database.schemaVersion() == 4,
          accounts.database.lastError());

    // Every table of version 2 must be exactly as before, row by row.
    bool allSame = true;
    for (int i = 0; i < VERSION_2_TABLES.size(); i++) {
        QString condition = "1 = 1";
        if (VERSION_2_TABLES[i] == "audit_events") {
            condition = "id <= " + QString::number(lastEventBefore);   // the upgrade adds one event
        }
        if (tableSnapshot(accounts.connection, VERSION_2_TABLES[i], condition) != before[i]) {
            allSame = false;
            std::cout << "        changed table: " << VERSION_2_TABLES[i].toStdString() << "\n";
        }
    }
    check(counter, "every row of the 9 older tables is unchanged after the upgrade", allSame);
    check(counter, "including every password hash and recovery-code hash",
          !before.isEmpty() && before[VERSION_2_TABLES.indexOf("users")].contains("$argon2id$")
              && before[VERSION_2_TABLES.indexOf("recovery_codes")].contains("$argon2id$"));

    check(counter, "each of the 2 accounts received exactly one Free subscription",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM subscriptions WHERE plan = 'free' "
                                             "AND status = 'free'") == 2
              && numberFromSql(accounts.connection, "SELECT COUNT(DISTINCT user_id) FROM subscriptions") == 2);
    check(counter, "Free counts from the day each account was created",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM subscriptions s JOIN users u "
                                             "ON u.id = s.user_id WHERE s.started_at = u.created_at") == 2);
    check(counter, "the upgrade creates no payment", numberFromSql(accounts.connection,
                                                                   "SELECT COUNT(*) FROM payments") == 0);
    check(counter, "the upgrade is written to the audit history as 2 to 4",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM audit_events WHERE event_type = "
                                             "'database_upgraded' AND details LIKE 'Schema version 2 to 4%'") == 1);
    check(counter, "every foreign key still points to an existing row",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM pragma_foreign_key_check") == 0);

    QString backup = accounts.database.lastBackupPath();
    check(counter, "a verified backup of the version 2 file was saved first",
          !backup.isEmpty() && DatabaseManager::storedSchemaVersion(backup) == 2
              && numberFromFile(backup, "SELECT COUNT(*) FROM users") == 2
              && numberFromFile(backup, "SELECT COUNT(*) FROM transactions") == 4,
          backup);
    check(counter, "the backup holds the same password hashes",
          snapshotsOfFile(backup, {"users"}) == QStringList{before[VERSION_2_TABLES.indexOf("users")]});

    // The real proof: the people can still sign in and see their records.
    bool amnaIn = signIn(accounts.auth, "amna@example.com");
    std::vector<Transaction> rows;
    accounts.database.loadTransactions(rows);
    Subscription plan;
    accounts.subscriptions.loadSubscription(plan);
    check(counter, "after the upgrade Amna signs in with her old password",
          amnaIn, accounts.auth.lastError());
    check(counter, "she sees her own 2 transactions and is on Finora Free",
          rows.size() == 2 && plan.plan == PLAN_FREE && plan.status == STATUS_FREE);
    accounts.auth.signOut("signed_out");
    check(counter, "the record from before accounts still belongs to no account",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM transactions WHERE user_id IS NULL") == 1);

    accounts.database.close();
    bool reopened = accounts.database.open(path);
    check(counter, "opening the upgraded file again makes no second backup",
          reopened && accounts.database.lastBackupPath().isEmpty()
              && QDir(folder + "/backups").entryList(QDir::Files).size() == 1);
}

void testUpgradeRollback(TestCounter &counter, const QString &parentFolder, const PasswordHasher &quick)
{
    // A version 2 file in which a table called "payments" already exists
    // (made by another tool). The upgrade creates "subscriptions" first and
    // then fails on "payments", so the rollback must undo both.
    QString folder = parentFolder + "/upgrade_v2_conflict";
    QDir().mkpath(folder);
    QString path = folder + "/billing_conflict.db";
    bool prepared = createVersion2File(folder, "billing_conflict", quick)
                    && runSqlOnFile(path, {"CREATE TABLE payments (note TEXT)"});
    check(counter, "a version 2 file with a conflicting \"payments\" table is prepared", prepared);

    QByteArray bytesBefore = readFile(path);
    {
        DatabaseManager database("billing_conflict_open");
        bool opened = database.open(path);
        check(counter, "the upgrade is refused", !opened);
        std::cout << "        message: " << database.lastError().toStdString() << "\n";
        check(counter, "the message says nothing was changed and where the backup is",
              database.lastError().contains("nothing in it was changed") && database.lastError().contains("backup"));
        check(counter, "the backup made before the attempt is kept",
              QFile::exists(database.lastBackupPath()));
    }
    check(counter, "after the rollback the file is still version 2", DatabaseManager::storedSchemaVersion(path) == 2);
    check(counter, "after the rollback the file is byte-for-byte as before", readFile(path) == bytesBefore);
    check(counter, "no subscriptions table was left behind",
          numberFromFile(path, "SELECT COUNT(*) FROM sqlite_master WHERE name = 'subscriptions'") == 0);
    check(counter, "both accounts and their password hashes are still there",
          numberFromFile(path, "SELECT COUNT(*) FROM users WHERE password_hash LIKE '$argon2id$%'") == 2);
}

// SQLite's own rules refuse bad subscription and payment rows, even
// from SQL that bypasses Finora's C++ code.
void testDatabaseRules(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_rules", folder, quick);
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    accounts.auth.signOut("signed_out");
    signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com");
    accounts.auth.signOut("signed_out");
    const QString &c = accounts.connection;
    QString amna = "(SELECT id FROM users WHERE email = 'amna@example.com')";
    QString bilal = "(SELECT id FROM users WHERE email = 'bilal@example.com')";
    QString amnaPlan = "(SELECT id FROM subscriptions WHERE user_id = " + amna + ")";

    check(counter, "an unknown plan is refused",
          !runSql(c, "UPDATE subscriptions SET plan = 'gold' WHERE user_id = " + amna));
    check(counter, "an active status on the Free plan is refused (plan and status must agree)",
          !runSql(c, "UPDATE subscriptions SET status = 'active' WHERE user_id = " + amna));
    check(counter, "an active paid plan without a renewal date is refused",
          !runSql(c, "UPDATE subscriptions SET plan = 'pro', status = 'active', expires_at = '2026-12-07T00:00:00Z' "
                     "WHERE user_id = " + amna));
    check(counter, "a trial without an end time is refused",
          !runSql(c, "UPDATE subscriptions SET plan = 'pro', status = 'trial' WHERE user_id = " + amna));
    check(counter, "a second subscription for the same account is refused",
          !runSql(c, "INSERT INTO subscriptions (user_id, plan, status) VALUES (" + amna + ", 'free', 'free')"));
    check(counter, "Amna's subscription is still Free after all these attempts",
          textFromSql(c, "SELECT plan || '/' || status FROM subscriptions WHERE user_id = " + amna) == "free/free");

    QString columns = "INSERT INTO payments (user_id, subscription_id, plan, reason, amount_minor, status, "
                      "provider, reference) VALUES (";
    check(counter, "a payment of zero is refused",
          !runSql(c, columns + amna + ", " + amnaPlan + ", 'plus', 'new', 0, 'pending', 'simulated', 'SIM-TEST00000001')"));
    check(counter, "an unknown payment provider is refused",
          !runSql(c, columns + amna + ", " + amnaPlan + ", 'plus', 'new', 599, 'pending', 'stripe', 'SIM-TEST00000002')"));
    check(counter, "a succeeded payment without its paid period is refused",
          !runSql(c, columns + amna + ", " + amnaPlan + ", 'plus', 'new', 599, 'succeeded', 'simulated', "
                     "'SIM-TEST00000003')"));
    check(counter, "a payment of the Free plan is refused",
          !runSql(c, columns + amna + ", " + amnaPlan + ", 'free', 'new', 599, 'pending', 'simulated', 'SIM-TEST00000004')"));
    check(counter, "a payment of Bilal pointing to Amna's subscription is refused (two-column foreign key)",
          !runSql(c, columns + bilal + ", " + amnaPlan + ", 'plus', 'new', 599, 'pending', 'simulated', "
                     "'SIM-TEST00000005')"));
    bool validSaved = runSql(c, columns + amna + ", " + amnaPlan + ", 'plus', 'new', 599, 'pending', 'simulated', "
                                "'SIM-TEST00000006')");
    check(counter, "a correct pending payment is accepted, so the rules are not simply refusing everything",
          validSaved);
    check(counter, "a second payment with the same reference is refused",
          !runSql(c, columns + amna + ", " + amnaPlan + ", 'plus', 'new', 599, 'pending', 'simulated', "
                     "'SIM-TEST00000006')"));
    runSql(c, "DELETE FROM payments");
}

// ---- 4C.2: entitlements -------------------------------------------------

// "NULL" for an empty value, otherwise the value in quotes (test values only).
QString sqlValue(const QString &value)
{
    if (value.isEmpty()) {
        return "NULL";
    }
    return "'" + value + "'";
}

// Puts a subscription row into a given state directly in SQL. The real
// program changes plans only through a paid checkout (stage 4C.4); the
// tests use this to reach every state quickly.
bool setPlanDirectly(const QString &connection, const QString &email, const QString &plan,
                     const QString &status, const QString &renewsAt, const QString &expiresAt)
{
    return runSql(connection, "UPDATE subscriptions SET plan = '" + plan + "', status = '" + status
                                  + "', renews_at = " + sqlValue(renewsAt) + ", expires_at = " + sqlValue(expiresAt)
                                  + " WHERE user_id = (SELECT id FROM users WHERE email = '" + email + "')");
}

const QString FAR_FUTURE = "2099-01-01T00:00:00Z";
const QString LONG_AGO = "2000-01-01T00:00:00Z";

void testCurrentPlan(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_plan", folder, quick);
    signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com");
    accounts.auth.signOut("signed_out");
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    DatabaseManager &database = accounts.database;

    check(counter, "a new account is on Finora Free", database.currentPlan() == PLAN_FREE);
    check(counter, "Free may use its own features", database.requireFeature(FEATURE_TRANSACTIONS));
    bool refused = !database.requireFeature(FEATURE_UNLIMITED_BUDGETS);
    check(counter, "Free may not use a Plus feature, and the message names Finora Plus",
          refused && database.lastError().contains("is part of Finora Plus"), database.lastError());

    setPlanDirectly(accounts.connection, "amna@example.com", "plus", "active", FAR_FUTURE, FAR_FUTURE);
    check(counter, "an active Plus row gives Plus", database.currentPlan() == PLAN_PLUS);
    check(counter, "Plus may use its features", database.requireFeature(FEATURE_UNLIMITED_BUDGETS));
    check(counter, "a planned feature is refused even when the plan includes it",
          !database.requireFeature(FEATURE_RECURRING) && database.lastError().contains("planned"));

    setPlanDirectly(accounts.connection, "amna@example.com", "plus", "active", LONG_AGO, FAR_FUTURE);
    check(counter, "after a missed renewal, Plus stays during the grace period", database.currentPlan() == PLAN_PLUS);
    setPlanDirectly(accounts.connection, "amna@example.com", "plus", "active", LONG_AGO, LONG_AGO);
    check(counter, "when the grace has ended, Free applies even though the row still says active",
          database.currentPlan() == PLAN_FREE);

    setPlanDirectly(accounts.connection, "amna@example.com", "business", "active", FAR_FUTURE, FAR_FUTURE);
    accounts.auth.signOut("signed_out");
    check(counter, "while nobody is signed in, the plan is Free", database.currentPlan() == PLAN_FREE);
    signIn(accounts.auth, "bilal@example.com");
    check(counter, "Bilal stays on Free while Amna has Business: plans are per account",
          database.currentPlan() == PLAN_FREE && !database.requireFeature(FEATURE_UNLIMITED_BUDGETS));
    accounts.auth.signOut("signed_out");

    signIn(accounts.auth, "amna@example.com");
    runSql(accounts.connection, "DELETE FROM subscriptions WHERE user_id = (SELECT id FROM users "
                                "WHERE email = 'amna@example.com')");
    check(counter, "if the subscription row is missing, Finora falls back to Free (fails closed)",
          database.currentPlan() == PLAN_FREE);
}

// The category of every budget, with "(paused)" after the paused ones.
QString describeBudgets(const std::vector<Budget> &budgets)
{
    QStringList parts;
    for (int i = 0; i < static_cast<int>(budgets.size()); i++) {
        QString part = QString::fromStdString(budgets[i].category);
        if (budgets[i].paused) {
            part += " (paused)";
        }
        parts.append(part);
    }
    return parts.join(", ");
}

void testBudgetLimit(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_budgets", folder, quick);
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    BudgetStore &budgets = accounts.budgets;

    Budget food = makeBudget("Food", 2000000);
    Budget rent = makeBudget("Rent", 5000000);
    Budget transport = makeBudget("Transport", 800000);
    bool firstThree = budgets.addBudget(food) && budgets.addBudget(rent) && budgets.addBudget(transport);
    check(counter, "Free: the first 3 budgets are saved", firstThree, budgets.lastError());

    Budget health = makeBudget("Health", 300000);
    bool fourthSaved = budgets.addBudget(health);
    check(counter, "Free: a fourth budget is refused by BudgetStore itself, below every screen",
          !fourthSaved && budgets.planLimitReached());
    check(counter, "the refusal explains the plans",
          budgets.lastError() == QString::fromStdString(budgetLimitMessage(PLAN_FREE)), budgets.lastError());
    check(counter, "and nothing was stored", numberFromSql(accounts.connection, "SELECT COUNT(*) FROM budgets") == 3);

    Budget secondFood = makeBudget("Food", 100000);
    check(counter, "a duplicate still gets its own message first, not the plan message",
          !budgets.addBudget(secondFood) && !budgets.planLimitReached() && budgets.lastError().contains("already"));

    setPlanDirectly(accounts.connection, "amna@example.com", "plus", "active", FAR_FUTURE, FAR_FUTURE);
    Budget shopping = makeBudget("Shopping", 400000);
    check(counter, "Plus: a fourth and a fifth budget are saved",
          budgets.addBudget(health) && budgets.addBudget(shopping), budgets.lastError());
    std::vector<Budget> loaded;
    budgets.loadBudgets(loaded);
    check(counter, "Plus: all 5 budgets are active", loaded.size() == 5 && countPausedBudgets(loaded) == 0,
          describeBudgets(loaded));

    // Back to Free (for example when Plus expires): nothing is deleted.
    setPlanDirectly(accounts.connection, "amna@example.com", "free", "expired", "", LONG_AGO);
    budgets.loadBudgets(loaded);
    check(counter, "back on Free: all 5 budgets are still there, the 2 newest are paused",
          loaded.size() == 5 && countPausedBudgets(loaded) == 2
              && describeBudgets(loaded) == "Food, Health (paused), Rent, Shopping (paused), Transport",
          describeBudgets(loaded));
    check(counter, "no budget row was deleted", numberFromSql(accounts.connection, "SELECT COUNT(*) FROM budgets") == 5);
    Budget education = makeBudget("Education", 100000);
    check(counter, "with 5 budgets on Free, a new one is refused", !budgets.addBudget(education));

    health.limitMinor = 350000;
    check(counter, "a paused budget can still be edited", budgets.updateBudget(health), budgets.lastError());
    check(counter, "deleting an active budget makes room: the next oldest becomes active",
          budgets.deleteBudget(food.id) && budgets.loadBudgets(loaded)
              && describeBudgets(loaded) == "Health, Rent, Shopping (paused), Transport",
          describeBudgets(loaded));
    accounts.auth.signOut("signed_out");

    // Every person has their own limit.
    signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com");
    Budget bilalFood = makeBudget("Food", 1500000);
    Budget bilalRent = makeBudget("Rent", 3000000);
    Budget bilalBills = makeBudget("Bills & Utilities", 900000);
    check(counter, "Bilal has his own 3 budgets on Free, whatever Amna has",
          budgets.addBudget(bilalFood) && budgets.addBudget(bilalRent) && budgets.addBudget(bilalBills),
          budgets.lastError());
    accounts.auth.signOut("signed_out");

    // Signed out, no budget can be added at all, so signing out cannot
    // be used to get around the limit.
    Budget gifts = makeBudget("Gifts", 100);
    check(counter, "signed out, no budget can be added at all",
          !budgets.addBudget(gifts) && budgets.lastError() == "Please sign in first.");

    // Without a Session there are no accounts and no plans. Only the tests
    // of Milestones 2 to 4A run like this (Finora always attaches one), and
    // they must keep working exactly as in Phase 4A.
    DatabaseManager noSession("billing_budgets_no_session");
    noSession.open(folder + "/billing_budgets_no_session.db");
    BudgetStore oldStore(noSession);
    Budget a = makeBudget("Food", 100);
    Budget b = makeBudget("Rent", 100);
    Budget c = makeBudget("Health", 100);
    Budget d = makeBudget("Shopping", 100);
    bool fourSaved = oldStore.addBudget(a) && oldStore.addBudget(b) && oldStore.addBudget(c) && oldStore.addBudget(d);
    std::vector<Budget> oldBudgets;
    oldStore.loadBudgets(oldBudgets);
    check(counter, "without a Session (the Phase 4A tests), budgets work as in Phase 4A: no plan limit",
          fourSaved && oldBudgets.size() == 4 && countPausedBudgets(oldBudgets) == 0, oldStore.lastError());
}

void testClaimOverTheLimit(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    // 3 budgets from before accounts existed, then Zara with 2 budgets of
    // her own moves them into her account on Free: 5 budgets, 2 paused.
    QString path = folder + "/billing_claim.db";
    {
        DatabaseManager oldDatabase("billing_claim_old");
        oldDatabase.open(path);
        BudgetStore oldBudgets(oldDatabase);
        Budget a = makeBudget("Food", 100000);
        Budget b = makeBudget("Rent", 200000);
        Budget c = makeBudget("Health", 300000);
        oldBudgets.addBudget(a);
        oldBudgets.addBudget(b);
        oldBudgets.addBudget(c);
    }
    TestAccounts accounts("billing_claim", folder, quick);
    signUp(accounts.auth, "Zara Malik", "zara@example.com");
    Budget shopping = makeBudget("Shopping", 400000);
    Budget transport = makeBudget("Transport", 500000);
    accounts.budgets.addBudget(shopping);
    accounts.budgets.addBudget(transport);

    int movedTransactions = 0;
    int movedBudgets = 0;
    int keptBudgets = 0;
    bool claimed = accounts.auth.claimRecordsWithoutOwner(movedTransactions, movedBudgets, keptBudgets);
    std::vector<Budget> loaded;
    accounts.budgets.loadBudgets(loaded);
    check(counter, "moving older budgets into a Free account works: 5 budgets, 2 of them paused",
          claimed && movedBudgets == 3 && loaded.size() == 5 && countPausedBudgets(loaded) == 2,
          describeBudgets(loaded));
}

// ---- 4C.4: the simulated checkout and the lifecycle ----------------------

void testSimulatedGateway(TestCounter &counter)
{
    SimulatedPaymentGateway gateway;
    PaymentGateway &provider = gateway;   // used through the abstract class, as Finora does

    check(counter, "the simulated gateway says that it is simulated and moves no real money",
          provider.isSimulated() && provider.providerName().find("no real money") != std::string::npos);
    std::string first = provider.newReference();
    std::string second = provider.newReference();
    bool wellFormed = first.size() == 16 && first.compare(0, 4, "SIM-") == 0;
    for (int i = 4; wellFormed && i < 16; i++) {
        wellFormed = RECOVERY_CODE_ALPHABET.find(first[i]) != std::string::npos;
    }
    check(counter, "a reference is SIM- and 12 characters without 0, O, 1 or I",
          wellFormed, QString::fromStdString(first));
    check(counter, "two references differ", first != second);

    PaymentRequest request;
    request.reference = first;
    request.description = "Finora Plus, 1 month";
    request.amountMinor = 599;
    request.currencyCode = "USD";
    check(counter, "a complete request is approved", provider.charge(request).succeeded);
    request.simulateDecline = true;
    PaymentResult declined = provider.charge(request);
    check(counter, "a simulated decline is refused and says that no money was moved",
          !declined.succeeded && declined.message.find("No money was moved") != std::string::npos);
    request.simulateDecline = false;
    request.amountMinor = 0;
    check(counter, "a request without an amount is refused", !provider.charge(request).succeeded);
    check(counter, "the gateway counted 3 requests", gateway.chargeCount() == 3);
}

void testPayingForPlans(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_pay", folder, quick);
    SimulatedPaymentGateway gateway;
    SubscriptionService service(accounts.database, gateway);
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    const QString &c = accounts.connection;

    std::string reference;
    bool paid = service.payForPlan(PLAN_PLUS, true, reference);
    check(counter, "a declined payment does not switch on Plus, and says so",
          !paid && accounts.database.currentPlan() == PLAN_FREE && service.lastError().contains("Your plan has not changed"),
          service.lastError());
    check(counter, "the declined attempt is in the billing history as failed, with its reason",
          numberFromSql(c, "SELECT COUNT(*) FROM payments WHERE status = 'failed' "
                           "AND failure_reason LIKE '%declined%'") == 1);
    check(counter, "the subscription remembers that the last payment failed",
          textFromSql(c, "SELECT last_payment_status FROM subscriptions") == "failed");

    paid = service.payForPlan(PLAN_PLUS, false, reference);
    check(counter, "a confirmed simulated payment switches on Plus",
          paid && accounts.database.currentPlan() == PLAN_PLUS, service.lastError());
    check(counter, "the payment is recorded: succeeded, 599 cents, USD, simulated, reason new, with its reference",
          numberFromSql(c, "SELECT COUNT(*) FROM payments WHERE status = 'succeeded' AND amount_minor = 599 "
                           "AND currency_code = 'USD' AND provider = 'simulated' AND reason = 'new' "
                           "AND reference = '" + QString::fromStdString(reference) + "'") == 1);
    check(counter, "the subscription is active and remembers that payment's reference",
          textFromSql(c, "SELECT plan || '/' || status || '/' || last_payment_reference FROM subscriptions")
              == "plus/active/" + QString::fromStdString(reference));
    check(counter, "it renews one month later, and without renewal the access lasts 7 more days",
          numberFromSql(c, "SELECT COUNT(*) FROM subscriptions WHERE "
                           "julianday(renews_at) - julianday(started_at) BETWEEN 28 AND 31 "
                           "AND julianday(expires_at) - julianday(renews_at) = 7") == 1);
    check(counter, "the activation is in the audit history",
          numberFromSql(c, "SELECT COUNT(*) FROM audit_events WHERE event_type = 'plan_activated'") == 1);
    check(counter, "paying again for the plan in use is refused: there is nothing to pay",
          !service.payForPlan(PLAN_PLUS, false, reference));

    paid = service.payForPlan(PLAN_PRO, false, reference);
    check(counter, "an upgrade to Pro is paid at Pro's full price, with the reason upgrade",
          paid && accounts.database.currentPlan() == PLAN_PRO
              && numberFromSql(c, "SELECT COUNT(*) FROM payments WHERE reason = 'upgrade' AND amount_minor = 1299 "
                                  "AND status = 'succeeded'") == 1);
    std::vector<PaymentRecord> history;
    accounts.subscriptions.loadPayments(history);
    check(counter, "the billing history has the 3 attempts, newest first",
          history.size() == 3 && history[0].plan == PLAN_PRO && history[2].status == "failed");
    check(counter, "every attempt went to the gateway exactly once", gateway.chargeCount() == 3);
    accounts.auth.signOut("signed_out");

    signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com");
    accounts.subscriptions.loadPayments(history);
    check(counter, "Bilal sees none of Amna's payments, and is still on Free",
          history.empty() && accounts.database.currentPlan() == PLAN_FREE);
    accounts.auth.signOut("signed_out");
    check(counter, "signed out, no payment can be made",
          !service.payForPlan(PLAN_PLUS, false, reference) && service.lastError() == "Please sign in first.");
}

void testSmallerPlanCancelAndResume(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_changes", folder, quick);
    SimulatedPaymentGateway gateway;
    SubscriptionService service(accounts.database, gateway);
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    const QString &c = accounts.connection;
    std::string reference;
    service.payForPlan(PLAN_PRO, false, reference);

    check(counter, "Pro to Plus is arranged for the renewal date, and nothing is charged",
          service.switchAtRenewal(PLAN_PLUS) && numberFromSql(c, "SELECT COUNT(*) FROM payments") == 1
              && textFromSql(c, "SELECT scheduled_plan FROM subscriptions") == "plus", service.lastError());
    check(counter, "until the renewal date, Pro's features stay", accounts.database.currentPlan() == PLAN_PRO);
    check(counter, "paying for Plus now is refused: it is a change for the renewal date",
          !service.payForPlan(PLAN_PLUS, false, reference));
    check(counter, "'Keep Pro' withdraws the smaller plan",
          service.keepCurrentPlan() && textFromSql(c, "SELECT ifnull(scheduled_plan, 'none') FROM subscriptions") == "none");

    QString renewsBefore = textFromSql(c, "SELECT renews_at FROM subscriptions");
    check(counter, "cancelling keeps Pro until the end of the paid period",
          service.cancel() && textFromSql(c, "SELECT status FROM subscriptions") == "cancelled"
              && textFromSql(c, "SELECT expires_at FROM subscriptions") == renewsBefore
              && accounts.database.currentPlan() == PLAN_PRO, service.lastError());
    check(counter, "cancelling a second time is refused", !service.cancel());
    check(counter, "resuming continues Pro with the same renewal date and no new payment",
          service.keepCurrentPlan() && textFromSql(c, "SELECT status FROM subscriptions") == "active"
              && textFromSql(c, "SELECT renews_at FROM subscriptions") == renewsBefore
              && numberFromSql(c, "SELECT COUNT(*) FROM payments") == 1);

    service.cancel();
    check(counter, "choosing Plus after cancelling resumes Pro and arranges Plus for the renewal date",
          service.switchAtRenewal(PLAN_PLUS) && textFromSql(c, "SELECT status || '/' || scheduled_plan FROM subscriptions")
                                                     == "active/plus", service.lastError());
    check(counter, "every step is in the audit history",
          numberFromSql(c, "SELECT COUNT(*) FROM audit_events WHERE event_type IN ('smaller_plan_scheduled', "
                           "'smaller_plan_withdrawn', 'subscription_cancelled', 'subscription_resumed')") == 7);
}

void testTimeAndRenewal(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_time", folder, quick);
    SimulatedPaymentGateway gateway;
    SubscriptionService normal(accounts.database, gateway, false);
    SubscriptionService demo(accounts.database, gateway, true);   // as with Finora.exe --billing-demo
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    const QString &c = accounts.connection;
    std::string reference;
    normal.payForPlan(PLAN_PLUS, false, reference);

    check(counter, "outside the demonstration mode, a period cannot be made to end",
          !normal.simulatePeriodEnd() && normal.lastError().contains("--billing-demo"));

    const char *categories[5] = {"Food", "Rent", "Health", "Shopping", "Transport"};
    bool allSaved = true;
    for (int i = 0; i < 5; i++) {
        Budget budget = makeBudget(categories[i], 100000);
        allSaved = allSaved && accounts.budgets.addBudget(budget);
    }
    check(counter, "on Plus, 5 budgets are saved", allSaved);

    check(counter, "the demonstration makes the renewal date arrive: Past due",
          demo.simulatePeriodEnd() && textFromSql(c, "SELECT status FROM subscriptions") == "past_due",
          demo.lastError());
    std::vector<Budget> loaded;
    accounts.budgets.loadBudgets(loaded);
    check(counter, "during the 7 days of grace, Plus still works: 5 budgets, none paused",
          accounts.database.currentPlan() == PLAN_PLUS && loaded.size() == 5 && countPausedBudgets(loaded) == 0);
    check(counter, "the missed renewal is in the audit history",
          numberFromSql(c, "SELECT COUNT(*) FROM audit_events WHERE event_type = 'renewal_due'") == 1);

    check(counter, "paying the renewal makes Plus active again, with the reason renewal",
          normal.payForPlan(PLAN_PLUS, false, reference) && textFromSql(c, "SELECT status FROM subscriptions") == "active"
              && numberFromSql(c, "SELECT COUNT(*) FROM payments WHERE reason = 'renewal' AND status = 'succeeded'") == 1,
          normal.lastError());

    demo.simulatePeriodEnd();   // the renewal date arrives: Past due
    demo.simulatePeriodEnd();   // the grace ends without payment
    check(counter, "when the grace ends unpaid, the plan expires and Finora Free applies",
          textFromSql(c, "SELECT plan || '/' || status FROM subscriptions") == "free/expired"
              && accounts.database.currentPlan() == PLAN_FREE);
    accounts.budgets.loadBudgets(loaded);
    check(counter, "nothing is deleted: all 5 budgets stay, the 2 newest are paused",
          loaded.size() == 5 && countPausedBudgets(loaded) == 2
              && numberFromSql(c, "SELECT COUNT(*) FROM budgets") == 5);
    check(counter, "the end is in the audit history",
          numberFromSql(c, "SELECT COUNT(*) FROM audit_events WHERE event_type = 'subscription_expired'") == 1);
    check(counter, "an ended plan cannot be ended again", !demo.simulatePeriodEnd());

    check(counter, "a new payment after the end starts Plus again, and every budget is active again",
          normal.payForPlan(PLAN_PLUS, false, reference) && accounts.budgets.loadBudgets(loaded)
              && countPausedBudgets(loaded) == 0);
    demo.simulatePeriodEnd();
    check(counter, "'Stop now' while past due ends the plan at once",
          normal.cancel() && textFromSql(c, "SELECT plan || '/' || status FROM subscriptions") == "free/expired");
}

void testTrial(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_trial", folder, quick);
    SimulatedPaymentGateway gateway;
    SubscriptionService service(accounts.database, gateway, true);
    const QString &c = accounts.connection;

    signUp(accounts.auth, "Zara Malik", "zara@example.com");
    check(counter, "the free Pro trial starts without any payment",
          service.startTrial() && textFromSql(c, "SELECT plan || '/' || status FROM subscriptions") == "pro/trial"
              && numberFromSql(c, "SELECT COUNT(*) FROM payments") == 0 && accounts.database.currentPlan() == PLAN_PRO,
          service.lastError());
    check(counter, "the trial lasts exactly 14 days",
          numberFromSql(c, "SELECT julianday(expires_at) - julianday(started_at) FROM subscriptions") == 14);
    check(counter, "a second trial is refused while the first one runs", !service.startTrial());
    service.simulatePeriodEnd();
    check(counter, "when the trial ends, Finora Free applies and nothing was charged",
          textFromSql(c, "SELECT plan || '/' || status FROM subscriptions") == "free/expired"
              && numberFromSql(c, "SELECT COUNT(*) FROM payments") == 0);
    check(counter, "the trial can never start again for this account",
          !service.startTrial() && numberFromSql(c, "SELECT trial_used FROM subscriptions") == 1);
    accounts.auth.signOut("signed_out");

    signUp(accounts.auth, "Omar Farooq", "omar@example.com");
    service.startTrial();
    std::string reference;
    check(counter, "during the trial, paying for Pro keeps Pro as a new subscription",
          service.payForPlan(PLAN_PRO, false, reference)
              && textFromSql(c, "SELECT plan || '/' || status FROM subscriptions WHERE user_id = "
                                "(SELECT id FROM users WHERE email = 'omar@example.com')") == "pro/active"
              && numberFromSql(c, "SELECT COUNT(*) FROM payments WHERE reason = 'new' AND plan = 'pro'") == 1);
    accounts.auth.signOut("signed_out");

    signUp(accounts.auth, "Hina Shah", "hina@example.com");
    service.startTrial();
    check(counter, "'End the trial' goes back to Free at once",
          service.cancel() && accounts.database.currentPlan() == PLAN_FREE);
}

void testInterruptedPayment(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_crash", folder, quick);
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    const QString &c = accounts.connection;
    // A pending payment, as a crash in the middle of a payment would leave it.
    runSql(c, "INSERT INTO payments (user_id, subscription_id, plan, reason, amount_minor, status, provider, reference) "
              "SELECT user_id, id, 'pro', 'new', 1299, 'pending', 'simulated', 'SIM-CRASHTEST001' FROM subscriptions");

    SimulatedPaymentGateway gateway;
    SubscriptionService nextStart(accounts.database, gateway);   // like the next start of Finora
    check(counter, "at the next start, an interrupted payment is marked as failed, with the reason",
          textFromSql(c, "SELECT status FROM payments WHERE reference = 'SIM-CRASHTEST001'") == "failed"
              && numberFromSql(c, "SELECT COUNT(*) FROM payments WHERE failure_reason LIKE 'Finora closed%'") == 1);
    check(counter, "and it switched on no plan", accounts.database.currentPlan() == PLAN_FREE);
}

// A gateway for this test only. It answers "succeeded", but first
// changes the pending payment behind Finora's back, as a bug or an
// attacker might. Finora must still refuse to switch the plan on.
class TamperingGateway : public SimulatedPaymentGateway
{
public:
    QString connection;
    QString tamperingSql;

    PaymentResult charge(const PaymentRequest &request) override
    {
        runSql(connection, tamperingSql);
        return SimulatedPaymentGateway::charge(request);
    }
};

void testActivationGuard(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_guard", folder, quick);
    signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com");
    accounts.auth.signOut("signed_out");
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    const QString &c = accounts.connection;
    QString bilal = "(SELECT id FROM users WHERE email = 'bilal@example.com')";

    TamperingGateway gateway;
    gateway.connection = c;
    SubscriptionService service(accounts.database, gateway);
    std::string reference;

    // 1. The payment is moved to another person while it is being paid.
    gateway.tamperingSql = "UPDATE payments SET user_id = " + bilal + ", subscription_id = (SELECT id FROM "
                           "subscriptions WHERE user_id = " + bilal + ") WHERE status = 'pending'";
    bool paid = service.payForPlan(PLAN_BUSINESS, false, reference);
    check(counter, "a payment that no longer belongs to Amna does not switch on her plan",
          !paid && accounts.database.currentPlan() == PLAN_FREE, service.lastError());
    check(counter, "and Bilal's plan did not change either",
          textFromSql(c, "SELECT plan FROM subscriptions WHERE user_id = " + bilal) == "free");

    // 2. The payment's plan is changed to a cheaper one.
    gateway.tamperingSql = "UPDATE payments SET plan = 'plus' WHERE status = 'pending'";
    paid = service.payForPlan(PLAN_BUSINESS, false, reference);
    check(counter, "a payment for another plan does not switch on Business",
          !paid && accounts.database.currentPlan() == PLAN_FREE);
    check(counter, "that attempt is marked as failed, not left pending",
          numberFromSql(c, "SELECT COUNT(*) FROM payments WHERE plan = 'plus' AND status = 'failed'") == 1);

    // 3. The payment is marked as failed in the meantime.
    gateway.tamperingSql = "UPDATE payments SET status = 'failed', failure_reason = 'test', "
                           "completed_at = '2026-10-31T00:00:00Z' WHERE status = 'pending'";
    paid = service.payForPlan(PLAN_BUSINESS, false, reference);
    check(counter, "a payment that is no longer pending does not switch on Business",
          !paid && accounts.database.currentPlan() == PLAN_FREE);
    check(counter, "after all three attempts, Amna is still on Free and no payment succeeded",
          numberFromSql(c, "SELECT COUNT(*) FROM payments WHERE status = 'succeeded'") == 0
              && textFromSql(c, "SELECT plan FROM subscriptions WHERE user_id = (SELECT id FROM users WHERE "
                                "email = 'amna@example.com')") == "free");
}

// ---- 4C.5: exports and the audit history --------------------------------

bool containsEvent(const std::vector<AuditEvent> &events, const std::string &eventType)
{
    for (int i = 0; i < static_cast<int>(events.size()); i++) {
        if (events[i].eventType == eventType) {
            return true;
        }
    }
    return false;
}

void testExportsAndAuditHistory(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("billing_exports", folder, quick);
    SimulatedPaymentGateway gateway;
    SubscriptionService service(accounts.database, gateway);
    ReportExporter exporter(accounts.database);
    const QString &c = accounts.connection;

    // Bilal's record must never appear in Amna's files.
    signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com");
    Transaction bilalRent = makeExpense("Rent", 3000000, "2026-10-01");
    accounts.database.addTransaction(bilalRent);
    accounts.auth.signOut("signed_out");

    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    Transaction food = makeExpense("Food", 250000, "2026-10-05");
    Transaction gift = makeExpense("Gifts", 50000, "2026-10-06");
    gift.description = "=HYPERLINK(\"http://evil.example\",\"click\")";   // a formula attack
    Transaction salary = makeExpense("Salary", 9000000, "2026-10-01");
    salary.type = TYPE_INCOME;
    salary.description = "October salary";
    accounts.database.addTransaction(food);
    accounts.database.addTransaction(gift);
    accounts.database.addTransaction(salary);
    Budget foodBudget = makeBudget("Food", 2000000);
    accounts.budgets.addBudget(foodBudget);

    QString csvPath = folder + "/amna.csv";
    QString pdfPath = folder + "/amna.pdf";
    int rows = 0;
    std::vector<AuditEvent> events;
    check(counter, "on Free, ReportExporter itself refuses the CSV export, naming Finora Plus, and makes no file",
          !exporter.exportTransactionsCsv(csvPath, rows) && exporter.lastError().contains("Finora Plus")
              && !QFile::exists(csvPath), exporter.lastError());
    check(counter, "on Free, the PDF summary is refused, naming Finora Pro, and no file is made",
          !exporter.exportMonthlyPdf("2026-10", "Amna", pdfPath) && exporter.lastError().contains("Finora Pro")
              && !QFile::exists(pdfPath));
    check(counter, "on Free, the audit history is refused, naming Finora Business",
          !accounts.database.loadAuditHistory(events) && accounts.database.lastError().contains("Finora Business"));

    std::string reference;
    service.payForPlan(PLAN_PLUS, false, reference);
    check(counter, "on Plus, the CSV export writes Amna's 3 transactions",
          exporter.exportTransactionsCsv(csvPath, rows) && rows == 3, exporter.lastError());
    QByteArray csv = readFile(csvPath);
    check(counter, "the file starts with the UTF-8 mark and the header line",
          csv.startsWith("\xEF\xBB\xBF" "Date,Type,Category,Description,Currency,Amount\r\n"));
    check(counter, "it holds only Amna's records, never Bilal's",
          csv.contains("October salary") && csv.contains("90000.00") && !csv.contains("30000.00"));
    check(counter, "a formula in a description is made harmless with a ' in front",
          csv.contains("\"'=HYPERLINK(") && !csv.contains("\"=HYPERLINK("));

    // A failed export must leave the earlier file exactly as it was: QSaveFile
    // never writes into the real file directly.
    QFile::setPermissions(csvPath, QFileDevice::ReadOwner | QFileDevice::ReadUser);   // read-only now
    Transaction transport = makeExpense("Transport", 30000, "2026-10-08");
    accounts.database.addTransaction(transport);   // so a new export would differ
    bool exportFailed = !exporter.exportTransactionsCsv(csvPath, rows);
    QByteArray afterFailure = readFile(csvPath);
    QFile::setPermissions(csvPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                       | QFileDevice::ReadUser | QFileDevice::WriteUser);
    check(counter, "when the file cannot be replaced, the export fails and the old file stays exactly as it was",
          exportFailed && afterFailure == csv && !exporter.lastError().isEmpty(), exporter.lastError());
    check(counter, "no half-written temporary file is left next to it",
          QDir(folder).entryList(QStringList{"amna.csv*"}, QDir::Files).size() == 1);
    QString missingFolder = folder + "/no_such_folder/amna.csv";
    check(counter, "an export into a folder that does not exist fails and makes no file",
          !exporter.exportTransactionsCsv(missingFolder, rows) && !QFile::exists(missingFolder));

    check(counter, "on Plus, the PDF summary is still refused: it is part of Pro",
          !exporter.exportMonthlyPdf("2026-10", "Amna", pdfPath));

    // A USD expense in the same month: the summary must keep it apart from PKR.
    Transaction book = makeExpense("Books", 4599, "2026-10-07");
    book.currencyCode = "USD";
    accounts.database.addTransaction(book);

    service.payForPlan(PLAN_PRO, false, reference);
    check(counter, "on Pro, the PDF summary is written",
          exporter.exportMonthlyPdf("2026-10", "Amna Siddiqui", pdfPath), exporter.lastError());
    QByteArray pdf = readFile(pdfPath);
    check(counter, "the file is a real PDF (it starts with %PDF) of a sensible size",
          pdf.startsWith("%PDF") && pdf.size() > 1000, QString::number(pdf.size()) + " bytes");
    std::vector<Transaction> mine;
    std::vector<Budget> myBudgets;
    accounts.database.loadTransactions(mine);
    accounts.budgets.loadBudgets(myBudgets);
    QString html = exporter.monthlySummaryHtml("2026-10", "Amna <b>Siddiqui</b>", mine, myBudgets);
    check(counter, "the summary shows the month's income and expenses and the Food budget",
          html.contains("PKR 90,000.00") && html.contains("PKR 3,300.00") && html.contains("Food (PKR)"));
    check(counter, "PKR and USD stay in separate rows and are never added together",
          html.contains("USD 45.99") && html.contains("Category (USD)") && !html.contains("3,345.99")
              && html.contains("Amounts are never converted between currencies"));
    check(counter, "text from the records is escaped, so it cannot change the page",
          html.contains("Amna &lt;b&gt;Siddiqui&lt;/b&gt;") && !html.contains("<b>Siddiqui"));
    check(counter, "an invalid month is refused", !exporter.exportMonthlyPdf("2026-13", "Amna", pdfPath));
    check(counter, "on Pro, the audit history is still refused: it is part of Business",
          !accounts.database.loadAuditHistory(events));

    service.payForPlan(PLAN_BUSINESS, false, reference);
    check(counter, "on Business, the audit history loads",
          accounts.database.loadAuditHistory(events) && !events.empty(), accounts.database.lastError());
    check(counter, "it is newest first and shows the sign-up, the exports and the plan changes",
          !events.empty() && events[0].eventType == "plan_activated" && containsEvent(events, "account_created")
              && containsEvent(events, "csv_exported") && containsEvent(events, "pdf_summary_exported"));
    check(counter, "it holds only Amna's own lines",
          numberFromSql(c, "SELECT COUNT(*) FROM audit_events WHERE user_id = "
                           "(SELECT id FROM users WHERE email = 'amna@example.com')")
              == static_cast<long long>(events.size()));
    check(counter, "Bilal's lines are in the same table, but none of them is shown",
          numberFromSql(c, "SELECT COUNT(*) FROM audit_events WHERE user_id = "
                           "(SELECT id FROM users WHERE email = 'bilal@example.com')") > 0
              && numberFromSql(c, "SELECT COUNT(*) FROM audit_events") > static_cast<long long>(events.size()));
    accounts.auth.signOut("signed_out");
    check(counter, "signed out, nothing can be exported", !exporter.exportTransactionsCsv(csvPath, rows));
}

// ---- Milestone 5: the upgrade from version 3 to 4 ----------------------

// Builds a real version 3 file: two accounts, a transaction, a budget and
// a paid (simulated) Plus subscription made by Finora's own classes, then
// the version 4 tables removed again. Returns false on any problem.
bool createVersion3File(const QString &folder, const QString &name, const PasswordHasher &quick)
{
    {
        TestAccounts accounts(name, folder, quick);
        SimulatedPaymentGateway gateway;
        SubscriptionService service(accounts.database, gateway);
        if (!signUp(accounts.auth, "Amna Siddiqui", "amna@example.com")) {
            return false;
        }
        Transaction food = makeExpense("Food", 250000, "2026-10-05");
        Budget foodBudget = makeBudget("Food", 2000000);
        std::string reference;
        bool saved = accounts.database.addTransaction(food) && accounts.budgets.addBudget(foodBudget)
                     && service.payForPlan(PLAN_PLUS, false, reference);
        accounts.auth.signOut("signed_out");
        if (!saved || !signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com")) {
            return false;
        }
        accounts.auth.signOut("signed_out");
    }   // the DatabaseManager closes the file here

    return runSqlOnFile(folder + "/" + name + ".db", {
        "DROP TABLE goal_contributions",
        "DROP TABLE goals",
        "PRAGMA user_version = 3"
    });
}

void testUpgradeFromVersion3(TestCounter &counter, const QString &parentFolder, const PasswordHasher &quick)
{
    QString folder = parentFolder + "/upgrade_v3";
    QDir().mkpath(folder);
    QString path = folder + "/goals_upgrade.db";
    bool prepared = createVersion3File(folder, "goals_upgrade", quick);
    check(counter, "a version 3 file with 2 accounts, a paid Plus plan and its payment is prepared",
          prepared && DatabaseManager::storedSchemaVersion(path) == 3
              && numberFromFile(path, "SELECT COUNT(*) FROM payments WHERE status = 'succeeded'") == 1
              && numberFromFile(path, "SELECT COUNT(*) FROM sqlite_master WHERE name LIKE 'goal%'") == 0);
    if (!prepared) {
        return;
    }

    QStringList before = snapshotsOfFile(path, VERSION_3_TABLES);
    long long lastEventBefore = numberFromFile(path, "SELECT MAX(id) FROM audit_events");

    TestAccounts accounts("goals_upgrade", folder, quick);   // opens and upgrades the file
    const QString &c = accounts.connection;
    check(counter, "open() upgrades the version 3 file to version 4",
          accounts.database.isOpen() && accounts.database.schemaVersion() == 4, accounts.database.lastError());

    // Every table of version 3 must be exactly as before, row by row.
    bool allSame = true;
    for (int i = 0; i < VERSION_3_TABLES.size(); i++) {
        QString condition = "1 = 1";
        if (VERSION_3_TABLES[i] == "audit_events") {
            condition = "id <= " + QString::number(lastEventBefore);   // the upgrade adds one event
        }
        if (tableSnapshot(c, VERSION_3_TABLES[i], condition) != before[i]) {
            allSame = false;
            std::cout << "        changed table: " << VERSION_3_TABLES[i].toStdString() << "\n";
        }
    }
    check(counter, "every row of the 11 older tables is unchanged, including the subscription and payment",
          allSame);
    check(counter, "the two goal tables and the trigger were added, and the goal tables are empty",
          numberFromSql(c, "SELECT COUNT(*) FROM sqlite_master WHERE name IN "
                           "('goals', 'goal_contributions', 'goal_saved_amount_stays_valid')") == 3
              && numberFromSql(c, "SELECT COUNT(*) FROM goals") == 0
              && numberFromSql(c, "SELECT COUNT(*) FROM goal_contributions") == 0);
    check(counter, "the upgrade is written to the audit history as 3 to 4",
          numberFromSql(c, "SELECT COUNT(*) FROM audit_events WHERE event_type = 'database_upgraded' "
                           "AND details LIKE 'Schema version 3 to 4%'") == 1);
    check(counter, "every foreign key still points to an existing row",
          numberFromSql(c, "SELECT COUNT(*) FROM pragma_foreign_key_check") == 0);

    QString backup = accounts.database.lastBackupPath();
    check(counter, "a verified backup of the version 3 file was saved first",
          !backup.isEmpty() && DatabaseManager::storedSchemaVersion(backup) == 3
              && numberFromFile(backup, "SELECT COUNT(*) FROM users") == 2
              && numberFromFile(backup, "SELECT COUNT(*) FROM payments") == 1,
          backup);

    bool amnaIn = signIn(accounts.auth, "amna@example.com");
    check(counter, "after the upgrade Amna signs in with her old password and is still on Plus",
          amnaIn && accounts.database.currentPlan() == PLAN_PLUS, accounts.auth.lastError());
    GoalStore goals(accounts.database);
    Goal first;
    first.name = "First goal";
    first.currencyCode = "PKR";
    first.targetMinor = 100000;
    check(counter, "and she can create her first savings goal", goals.addGoal(first), goals.lastError());
}

void testUpgradeFromVersion3Rollback(TestCounter &counter, const QString &parentFolder,
                                     const PasswordHasher &quick)
{
    // A version 3 file in which a table called "goal_contributions" already
    // exists. The upgrade creates "goals" first and then fails, so the
    // rollback must undo it.
    QString folder = parentFolder + "/upgrade_v3_conflict";
    QDir().mkpath(folder);
    QString path = folder + "/goals_conflict.db";
    bool prepared = createVersion3File(folder, "goals_conflict", quick)
                    && runSqlOnFile(path, {"CREATE TABLE goal_contributions (note TEXT)"});
    check(counter, "a version 3 file with a conflicting \"goal_contributions\" table is prepared", prepared);

    QByteArray bytesBefore = readFile(path);
    {
        DatabaseManager database("goals_conflict_open");
        check(counter, "the upgrade to version 4 is refused", !database.open(path));
        std::cout << "        message: " << database.lastError().toStdString() << "\n";
        check(counter, "the backup made before the attempt is kept", QFile::exists(database.lastBackupPath()));
    }
    check(counter, "after the rollback the file is still version 3", DatabaseManager::storedSchemaVersion(path) == 3);
    check(counter, "after the rollback the file is byte-for-byte as before", readFile(path) == bytesBefore);
    check(counter, "no goals table was left behind",
          numberFromFile(path, "SELECT COUNT(*) FROM sqlite_master WHERE name = 'goals'") == 0);
}

// ---- Milestone 5: savings goals ----------------------------------------

Goal makeGoal(const std::string &name, long long targetMinor, const std::string &targetDate)
{
    Goal goal;
    goal.name = name;
    goal.currencyCode = "PKR";
    goal.targetMinor = targetMinor;
    goal.targetDate = targetDate;
    return goal;
}

GoalContribution makeContribution(int goalId, long long amountMinor)
{
    GoalContribution contribution;
    contribution.goalId = goalId;
    contribution.amountMinor = amountMinor;
    contribution.date = "2026-10-31";
    return contribution;
}

// The saved amount of one goal as loadGoals() reports it, or -1.
long long savedOf(GoalStore &goals, int goalId)
{
    std::vector<Goal> list;
    goals.loadGoals(list);
    for (int i = 0; i < static_cast<int>(list.size()); i++) {
        if (list[i].id == goalId) {
            return list[i].savedMinor;
        }
    }
    return -1;
}

void testGoalsNeedPlus(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("goals_plan", folder, quick);
    GoalStore goals(accounts.database);
    const QString &c = accounts.connection;
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");

    Goal laptop = makeGoal("New laptop", 25000000, "2027-03-31");
    check(counter, "on Free, GoalStore itself refuses a new goal and names Finora Plus",
          !goals.addGoal(laptop) && goals.planBlocked() && goals.lastError().contains("Finora Plus"),
          goals.lastError());
    check(counter, "and nothing was stored", numberFromSql(c, "SELECT COUNT(*) FROM goals") == 0);
    std::vector<Goal> list;
    check(counter, "on Free, loading goals still works (an empty list)", goals.loadGoals(list) && list.empty());

    setPlanDirectly(c, "amna@example.com", "plus", "active", FAR_FUTURE, FAR_FUTURE);
    check(counter, "on Plus, the goal is saved and gets an id",
          goals.addGoal(laptop) && laptop.id > 0 && !goals.planBlocked(), goals.lastError());
    Goal sameName = makeGoal("  new LAPTOP ", 100000, "");
    check(counter, "a second goal with the same name in other letters is refused",
          !goals.addGoal(sameName) && goals.lastError().contains("already a goal") && !goals.planBlocked());
    Goal noName = makeGoal("   ", 100000, "");
    check(counter, "a goal without a name is refused before any SQL runs",
          !goals.addGoal(noName) && goals.lastError() == "Please give the goal a name.");
    Goal emergency = makeGoal("  Emergency fund ", 10000000, "");
    check(counter, "a goal without a date is saved with a trimmed name and NULL as its date",
          goals.addGoal(emergency)
              && numberFromSql(c, "SELECT COUNT(*) FROM goals WHERE name = 'Emergency fund' "
                                  "AND target_date IS NULL") == 1);
    goals.loadGoals(list);
    check(counter, "loadGoals returns both goals sorted by name, nothing saved yet",
          list.size() == 2 && list[0].name == "Emergency fund" && list[0].targetDate.empty()
              && list[1].name == "New laptop" && list[1].targetDate == "2027-03-31"
              && list[1].targetMinor == 25000000 && list[0].savedMinor == 0 && list[1].savedMinor == 0);

    setPlanDirectly(c, "amna@example.com", "business", "active", FAR_FUTURE, FAR_FUTURE);
    Goal house = makeGoal("House", 500000000, "2035-12-31");
    check(counter, "every larger plan includes goals too (Business)", goals.addGoal(house), goals.lastError());
}

void testGoalContributions(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("goals_money", folder, quick);
    GoalStore goals(accounts.database);
    const QString &c = accounts.connection;
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    setPlanDirectly(c, "amna@example.com", "plus", "active", FAR_FUTURE, FAR_FUTURE);
    Goal laptop = makeGoal("New laptop", 25000000, "2027-03-31");
    goals.addGoal(laptop);

    GoalContribution first = makeContribution(laptop.id, 5000000);   // 50,000.00 in
    first.note = "October salary";
    check(counter, "money put into a goal is saved and gets an id",
          goals.addContribution(first) && first.id > 0, goals.lastError());
    GoalContribution out = makeContribution(laptop.id, -2000000);    // 20,000.00 out
    check(counter, "taking out less than is saved works", goals.addContribution(out), goals.lastError());
    check(counter, "the goal now has 30,000.00 saved", savedOf(goals, laptop.id) == 3000000);

    GoalContribution tooMuch = makeContribution(laptop.id, -3000001);
    check(counter, "taking out 0.01 more than is saved is refused",
          !goals.addContribution(tooMuch) && goals.lastError() == "You can take out at most what is saved.");
    GoalContribution zero = makeContribution(laptop.id, 0);
    check(counter, "an amount of zero is refused", !goals.addContribution(zero));
    GoalContribution badDate = makeContribution(laptop.id, 100);
    badDate.date = "2026-02-30";
    check(counter, "an invalid date is refused", !goals.addContribution(badDate));
    GoalContribution everything = makeContribution(laptop.id, -3000000);
    check(counter, "taking out exactly what is saved works and leaves 0",
          goals.addContribution(everything) && savedOf(goals, laptop.id) == 0);

    GoalContribution largest = makeContribution(laptop.id, MAX_AMOUNT_MINOR);
    GoalContribution onePaisa = makeContribution(laptop.id, 1);
    check(counter, "the largest amount can be saved", goals.addContribution(largest), goals.lastError());
    check(counter, "but not even 0.01 more: the total can never overflow",
          !goals.addContribution(onePaisa) && goals.lastError() == "The saved amount would become too large.");
    check(counter, "the refused contributions left no rows behind",
          numberFromSql(c, "SELECT COUNT(*) FROM goal_contributions") == 4);
    // BEGIN fails while another database transaction is still open.
    check(counter, "every refusal ended its database transaction (a new one can begin)",
          runSql(c, "BEGIN") && runSql(c, "ROLLBACK"));

    std::vector<GoalContribution> history;
    check(counter, "the history lists the 4 contributions, newest first",
          goals.loadContributions(laptop.id, history) && history.size() == 4
              && history[0].amountMinor == MAX_AMOUNT_MINOR && history[3].note == "October salary");

    // The currency may change only while nothing is saved.
    Goal trip = makeGoal("Trip", 50000, "");
    goals.addGoal(trip);
    trip.currencyCode = "USD";
    check(counter, "the currency of a goal with nothing saved can change", goals.updateGoal(trip), goals.lastError());
    GoalContribution tripMoney = makeContribution(trip.id, 1000);
    goals.addContribution(tripMoney);
    trip.currencyCode = "EUR";
    check(counter, "once money is in, the currency cannot change",
          !goals.updateGoal(trip) && goals.lastError().contains("currency cannot change"));
    trip.currencyCode = "USD";
    trip.name = "Summer trip";
    trip.targetMinor = 80000;
    check(counter, "the name and the target can still change",
          goals.updateGoal(trip)
              && textFromSql(c, "SELECT name || '/' || target_minor FROM goals WHERE currency_code = 'USD'")
                     == "Summer trip/80000",
          goals.lastError());

    check(counter, "deleting a goal deletes its own contributions with it, and no others",
          goals.deleteGoal(laptop.id)
              && numberFromSql(c, "SELECT COUNT(*) FROM goal_contributions WHERE goal_id = "
                                      + QString::number(laptop.id)) == 0
              && numberFromSql(c, "SELECT COUNT(*) FROM goal_contributions") == 1);
}

// Money put into or taken out of a goal is not a transaction: the
// transactions, the balance and the budgets stay exactly the same.
void testGoalsLeaveBalancesAlone(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("goals_balances", folder, quick);
    GoalStore goals(accounts.database);
    const QString &c = accounts.connection;
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    setPlanDirectly(c, "amna@example.com", "plus", "active", FAR_FUTURE, FAR_FUTURE);
    Transaction salary = makeExpense("Salary", 10000000, "2026-10-01");
    salary.type = TYPE_INCOME;
    Transaction food = makeExpense("Food", 250000, "2026-10-05");
    Budget foodBudget = makeBudget("Food", 2000000);
    accounts.database.addTransaction(salary);
    accounts.database.addTransaction(food);
    accounts.budgets.addBudget(foodBudget);

    std::vector<Transaction> before;
    accounts.database.loadTransactions(before);
    CurrencySummary pkrBefore = summarizeByCurrency(before, "2026-10")[0];   // PKR comes first
    QString transactionsBefore = tableSnapshot(c, "transactions", "1 = 1");
    QString budgetsBefore = tableSnapshot(c, "budgets", "1 = 1");

    Goal laptop = makeGoal("New laptop", 25000000, "");
    goals.addGoal(laptop);
    GoalContribution moneyIn = makeContribution(laptop.id, 5000000);     // 50,000.00 in
    GoalContribution moneyOut = makeContribution(laptop.id, -1000000);   // 10,000.00 out
    bool saved = goals.addContribution(moneyIn) && goals.addContribution(moneyOut);

    std::vector<Transaction> after;
    accounts.database.loadTransactions(after);
    CurrencySummary pkrAfter = summarizeByCurrency(after, "2026-10")[0];
    check(counter, "putting money into a goal and taking some out adds no transaction and changes none",
          saved && after.size() == before.size() && tableSnapshot(c, "transactions", "1 = 1") == transactionsBefore);
    check(counter, "so the PKR balance stays PKR 97,500.00, and this month's income and spending too",
          pkrBefore.currencyCode == "PKR" && balanceOf(pkrAfter) == 9750000 && balanceOf(pkrAfter) == balanceOf(pkrBefore)
              && pkrAfter.monthIncomeMinor == pkrBefore.monthIncomeMinor
              && pkrAfter.monthExpenseMinor == pkrBefore.monthExpenseMinor);
    check(counter, "and the budgets are untouched", tableSnapshot(c, "budgets", "1 = 1") == budgetsBefore);
    check(counter, "only the goal shows the money: PKR 40,000.00 set aside", savedOf(goals, laptop.id) == 4000000);
}

void testGoalsArePrivate(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("goals_private", folder, quick);
    GoalStore goals(accounts.database);
    const QString &c = accounts.connection;
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    setPlanDirectly(c, "amna@example.com", "plus", "active", FAR_FUTURE, FAR_FUTURE);
    Goal amnaGoal = makeGoal("Hajj", 100000000, "");
    goals.addGoal(amnaGoal);
    GoalContribution amnaMoney = makeContribution(amnaGoal.id, 2500000);
    goals.addContribution(amnaMoney);
    accounts.auth.signOut("signed_out");

    signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com");
    setPlanDirectly(c, "bilal@example.com", "plus", "active", FAR_FUTURE, FAR_FUTURE);
    std::vector<Goal> list;
    check(counter, "Bilal does not see Amna's goal", goals.loadGoals(list) && list.empty());
    Goal bilalGoal = makeGoal("Hajj", 50000000, "");
    check(counter, "Bilal may use the same goal name as Amna", goals.addGoal(bilalGoal), goals.lastError());
    GoalContribution intoAmna = makeContribution(amnaGoal.id, 100);
    check(counter, "Bilal cannot put money into Amna's goal: it is \"not found\"",
          !goals.addContribution(intoAmna) && goals.lastError().contains("was found"));
    GoalContribution outOfAmna = makeContribution(amnaGoal.id, -100);
    check(counter, "nor take money out of it", !goals.addContribution(outOfAmna));
    Goal changed = amnaGoal;
    changed.name = "Changed by Bilal";
    check(counter, "nor change it", !goals.updateGoal(changed));
    check(counter, "nor delete it", !goals.deleteGoal(amnaGoal.id));
    std::vector<GoalContribution> history;
    check(counter, "nor read its history", goals.loadContributions(amnaGoal.id, history) && history.empty());
    check(counter, "Amna's goal and money are untouched",
          textFromSql(c, "SELECT name FROM goals WHERE id = " + QString::number(amnaGoal.id)) == "Hajj"
              && numberFromSql(c, "SELECT SUM(amount_minor) FROM goal_contributions WHERE goal_id = "
                                      + QString::number(amnaGoal.id)) == 2500000);

    accounts.auth.signOut("signed_out");
    check(counter, "signed out, no goals can be loaded",
          !goals.loadGoals(list) && goals.lastError() == "Please sign in first.");
}

void testGoalsAfterDowngrade(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("goals_downgrade", folder, quick);
    SimulatedPaymentGateway gateway;
    SubscriptionService service(accounts.database, gateway);
    GoalStore goals(accounts.database);
    const QString &c = accounts.connection;
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    std::string reference;
    check(counter, "Amna pays for Plus with the simulated checkout",
          service.payForPlan(PLAN_PLUS, false, reference), service.lastError());
    Goal car = makeGoal("Car", 300000000, "2028-01-31");
    goals.addGoal(car);
    GoalContribution money = makeContribution(car.id, 7500000);
    goals.addContribution(money);

    // The paid period and its grace both end: Finora Free applies again.
    setPlanDirectly(c, "amna@example.com", "plus", "active", LONG_AGO, LONG_AGO);
    check(counter, "after the plan has ended, Amna is on Free", accounts.database.currentPlan() == PLAN_FREE);
    std::vector<Goal> list;
    check(counter, "her goal and its 75,000.00 are kept and still load",
          goals.loadGoals(list) && list.size() == 1 && list[0].savedMinor == 7500000);
    GoalContribution more = makeContribution(car.id, 100);
    check(counter, "on Free, no money can be put in", !goals.addContribution(more) && goals.planBlocked());
    GoalContribution less = makeContribution(car.id, -100);
    check(counter, "or taken out", !goals.addContribution(less) && goals.planBlocked());
    Goal renamed = list[0];
    renamed.name = "Family car";
    check(counter, "or the goal changed", !goals.updateGoal(renamed) && goals.planBlocked());
    check(counter, "nothing in the database changed",
          numberFromSql(c, "SELECT COUNT(*) FROM goal_contributions") == 1
              && textFromSql(c, "SELECT name FROM goals") == "Car");
    check(counter, "on Free, a person may still delete their own goal",
          goals.deleteGoal(car.id) && !goals.planBlocked() && numberFromSql(c, "SELECT COUNT(*) FROM goals") == 0,
          goals.lastError());
}

// SQLite's own rules refuse bad goal rows, even from SQL that bypasses
// Finora's C++ code.
void testGoalDatabaseRules(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("goals_rules", folder, quick);
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");
    const QString &c = accounts.connection;
    QString amna = "(SELECT id FROM users WHERE email = 'amna@example.com')";
    QString insertGoal = "INSERT INTO goals (user_id, name, currency_code, target_minor, target_date) VALUES ";

    check(counter, "a target of zero is refused",
          !runSql(c, insertGoal + "(" + amna + ", 'Zero', 'PKR', 0, NULL)"));
    check(counter, "a goal without an owner is refused",
          !runSql(c, insertGoal + "(NULL, 'Nobody', 'PKR', 100, NULL)"));
    check(counter, "an impossible target date is refused",
          !runSql(c, insertGoal + "(" + amna + ", 'Bad date', 'PKR', 100, '2026-02-30')"));
    check(counter, "an unknown currency is refused",
          !runSql(c, insertGoal + "(" + amna + ", 'Pounds', 'GBP', 100, NULL)"));
    check(counter, "a valid goal is accepted", runSql(c, insertGoal + "(" + amna + ", 'Bike', 'PKR', 100000, NULL)"));
    check(counter, "the same name in capitals is refused by the UNIQUE index",
          !runSql(c, insertGoal + "(" + amna + ", 'BIKE', 'PKR', 100, NULL)"));

    QString insertMoney = "INSERT INTO goal_contributions (goal_id, amount_minor, contribution_date) VALUES ";
    QString bike = "(SELECT id FROM goals WHERE name = 'Bike')";
    check(counter, "money can be put in with plain SQL", runSql(c, insertMoney + "(" + bike + ", 5000, '2026-10-31')"));
    check(counter, "the trigger refuses taking out more than is saved",
          !runSql(c, insertMoney + "(" + bike + ", -5001, '2026-10-31')"));
    check(counter, "the trigger refuses a saved amount above the largest amount",
          !runSql(c, insertMoney + "(" + bike + ", 999999999999, '2026-10-31')"));
    check(counter, "a contribution of zero is refused", !runSql(c, insertMoney + "(" + bike + ", 0, '2026-10-31')"));
    check(counter, "a contribution for a goal that does not exist is refused",
          !runSql(c, insertMoney + "(9999, 100, '2026-10-31')"));
    check(counter, "the saved amount is still exactly 50.00",
          numberFromSql(c, "SELECT SUM(amount_minor) FROM goal_contributions") == 5000);
}

// ---- Milestone 5: saved exchange rates -----------------------------------

void testExchangeRateSettings(TestCounter &counter, const QString &folder, const PasswordHasher &quick)
{
    TestAccounts accounts("rates", folder, quick);
    DatabaseManager &database = accounts.database;
    const QString &c = accounts.connection;
    signUp(accounts.auth, "Amna Siddiqui", "amna@example.com");

    ExchangeRates none = database.exchangeRates();
    check(counter, "a new account has no rates yet",
          none.usdInPkr == 0 && none.eurInPkr == 0 && database.exchangeRatesSavedOn().isEmpty());

    ExchangeRates rates;
    rates.usdInPkr = 2785000;   // 278.5000
    rates.eurInPkr = 3021234;   // 302.1234
    check(counter, "the rates are saved", database.saveExchangeRates(rates), database.lastError());
    ExchangeRates loaded = database.exchangeRates();
    check(counter, "and read back exactly", loaded.usdInPkr == 2785000 && loaded.eurInPkr == 3021234);
    check(counter, "they are stored as readable text with 4 decimal places",
          textFromSql(c, "SELECT value FROM user_settings WHERE key = 'usd_in_pkr'") == "278.5000");
    check(counter, "the day they were saved is today",
          database.exchangeRatesSavedOn() == QDate::currentDate().toString(Qt::ISODate));

    ExchangeRates negative;
    negative.usdInPkr = -1;
    check(counter, "a negative rate is refused and the saved rates stay",
          !database.saveExchangeRates(negative) && database.exchangeRates().usdInPkr == 2785000);
    ExchangeRates tooLarge;
    tooLarge.eurInPkr = MAX_RATE + 1;
    check(counter, "a rate above the largest rate is refused", !database.saveExchangeRates(tooLarge));

    runSql(c, "UPDATE user_settings SET value = '12abc' WHERE key = 'eur_in_pkr'");
    check(counter, "a saved rate changed by hand into nonsense counts as not entered",
          database.exchangeRates().eurInPkr == 0 && database.exchangeRates().usdInPkr == 2785000);

    ExchangeRates usdOnly;
    usdOnly.usdInPkr = 2800000;
    check(counter, "saving only one rate leaves the other empty",
          database.saveExchangeRates(usdOnly) && database.exchangeRates().eurInPkr == 0
              && textFromSql(c, "SELECT value FROM user_settings WHERE key = 'eur_in_pkr'").isEmpty());

    accounts.auth.signOut("signed_out");
    signUp(accounts.auth, "Bilal Ahmed", "bilal@example.com");
    check(counter, "Bilal does not see Amna's rates: rates belong to each account",
          database.exchangeRates().usdInPkr == 0 && database.exchangeRatesSavedOn().isEmpty());
}

// ---- main --------------------------------------------------------------

int main(int argc, char *argv[])
{
    // A QGuiApplication, because the PDF summary needs fonts. CTest runs
    // it with QT_QPA_PLATFORM=offscreen, so no window can appear.
    QGuiApplication app(argc, argv);

    TestCounter counter;
    std::cout << "Finora subscription and billing tests\n";
    std::cout << "-------------------------------------\n";

    bool started = PasswordHasher::initializeLibrary();
    check(counter, "libsodium starts (sodium_init)", started);
    if (!started) {
        return 1;
    }

    QTemporaryDir temporaryFolder;
    if (!temporaryFolder.isValid()) {
        std::cout << "Could not create a temporary folder.\n";
        return 1;
    }
    QString folder = temporaryFolder.path();

    // These tests are about subscriptions, not password strength, so the
    // accounts use libsodium's smallest Argon2id setting (1 pass, 8 KiB)
    // to keep the tests fast. Finora itself uses MODERATE (256 MiB).
    PasswordHasher quick(1, 8192);

    std::cout << "---- 4C.1 the subscription data model ----\n";
    testNewDatabase(counter, folder);
    testSignUpStartsOnFree(counter, folder, quick);
    testSignUpIsAllOrNothing(counter, folder, quick);
    std::cout << "---- 4C.1 the upgrade from version 2 (now to version 4) ----\n";
    testUpgradeFromVersion2(counter, folder, quick);
    testUpgradeRollback(counter, folder, quick);
    std::cout << "---- 4C.1 the database's own rules ----\n";
    testDatabaseRules(counter, folder, quick);
    std::cout << "---- 4C.2 entitlements ----\n";
    testCurrentPlan(counter, folder, quick);
    testBudgetLimit(counter, folder, quick);
    testClaimOverTheLimit(counter, folder, quick);
    std::cout << "---- 4C.4 the simulated checkout and the lifecycle ----\n";
    testSimulatedGateway(counter);
    testPayingForPlans(counter, folder, quick);
    testSmallerPlanCancelAndResume(counter, folder, quick);
    testTimeAndRenewal(counter, folder, quick);
    testTrial(counter, folder, quick);
    testInterruptedPayment(counter, folder, quick);
    testActivationGuard(counter, folder, quick);
    std::cout << "---- 4C.5 exports and the audit history ----\n";
    testExportsAndAuditHistory(counter, folder, quick);
    std::cout << "---- Milestone 5: the upgrade from version 3 to 4 ----\n";
    testUpgradeFromVersion3(counter, folder, quick);
    testUpgradeFromVersion3Rollback(counter, folder, quick);
    std::cout << "---- Milestone 5: savings goals and saved exchange rates ----\n";
    testGoalsNeedPlus(counter, folder, quick);
    testGoalContributions(counter, folder, quick);
    testGoalsLeaveBalancesAlone(counter, folder, quick);
    testGoalsArePrivate(counter, folder, quick);
    testGoalsAfterDowngrade(counter, folder, quick);
    testGoalDatabaseRules(counter, folder, quick);
    testExchangeRateSettings(counter, folder, quick);

    std::cout << "-------------------------------------\n";
    std::cout << counter.checksRun << " checks, " << counter.checksFailed << " failed\n";
    if (counter.checksFailed > 0) {
        return 1;
    }
    return 0;
}
