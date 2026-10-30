// =====================================================================
//  AuthService.h
//  Accounts: sign-up, sign-in, sign-out, sessions, profile, password
//  change and password recovery with single-use recovery codes.
// =====================================================================
#ifndef FINORA_AUTHSERVICE_H
#define FINORA_AUTHSERVICE_H

#include <QDateTime>
#include <QString>

#include <string>
#include <vector>

#include "PasswordHasher.h"
#include "core/AccountRules.h"
#include "core/Session.h"
#include "database/DatabaseManager.h"
#include "database/UserStore.h"

// The answers to a failed sign-in or recovery are always the same, so
// nobody can find out whether an email address has an account.
const QString SIGN_IN_FAILED_MESSAGE = "The email or password is not correct.";
const QString RECOVERY_FAILED_MESSAGE = "The email or recovery code is not correct.";

// The only class that can sign a person in or out (it is a friend of
// Session). Every function that receives a password takes it by
// reference and overwrites it with zeros before returning, so a password
// stays in memory as briefly as possible.
class AuthService
{
public:
    // passwordHasher protects account passwords (Finora: HASH_MODERATE);
    // recoveryCodeHasher protects recovery codes (Finora: HASH_INTERACTIVE).
    // The tests pass faster hashers so they finish quickly.
    AuthService(DatabaseManager &sharedDatabase, Session &sharedSession,
                const PasswordHasher &sharedPasswordHasher, const PasswordHasher &sharedRecoveryCodeHasher);

    const Session &session() const;
    QString lastError() const;

    // True once at least one account exists on this computer.
    bool hasAnyAccount();

    // ---- Registration and sign-in ----

    // Creates an account and signs the new person in. On success,
    // `recoveryCodes` holds 10 codes such as "7KQ4-M2TX-9RHC" in plain
    // text: they are shown ONCE and only their hashes are stored.
    bool signUp(const std::string &displayName, const std::string &email, std::string &password,
                std::vector<std::string> &recoveryCodes);

    // Signs in. A wrong password and an unknown email give the same
    // message; after 3 failures the next try must wait (see AccountRules.h).
    bool signIn(const std::string &email, std::string &password);

    // Ends the session. `reason` is stored, for example "signed_out" or "idle".
    void signOut(const std::string &reason);

    // ---- Sessions ----

    // Called while the person is active. Returns false, and signs out, if
    // the session reached its 12-hour limit.
    bool keepSessionAlive();

    // Signs out after limitSeconds without activity (normally
    // SESSION_IDLE_MINUTES). Returns true if it signed out.
    bool endSessionIfIdle(long long idleSeconds, long long limitSeconds = SESSION_IDLE_MINUTES * 60LL);

    // ---- Profile and password (signed in) ----

    // The signed-in account, without its password hash.
    bool signedInAccount(UserAccount &account);

    // Changes the name and email. A new email also needs the current password.
    bool updateProfile(const std::string &displayName, const std::string &email, std::string &currentPassword);

    bool changePassword(std::string &currentPassword, std::string &newPassword);

    // ---- Recovery codes ----

    bool recoveryCodesLeft(int &count);

    // Replaces all recovery codes with 10 new ones (needs the password).
    bool regenerateRecoveryCodes(std::string &currentPassword, std::vector<std::string> &recoveryCodes);

    // Sets a new password with one recovery code, when the password is
    // forgotten. The code is used up; codesLeft tells how many remain.
    // The person then signs in with the new password.
    bool resetPasswordWithRecoveryCode(const std::string &email, std::string &recoveryCode,
                                       std::string &newPassword, int &codesLeft);

    // ---- Records made before accounts existed ----

    bool countRecordsWithoutOwner(int &transactions, int &budgets);
    // Moves them into the signed-in account; called only after the person
    // agreed. keptBudgets: budgets left out because the account already
    // has a budget for that category and currency.
    bool claimRecordsWithoutOwner(int &movedTransactions, int &movedBudgets, int &keptBudgets);

private:
    // Wrong tries for email addresses that have no account, kept only in
    // memory, so that they get the same answers and delays as real accounts.
    struct UnknownEmailTries {
        std::string email;
        int failures = 0;
        QDateTime waitUntil;   // no try before this time (UTC)
    };

    bool registerFailure(const UserAccount &account, const std::string &eventType);
    bool failForUnknownEmail(const std::string &email, const QString &message);
    bool unknownEmailMustWait(const std::string &email, int &seconds);
    bool makeRecoveryCodes(std::vector<std::string> &plainCodes, std::vector<std::string> &codeHashes);
    bool verifyCurrentPassword(const UserAccount &account, std::string &currentPassword);
    bool startSessionFor(const UserAccount &account, const std::string &eventDetails);
    const std::string &dummyPasswordHash();
    int secondsToWait(const std::string &waitUntilText) const;
    QString tooManyAttemptsMessage(int seconds) const;
    bool fail(const QString &message);

    DatabaseManager &database;    // created in main(); used, not owned
    Session &currentSession;      // created in main(); only AuthService changes it
    UserStore users;              // the account SQL (composition)
    const PasswordHasher &passwordHasher;
    const PasswordHasher &recoveryCodeHasher;
    std::vector<UnknownEmailTries> unknownEmails;
    std::string dummyHash;        // checked against for unknown emails, so they take the same time
    QString errorMessage;
};

#endif // FINORA_AUTHSERVICE_H
