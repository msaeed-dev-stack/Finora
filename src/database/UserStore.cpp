// =====================================================================
//  UserStore.cpp
// =====================================================================
#include "UserStore.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "core/AccountRules.h"   // SESSION_MAX_HOURS

// The current time in UTC, as stored in the database: "2026-10-30T18:00:00Z".
QString utcTimeText(int secondsFromNow)
{
    return QDateTime::currentDateTimeUtc().addSecs(secondsFromNow).toString(Qt::ISODate);
}

// NULL for "no account" (0), otherwise the id.
QVariant userIdValue(int userId)
{
    if (userId <= 0) {
        return QVariant();
    }
    return userId;
}

UserAccount readAccountRow(const QSqlQuery &query)
{
    UserAccount account;
    account.id = query.value(0).toInt();
    account.email = query.value(1).toString().toStdString();
    account.displayName = query.value(2).toString().toStdString();
    account.passwordHash = query.value(3).toString().toStdString();
    account.failedSignIns = query.value(4).toInt();
    account.lockedUntil = query.value(5).toString().toStdString();
    account.createdAt = query.value(6).toString().toStdString();
    account.lastSignInAt = query.value(7).toString().toStdString();
    return account;
}

const QString ACCOUNT_COLUMNS =
    "SELECT id, email, display_name, password_hash, failed_login_count, "
    "ifnull(locked_until, ''), created_at, ifnull(last_login_at, '') FROM users ";

UserStore::UserStore(DatabaseManager &sharedDatabase)
    : database(sharedDatabase)
{
}

bool UserStore::countAccounts(int &count)
{
    count = 0;
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    if (!query.exec("SELECT COUNT(*) FROM users") || !query.next()) {
        return fail("Could not count the accounts: " + query.lastError().text());
    }
    count = query.value(0).toInt();
    return true;
}

bool UserStore::findByEmail(const std::string &email, UserAccount &account, bool &found)
{
    found = false;
    // The email column is COLLATE NOCASE, so "Ali@Mail.pk" finds "ali@mail.pk".
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare(ACCOUNT_COLUMNS + "WHERE email = :email");
    query.bindValue(":email", QString::fromStdString(email));
    if (!query.exec()) {
        return fail("Could not look up the account: " + query.lastError().text());
    }
    if (query.next()) {
        account = readAccountRow(query);
        found = true;
    }
    return true;
}

bool UserStore::findById(int userId, UserAccount &account)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare(ACCOUNT_COLUMNS + "WHERE id = :id");
    query.bindValue(":id", userId);
    if (!query.exec()) {
        return fail("Could not read the account: " + query.lastError().text());
    }
    if (!query.next()) {
        return fail("The account was not found.");
    }
    account = readAccountRow(query);
    return true;
}

bool UserStore::isEmailUsedByOther(const std::string &email, int userId, bool &used)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("SELECT COUNT(*) FROM users WHERE email = :email AND id <> :id");
    query.bindValue(":email", QString::fromStdString(email));
    query.bindValue(":id", userId);
    if (!query.exec() || !query.next()) {
        return fail("Could not check the email address: " + query.lastError().text());
    }
    used = query.value(0).toInt() > 0;
    return true;
}

bool UserStore::createAccount(UserAccount &account, const std::vector<std::string> &recoveryCodeHashes)
{
    QSqlDatabase db = QSqlDatabase::database(database.sqlConnectionName(), false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }

    QSqlQuery insertUser(db);
    insertUser.prepare("INSERT INTO users (email, display_name, password_hash) "
                       "VALUES (:email, :name, :hash)");
    insertUser.bindValue(":email", QString::fromStdString(account.email));
    insertUser.bindValue(":name", QString::fromStdString(account.displayName));
    insertUser.bindValue(":hash", QString::fromStdString(account.passwordHash));
    if (!insertUser.exec()) {
        return rollbackAndFail("Could not create the account: " + insertUser.lastError().text());
    }
    int newId = insertUser.lastInsertId().toInt();

    // The new person starts with the theme and motion setting in use now.
    QSqlQuery copySettings(db);
    copySettings.prepare("INSERT INTO user_settings (user_id, key, value) "
                         "SELECT :id, key, value FROM app_settings WHERE key IN ('theme', 'reduced_motion')");
    copySettings.bindValue(":id", newId);
    if (!copySettings.exec()) {
        return rollbackAndFail("Could not save the account's settings: " + copySettings.lastError().text());
    }

    // Every new account starts on Finora Free (Phase 4C). It is created
    // in the same transaction, so an account never exists without a plan.
    QSqlQuery freePlan(db);
    freePlan.prepare("INSERT INTO subscriptions (user_id, plan, status) VALUES (:id, 'free', 'free')");
    freePlan.bindValue(":id", newId);
    if (!freePlan.exec()) {
        return rollbackAndFail("Could not start the Free plan: " + freePlan.lastError().text());
    }

    if (!insertRecoveryCodes(newId, recoveryCodeHashes) || !insertEvent(newId, "account_created", "")
        || !insertEvent(newId, "plan_started", "Finora Free")) {
        return rollbackAndFail(errorMessage);
    }
    if (!db.commit()) {
        return rollbackAndFail("Could not save the new account: " + db.lastError().text());
    }
    account.id = newId;
    return true;
}

