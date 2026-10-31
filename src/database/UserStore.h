// =====================================================================
//  UserStore.h
//  All SQL for accounts: users, their settings, recovery codes, sign-in
//  sessions, the audit history, and moving records made before accounts
//  existed into an account.
// =====================================================================
#ifndef FINORA_USERSTORE_H
#define FINORA_USERSTORE_H

#include <QString>

#include <string>
#include <vector>

#include "DatabaseManager.h"

// One row of the users table. passwordHash is the Argon2id string, never
// the password itself.
struct UserAccount {
    int id = 0;
    std::string email;
    std::string displayName;
    std::string passwordHash;
    int failedSignIns = 0;      // wrong passwords since the last successful sign-in
    std::string lockedUntil;    // UTC "2026-10-30T18:00:00Z" before which no try is allowed; "" if none
    std::string createdAt;
    std::string lastSignInAt;
};

// One unused recovery code: only its Argon2id hash is stored.
struct StoredRecoveryCode {
    int id = 0;
    std::string codeHash;
};

// UserStore is used only by AuthService, which decides WHICH user id to
// pass: an id found by the email address or the signed-in Session's id,
// never a number typed into the user interface.
class UserStore
{
public:
    UserStore(DatabaseManager &sharedDatabase);

    bool countAccounts(int &count);
    bool findByEmail(const std::string &email, UserAccount &account, bool &found);
    bool findById(int userId, UserAccount &account);
    bool isEmailUsedByOther(const std::string &email, int userId, bool &used);

    // Creates the account, copies the current appearance settings into
    // the new user's settings, stores the recovery code hashes and starts
    // the Free plan, all in ONE database transaction. On success
    // account.id is set.
    bool createAccount(UserAccount &account, const std::vector<std::string> &recoveryCodeHashes);

    bool recordFailedSignIn(int userId, int failedSignIns, const std::string &lockedUntil);
    bool recordSuccessfulSignIn(int userId);
    bool updatePasswordHash(int userId, const std::string &passwordHash);
    bool updateProfile(int userId, const std::string &email, const std::string &displayName);

    // ---- Recovery codes ----
    bool loadUnusedRecoveryCodes(int userId, std::vector<StoredRecoveryCode> &codes);
    bool countUnusedRecoveryCodes(int userId, int &count);
    bool replaceRecoveryCodes(int userId, const std::vector<std::string> &recoveryCodeHashes);
    // In one transaction: marks the code as used, stores the new password
    // hash, clears the wrong-password counter and ends open sessions.
    bool completePasswordReset(int userId, int recoveryCodeId, const std::string &newPasswordHash);

    // ---- Sessions ----
    bool startSession(int userId, int &sessionId);
    bool sessionExpiresAt(int sessionId, std::string &expiresAt);
    bool touchSession(int sessionId);
    bool endSession(int sessionId, const std::string &reason);
    bool endInterruptedSessions();   // sessions left open when Finora stopped unexpectedly

    // ---- Records made before accounts existed (user_id IS NULL) ----
    bool countRecordsWithoutOwner(int &transactions, int &budgets);
    // Moves them into the account in one transaction. A budget is kept
    // where it is if the account already has a budget for the same
    // category and currency (keptBudgets counts those).
    bool claimRecordsWithoutOwner(int userId, int &movedTransactions, int &movedBudgets, int &keptBudgets);

    // Adds one line to the audit history. userId 0 means "no account".
    bool recordEvent(int userId, const std::string &eventType, const std::string &details);

    QString lastError() const;

private:
    bool insertRecoveryCodes(int userId, const std::vector<std::string> &recoveryCodeHashes);
    bool insertEvent(int userId, const std::string &eventType, const std::string &details);
    bool rollbackAndFail(const QString &message);
    bool fail(const QString &message);

    DatabaseManager &database;   // created in main(); this store only uses it
    QString errorMessage;
};

#endif // FINORA_USERSTORE_H