bool UserStore::recordFailedSignIn(int userId, int failedSignIns, const std::string &lockedUntil)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE users SET failed_login_count = :count, locked_until = :until WHERE id = :id");
    query.bindValue(":count", failedSignIns);
    if (lockedUntil.empty()) {
        query.bindValue(":until", QVariant());   // NULL: no waiting needed
    } else {
        query.bindValue(":until", QString::fromStdString(lockedUntil));
    }
    query.bindValue(":id", userId);
    if (!query.exec()) {
        return fail("Could not record the failed sign-in: " + query.lastError().text());
    }
    return true;
}

bool UserStore::recordSuccessfulSignIn(int userId)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE users SET failed_login_count = 0, locked_until = NULL, last_login_at = :now "
                  "WHERE id = :id");
    query.bindValue(":now", utcTimeText(0));
    query.bindValue(":id", userId);
    if (!query.exec()) {
        return fail("Could not record the sign-in: " + query.lastError().text());
    }
    return true;
}

bool UserStore::updatePasswordHash(int userId, const std::string &passwordHash)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE users SET password_hash = :hash, updated_at = :now WHERE id = :id");
    query.bindValue(":hash", QString::fromStdString(passwordHash));
    query.bindValue(":now", utcTimeText(0));
    query.bindValue(":id", userId);
    if (!query.exec() || query.numRowsAffected() != 1) {
        return fail("Could not save the new password: " + query.lastError().text());
    }
    return true;
}

bool UserStore::updateProfile(int userId, const std::string &email, const std::string &displayName)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE users SET email = :email, display_name = :name, updated_at = :now WHERE id = :id");
    query.bindValue(":email", QString::fromStdString(email));
    query.bindValue(":name", QString::fromStdString(displayName));
    query.bindValue(":now", utcTimeText(0));
    query.bindValue(":id", userId);
    if (!query.exec() || query.numRowsAffected() != 1) {
        return fail("Could not save the profile: " + query.lastError().text());
    }
    return true;
}

// ---- Recovery codes ------------------------------------------------------

bool UserStore::loadUnusedRecoveryCodes(int userId, std::vector<StoredRecoveryCode> &codes)
{
    codes.clear();
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("SELECT id, code_hash FROM recovery_codes WHERE user_id = :id AND used_at IS NULL ORDER BY id");
    query.bindValue(":id", userId);
    if (!query.exec()) {
        return fail("Could not read the recovery codes: " + query.lastError().text());
    }
    while (query.next()) {
        StoredRecoveryCode code;
        code.id = query.value(0).toInt();
        code.codeHash = query.value(1).toString().toStdString();
        codes.push_back(code);
    }
    return true;
}

bool UserStore::countUnusedRecoveryCodes(int userId, int &count)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("SELECT COUNT(*) FROM recovery_codes WHERE user_id = :id AND used_at IS NULL");
    query.bindValue(":id", userId);
    if (!query.exec() || !query.next()) {
        return fail("Could not count the recovery codes: " + query.lastError().text());
    }
    count = query.value(0).toInt();
    return true;
}

bool UserStore::replaceRecoveryCodes(int userId, const std::vector<std::string> &recoveryCodeHashes)
{
    QSqlDatabase db = QSqlDatabase::database(database.sqlConnectionName(), false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }
    // The old codes, used or not, stop working at once.
    QSqlQuery removeOld(db);
    removeOld.prepare("DELETE FROM recovery_codes WHERE user_id = :id");
    removeOld.bindValue(":id", userId);
    if (!removeOld.exec()) {
        return rollbackAndFail("Could not remove the old recovery codes: " + removeOld.lastError().text());
    }
    if (!insertRecoveryCodes(userId, recoveryCodeHashes) || !insertEvent(userId, "recovery_codes_replaced", "")) {
        return rollbackAndFail(errorMessage);
    }
    if (!db.commit()) {
        return rollbackAndFail("Could not save the new recovery codes: " + db.lastError().text());
    }
    return true;
}

bool UserStore::completePasswordReset(int userId, int recoveryCodeId, const std::string &newPasswordHash)
{
    QSqlDatabase db = QSqlDatabase::database(database.sqlConnectionName(), false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }
    QString now = utcTimeText(0);

    // "used_at IS NULL" in the WHERE makes sure a code works only once,
    // even if two resets raced each other.
    QSqlQuery useCode(db);
    useCode.prepare("UPDATE recovery_codes SET used_at = :now "
                    "WHERE id = :code AND user_id = :id AND used_at IS NULL");
    useCode.bindValue(":now", now);
    useCode.bindValue(":code", recoveryCodeId);
    useCode.bindValue(":id", userId);
    if (!useCode.exec() || useCode.numRowsAffected() != 1) {
        return rollbackAndFail("The recovery code could not be used.");
    }

    QSqlQuery setPassword(db);
    setPassword.prepare("UPDATE users SET password_hash = :hash, failed_login_count = 0, locked_until = NULL, "
                        "updated_at = :now WHERE id = :id");
    setPassword.bindValue(":hash", QString::fromStdString(newPasswordHash));
    setPassword.bindValue(":now", now);
    setPassword.bindValue(":id", userId);
    if (!setPassword.exec()) {
        return rollbackAndFail("Could not save the new password: " + setPassword.lastError().text());
    }

    QSqlQuery endSessions(db);
    endSessions.prepare("UPDATE login_sessions SET ended_at = :now, end_reason = 'password_reset' "
                        "WHERE user_id = :id AND ended_at IS NULL");
    endSessions.bindValue(":now", now);
    endSessions.bindValue(":id", userId);
    if (!endSessions.exec() || !insertEvent(userId, "password_reset_with_recovery_code", "")) {
        return rollbackAndFail("Could not finish the password reset.");
    }
    if (!db.commit()) {
        return rollbackAndFail("Could not save the password reset: " + db.lastError().text());
    }
    return true;
}

// ---- Sessions ----------------------------------------------------------------

bool UserStore::startSession(int userId, int &sessionId)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("INSERT INTO login_sessions (user_id, expires_at) VALUES (:id, :expires)");
    query.bindValue(":id", userId);
    query.bindValue(":expires", utcTimeText(SESSION_MAX_HOURS * 3600));
    if (!query.exec()) {
        return fail("Could not start the session: " + query.lastError().text());
    }
    sessionId = query.lastInsertId().toInt();
    return true;
}

bool UserStore::sessionExpiresAt(int sessionId, std::string &expiresAt)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("SELECT expires_at FROM login_sessions WHERE id = :id");
    query.bindValue(":id", sessionId);
    if (!query.exec() || !query.next()) {
        return fail("Could not read the session: " + query.lastError().text());
    }
    expiresAt = query.value(0).toString().toStdString();
    return true;
}

bool UserStore::touchSession(int sessionId)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE login_sessions SET last_active_at = :now WHERE id = :id");
    query.bindValue(":now", utcTimeText(0));
    query.bindValue(":id", sessionId);
    if (!query.exec()) {
        return fail("Could not update the session: " + query.lastError().text());
    }
    return true;
}

bool UserStore::endSession(int sessionId, const std::string &reason)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE login_sessions SET ended_at = :now, end_reason = :reason "
                  "WHERE id = :id AND ended_at IS NULL");
    query.bindValue(":now", utcTimeText(0));
    query.bindValue(":reason", QString::fromStdString(reason));
    query.bindValue(":id", sessionId);
    if (!query.exec()) {
        return fail("Could not end the session: " + query.lastError().text());
    }
    return true;
}

bool UserStore::endInterruptedSessions()
{
    // Only one copy of Finora can run (InstanceLock), so a session that is
    // still open when Finora starts was left behind by a crash.
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE login_sessions SET ended_at = :now, end_reason = 'interrupted' WHERE ended_at IS NULL");
    query.bindValue(":now", utcTimeText(0));
    if (!query.exec()) {
        return fail("Could not close old sessions: " + query.lastError().text());
    }
    return true;
}

// ---- Records made before accounts existed ------------------------------------

bool UserStore::countRecordsWithoutOwner(int &transactions, int &budgets)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    if (!query.exec("SELECT (SELECT COUNT(*) FROM transactions WHERE user_id IS NULL), "
                    "(SELECT COUNT(*) FROM budgets WHERE user_id IS NULL)")
        || !query.next()) {
        return fail("Could not count the records without an account: " + query.lastError().text());
    }
    transactions = query.value(0).toInt();
    budgets = query.value(1).toInt();
    return true;
}

bool UserStore::claimRecordsWithoutOwner(int userId, int &movedTransactions, int &movedBudgets, int &keptBudgets)
{
    movedTransactions = 0;
    movedBudgets = 0;
    keptBudgets = 0;
    int transactionsBefore = 0;
    int budgetsBefore = 0;
    if (!countRecordsWithoutOwner(transactionsBefore, budgetsBefore)) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(database.sqlConnectionName(), false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }
    QString now = utcTimeText(0);

    QSqlQuery moveTransactions(db);
    moveTransactions.prepare("UPDATE transactions SET user_id = :id, updated_at = :now WHERE user_id IS NULL");
    moveTransactions.bindValue(":id", userId);
    moveTransactions.bindValue(":now", now);
    if (!moveTransactions.exec()) {
        return rollbackAndFail("Could not move the transactions: " + moveTransactions.lastError().text());
    }
    movedTransactions = moveTransactions.numRowsAffected();

    // A budget moves only if the account has no budget for the same
    // category and currency yet; otherwise it stays where it is.
    QSqlQuery moveBudgets(db);
    // (Each placeholder name is used once, so every value binds safely.)
    moveBudgets.prepare("UPDATE budgets SET user_id = :id, updated_at = :now WHERE user_id IS NULL "
                        "AND NOT EXISTS (SELECT 1 FROM budgets AS mine WHERE mine.user_id = :owner "
                        "AND lower(mine.category) = lower(budgets.category) "
                        "AND mine.currency_code = budgets.currency_code)");
    moveBudgets.bindValue(":id", userId);
    moveBudgets.bindValue(":now", now);
    moveBudgets.bindValue(":owner", userId);
    if (!moveBudgets.exec()) {
        return rollbackAndFail("Could not move the budgets: " + moveBudgets.lastError().text());
    }
    movedBudgets = moveBudgets.numRowsAffected();
    keptBudgets = budgetsBefore - movedBudgets;

    // Prove that every transaction arrived: none may be left without an owner.
    QSqlQuery checkLeft(db);
    if (!checkLeft.exec("SELECT COUNT(*) FROM transactions WHERE user_id IS NULL") || !checkLeft.next()
        || checkLeft.value(0).toInt() != 0 || movedTransactions != transactionsBefore) {
        return rollbackAndFail("Not every transaction could be moved, so nothing was changed.");
    }

    std::string details = std::to_string(movedTransactions) + " transactions, " + std::to_string(movedBudgets)
                          + " budgets moved; " + std::to_string(keptBudgets) + " budgets kept";
    if (!insertEvent(userId, "records_claimed", details)) {
        return rollbackAndFail(errorMessage);
    }
    if (!db.commit()) {
        return rollbackAndFail("Could not save the move: " + db.lastError().text());
    }
    return true;
}

bool UserStore::recordEvent(int userId, const std::string &eventType, const std::string &details)
{
    return insertEvent(userId, eventType, details);
}

QString UserStore::lastError() const
{
    return errorMessage;
}

// ---- Helpers -----------------------------------------------------------------

bool UserStore::insertRecoveryCodes(int userId, const std::vector<std::string> &recoveryCodeHashes)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("INSERT INTO recovery_codes (user_id, code_hash) VALUES (:id, :hash)");
    int count = static_cast<int>(recoveryCodeHashes.size());
    for (int i = 0; i < count; i++) {
        query.bindValue(":id", userId);
        query.bindValue(":hash", QString::fromStdString(recoveryCodeHashes[i]));
        if (!query.exec()) {
            return fail("Could not save a recovery code: " + query.lastError().text());
        }
    }
    return true;
}

bool UserStore::insertEvent(int userId, const std::string &eventType, const std::string &details)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("INSERT INTO audit_events (user_id, event_type, details) VALUES (:id, :type, :details)");
    query.bindValue(":id", userIdValue(userId));
    query.bindValue(":type", QString::fromStdString(eventType));
    query.bindValue(":details", QString::fromStdString(details));
    if (!query.exec()) {
        return fail("Could not record an event: " + query.lastError().text());
    }
    return true;
}

// Undoes everything since db.transaction(), keeping the first error message.
bool UserStore::rollbackAndFail(const QString &message)
{
    QSqlDatabase db = QSqlDatabase::database(database.sqlConnectionName(), false);
    db.rollback();
    return fail(message);
}

bool UserStore::fail(const QString &message)
{
    errorMessage = message;
    return false;
}
