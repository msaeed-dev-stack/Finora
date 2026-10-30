// =====================================================================
//  AuthService.cpp
// =====================================================================
#include "AuthService.h"

#include "core/AccountRules.h"
#include "core/Money.h"            // trimSpaces
#include "core/PasswordPolicy.h"

const QString NOT_ENOUGH_MEMORY_MESSAGE =
    "Finora could not get enough memory to protect your password safely. "
    "Close some other programs and try again.";

AuthService::AuthService(DatabaseManager &sharedDatabase, Session &sharedSession,
                         const PasswordHasher &sharedPasswordHasher, const PasswordHasher &sharedRecoveryCodeHasher)
    : database(sharedDatabase), currentSession(sharedSession), users(sharedDatabase),
      passwordHasher(sharedPasswordHasher), recoveryCodeHasher(sharedRecoveryCodeHasher)
{
    // Sessions left open by a crash are closed when Finora starts.
    users.endInterruptedSessions();
}

const Session &AuthService::session() const
{
    return currentSession;
}

QString AuthService::lastError() const
{
    return errorMessage;
}

bool AuthService::hasAnyAccount()
{
    int count = 0;
    return users.countAccounts(count) && count > 0;
}

// ---- Registration and sign-in ---------------------------------------------

bool AuthService::signUp(const std::string &displayName, const std::string &email, std::string &password,
                         std::vector<std::string> &recoveryCodes)
{
    errorMessage.clear();
    recoveryCodes.clear();
    std::string name = trimSpaces(displayName);
    std::string address = normalizeEmail(email);

    std::string problem = validateDisplayName(name);
    if (problem.empty()) {
        problem = validateEmail(address);
    }
    if (problem.empty()) {
        problem = validateNewPassword(password, address, name);
    }
    if (!problem.empty()) {
        PasswordHasher::wipe(password);
        return fail(QString::fromStdString(problem));
    }

    UserAccount existing;
    bool found = false;
    if (!users.findByEmail(address, existing, found)) {
        PasswordHasher::wipe(password);
        return fail(users.lastError());
    }
    if (found) {
        PasswordHasher::wipe(password);
        return fail("An account with this email address already exists on this computer. Please sign in instead.");
    }

    // Hash first: if the memory is missing, no account is created at all.
    UserAccount account;
    account.email = address;
    account.displayName = name;
    bool hashed = passwordHasher.hash(password, account.passwordHash);
    PasswordHasher::wipe(password);
    if (!hashed) {
        return fail(NOT_ENOUGH_MEMORY_MESSAGE);
    }

    std::vector<std::string> codeHashes;
    if (!makeRecoveryCodes(recoveryCodes, codeHashes)) {
        return false;
    }
    if (!users.createAccount(account, codeHashes)) {
        recoveryCodes.clear();
        return fail(users.lastError());
    }

    if (currentSession.isSignedIn()) {
        signOut("signed_out");
    }
    return startSessionFor(account, "after sign-up");
}

bool AuthService::signIn(const std::string &email, std::string &password)
{
    errorMessage.clear();
    std::string address = normalizeEmail(email);

    UserAccount account;
    bool found = false;
    if (!users.findByEmail(address, account, found)) {
        PasswordHasher::wipe(password);
        return fail(users.lastError());
    }

    if (!found) {
        // No such account: do the same work and give the same answer as
        // for a wrong password, so the email's existence stays hidden.
        int wait = 0;
        if (unknownEmailMustWait(address, wait)) {
            PasswordHasher::wipe(password);
            return fail(tooManyAttemptsMessage(wait));
        }
        passwordHasher.verify(dummyPasswordHash(), password);
        PasswordHasher::wipe(password);
        return failForUnknownEmail(address, SIGN_IN_FAILED_MESSAGE);
    }

    // During a waiting time, not even the right password is checked.
    int wait = secondsToWait(account.lockedUntil);
    if (wait > 0) {
        PasswordHasher::wipe(password);
        return fail(tooManyAttemptsMessage(wait));
    }

    if (!passwordHasher.verify(account.passwordHash, password)) {
        PasswordHasher::wipe(password);
        registerFailure(account, "sign_in_failed");
        return fail(SIGN_IN_FAILED_MESSAGE);
    }

    // Right password. A hash made with older, weaker settings is replaced
    // now, while the password is known.
    if (passwordHasher.needsRehash(account.passwordHash)) {
        std::string newHash;
        if (passwordHasher.hash(password, newHash) && users.updatePasswordHash(account.id, newHash)) {
            users.recordEvent(account.id, "password_rehashed", "");
        }
    }
    PasswordHasher::wipe(password);

    if (currentSession.isSignedIn()) {
        signOut("signed_out");
    }
    if (!users.recordSuccessfulSignIn(account.id)) {
        return fail(users.lastError());
    }
    return startSessionFor(account, "");
}

void AuthService::signOut(const std::string &reason)
{
    if (!currentSession.isSignedIn()) {
        return;
    }
    users.endSession(currentSession.sessionId(), reason);
    users.recordEvent(currentSession.userId(), "signed_out", reason);
    currentSession.end();
}

// ---- Sessions ------------------------------------------------------------------

bool AuthService::keepSessionAlive()
{
    if (!currentSession.isSignedIn()) {
        return false;
    }
    std::string expiresAt;
    if (users.sessionExpiresAt(currentSession.sessionId(), expiresAt) && secondsToWait(expiresAt) == 0) {
        signOut("expired");
        return fail("Your session reached its " + QString::number(SESSION_MAX_HOURS)
                    + "-hour limit. Please sign in again.");
    }
    users.touchSession(currentSession.sessionId());
    return true;
}

bool AuthService::endSessionIfIdle(long long idleSeconds, long long limitSeconds)
{
    if (!currentSession.isSignedIn() || idleSeconds < limitSeconds) {
        return false;
    }
    signOut("idle");
    return true;
}

// ---- Profile and password ------------------------------------------------------

bool AuthService::signedInAccount(UserAccount &account)
{
    if (!currentSession.isSignedIn()) {
        return fail("Please sign in first.");
    }
    if (!users.findById(currentSession.userId(), account)) {
        return fail(users.lastError());
    }
    account.passwordHash.clear();   // the interface never needs the hash
    return true;
}

bool AuthService::updateProfile(const std::string &displayName, const std::string &email,
                                std::string &currentPassword)
{
    errorMessage.clear();
    if (!currentSession.isSignedIn()) {
        PasswordHasher::wipe(currentPassword);
        return fail("Please sign in first.");
    }
    std::string name = trimSpaces(displayName);
    std::string address = normalizeEmail(email);
    std::string problem = validateDisplayName(name);
    if (problem.empty()) {
        problem = validateEmail(address);
    }
    if (!problem.empty()) {
        PasswordHasher::wipe(currentPassword);
        return fail(QString::fromStdString(problem));
    }

    UserAccount account;
    if (!users.findById(currentSession.userId(), account)) {
        PasswordHasher::wipe(currentPassword);
        return fail(users.lastError());
    }

    // The email is what signs a person in, so changing it needs the password.
    bool emailChanged = (address != account.email);
    if (emailChanged) {
        if (!verifyCurrentPassword(account, currentPassword)) {
            return false;
        }
        bool used = false;
        if (!users.isEmailUsedByOther(address, account.id, used)) {
            return fail(users.lastError());
        }
        if (used) {
            return fail("Another account on this computer already uses this email address.");
        }
    }
    PasswordHasher::wipe(currentPassword);

    if (!users.updateProfile(account.id, address, name)) {
        return fail(users.lastError());
    }
    std::string changed = "name";
    if (emailChanged) {
        changed = "name and email";
    }
    users.recordEvent(account.id, "profile_updated", changed);
    currentSession.updateProfile(address, name);
    return true;
}

bool AuthService::changePassword(std::string &currentPassword, std::string &newPassword)
{
    errorMessage.clear();
    UserAccount account;
    if (!currentSession.isSignedIn() || !users.findById(currentSession.userId(), account)) {
        PasswordHasher::wipe(currentPassword);
        PasswordHasher::wipe(newPassword);
        return fail("Please sign in first.");
    }
    if (!verifyCurrentPassword(account, currentPassword)) {
        PasswordHasher::wipe(newPassword);
        return false;
    }

    std::string problem = validateNewPassword(newPassword, account.email, account.displayName);
    if (problem.empty() && newPassword == currentPassword) {
        problem = "The new password must be different from the current one.";
    }
    PasswordHasher::wipe(currentPassword);
    if (!problem.empty()) {
        PasswordHasher::wipe(newPassword);
        return fail(QString::fromStdString(problem));
    }

    std::string newHash;
    bool hashed = passwordHasher.hash(newPassword, newHash);
    PasswordHasher::wipe(newPassword);
    if (!hashed) {
        return fail(NOT_ENOUGH_MEMORY_MESSAGE);
    }
    if (!users.updatePasswordHash(account.id, newHash)) {
        return fail(users.lastError());
    }
    users.recordEvent(account.id, "password_changed", "");
    return true;
}

// ---- Recovery codes --------------------------------------------------------------

bool AuthService::recoveryCodesLeft(int &count)
{
    count = 0;
    if (!currentSession.isSignedIn()) {
        return fail("Please sign in first.");
    }
    if (!users.countUnusedRecoveryCodes(currentSession.userId(), count)) {
        return fail(users.lastError());
    }
    return true;
}

bool AuthService::regenerateRecoveryCodes(std::string &currentPassword, std::vector<std::string> &recoveryCodes)
{
    errorMessage.clear();
    recoveryCodes.clear();
    UserAccount account;
    if (!currentSession.isSignedIn() || !users.findById(currentSession.userId(), account)) {
        PasswordHasher::wipe(currentPassword);
        return fail("Please sign in first.");
    }
    if (!verifyCurrentPassword(account, currentPassword)) {
        return false;
    }
    PasswordHasher::wipe(currentPassword);

    std::vector<std::string> codeHashes;
    if (!makeRecoveryCodes(recoveryCodes, codeHashes)) {
        return false;
    }
    if (!users.replaceRecoveryCodes(account.id, codeHashes)) {
        recoveryCodes.clear();
        return fail(users.lastError());
    }
    return true;
}

bool AuthService::resetPasswordWithRecoveryCode(const std::string &email, std::string &recoveryCode,
                                                std::string &newPassword, int &codesLeft)
{
    errorMessage.clear();
    codesLeft = 0;
    std::string address = normalizeEmail(email);
    std::string code = normalizeRecoveryCode(recoveryCode);
    PasswordHasher::wipe(recoveryCode);

    // The password rules are checked first and without the account, so
    // this message never tells whether the email has an account.
    std::string problem = validateNewPassword(newPassword, address, "");
    if (!problem.empty()) {
        PasswordHasher::wipe(code);
        PasswordHasher::wipe(newPassword);
        return fail(QString::fromStdString(problem));
    }

    UserAccount account;
    bool found = false;
    if (!users.findByEmail(address, account, found)) {
        PasswordHasher::wipe(code);
        PasswordHasher::wipe(newPassword);
        return fail(users.lastError());
    }
    if (!found) {
        int wait = 0;
        bool mustWait = unknownEmailMustWait(address, wait);
        if (!mustWait) {
            recoveryCodeHasher.verify(dummyPasswordHash(), code);   // the same work as a real check
        }
        PasswordHasher::wipe(code);
        PasswordHasher::wipe(newPassword);
        if (mustWait) {
            return fail(tooManyAttemptsMessage(wait));
        }
        return failForUnknownEmail(address, RECOVERY_FAILED_MESSAGE);
    }

    int wait = secondsToWait(account.lockedUntil);
    if (wait > 0) {
        PasswordHasher::wipe(code);
        PasswordHasher::wipe(newPassword);
        return fail(tooManyAttemptsMessage(wait));
    }

    // Compare the typed code with every unused code of this account.
    int matchingCodeId = 0;
    std::vector<StoredRecoveryCode> storedCodes;
    if (!code.empty() && users.loadUnusedRecoveryCodes(account.id, storedCodes)) {
        int count = static_cast<int>(storedCodes.size());
        for (int i = 0; i < count && matchingCodeId == 0; i++) {
            if (recoveryCodeHasher.verify(storedCodes[i].codeHash, code)) {
                matchingCodeId = storedCodes[i].id;
            }
        }
    }
    PasswordHasher::wipe(code);
    if (matchingCodeId == 0) {
        PasswordHasher::wipe(newPassword);
        registerFailure(account, "recovery_failed");
        return fail(RECOVERY_FAILED_MESSAGE);
    }

    std::string newHash;
    bool hashed = passwordHasher.hash(newPassword, newHash);
    PasswordHasher::wipe(newPassword);
    if (!hashed) {
        return fail(NOT_ENOUGH_MEMORY_MESSAGE);
    }
    if (!users.completePasswordReset(account.id, matchingCodeId, newHash)) {
        return fail(users.lastError());
    }
    if (currentSession.isSignedIn() && currentSession.userId() == account.id) {
        currentSession.end();   // completePasswordReset already ended the session in the database
    }
    users.countUnusedRecoveryCodes(account.id, codesLeft);
    return true;
}

// ---- Records made before accounts existed ----------------------------------------

bool AuthService::countRecordsWithoutOwner(int &transactions, int &budgets)
{
    if (!users.countRecordsWithoutOwner(transactions, budgets)) {
        return fail(users.lastError());
    }
    return true;
}

bool AuthService::claimRecordsWithoutOwner(int &movedTransactions, int &movedBudgets, int &keptBudgets)
{
    if (!currentSession.isSignedIn()) {
        return fail("Please sign in first.");
    }
    if (!users.claimRecordsWithoutOwner(currentSession.userId(), movedTransactions, movedBudgets, keptBudgets)) {
        return fail(users.lastError());
    }
    return true;
}

// ---- Helpers ---------------------------------------------------------------------

// One more wrong try: counts it and, from the 3rd one on, sets a waiting time.
bool AuthService::registerFailure(const UserAccount &account, const std::string &eventType)
{
    int failures = account.failedSignIns + 1;
    int delay = signInDelaySeconds(failures);
    std::string waitUntil;
    if (delay > 0) {
        waitUntil = QDateTime::currentDateTimeUtc().addSecs(delay).toString(Qt::ISODate).toStdString();
    }
    users.recordEvent(account.id, eventType, "failure " + std::to_string(failures));
    return users.recordFailedSignIn(account.id, failures, waitUntil);
}

// The same counting for an email without an account, kept in memory only.
bool AuthService::failForUnknownEmail(const std::string &email, const QString &message)
{
    int index = -1;
    int count = static_cast<int>(unknownEmails.size());
    for (int i = 0; i < count; i++) {
        if (unknownEmails[i].email == email) {
            index = i;
        }
    }
    if (index < 0) {
        UnknownEmailTries tries;
        tries.email = email;
        unknownEmails.push_back(tries);
        index = count;
    }
    unknownEmails[index].failures++;
    int delay = signInDelaySeconds(unknownEmails[index].failures);
    if (delay > 0) {
        unknownEmails[index].waitUntil = QDateTime::currentDateTimeUtc().addSecs(delay);
    }
    return fail(message);
}

bool AuthService::unknownEmailMustWait(const std::string &email, int &seconds)
{
    seconds = 0;
    int count = static_cast<int>(unknownEmails.size());
    for (int i = 0; i < count; i++) {
        if (unknownEmails[i].email == email && unknownEmails[i].waitUntil.isValid()) {
            long long left = QDateTime::currentDateTimeUtc().secsTo(unknownEmails[i].waitUntil);
            if (left > 0) {
                seconds = static_cast<int>(left);
                return true;
            }
        }
    }
    return false;
}

// Makes 10 new codes. plainCodes gets "7KQ4-M2TX-9RHC" for showing once;
// codeHashes gets their Argon2id hashes for storing.
bool AuthService::makeRecoveryCodes(std::vector<std::string> &plainCodes, std::vector<std::string> &codeHashes)
{
    plainCodes.clear();
    codeHashes.clear();
    for (int i = 0; i < RECOVERY_CODE_COUNT; i++) {
        std::string code = PasswordHasher::newRecoveryCode();
        std::string codeHash;
        if (!recoveryCodeHasher.hash(code, codeHash)) {
            PasswordHasher::wipe(code);
            plainCodes.clear();
            return fail(NOT_ENOUGH_MEMORY_MESSAGE);
        }
        plainCodes.push_back(formatRecoveryCode(code));
        codeHashes.push_back(codeHash);
        PasswordHasher::wipe(code);
    }
    return true;
}

// Checks the current password before a sensitive change. Wrong tries
// count towards the same waiting time as wrong sign-ins.
bool AuthService::verifyCurrentPassword(const UserAccount &account, std::string &currentPassword)
{
    int wait = secondsToWait(account.lockedUntil);
    if (wait > 0) {
        PasswordHasher::wipe(currentPassword);
        return fail(tooManyAttemptsMessage(wait));
    }
    if (!passwordHasher.verify(account.passwordHash, currentPassword)) {
        PasswordHasher::wipe(currentPassword);
        registerFailure(account, "password_check_failed");
        return fail("The current password is not correct.");
    }
    return true;
}

bool AuthService::startSessionFor(const UserAccount &account, const std::string &eventDetails)
{
    int sessionId = 0;
    if (!users.startSession(account.id, sessionId)) {
        return fail(users.lastError());
    }
    users.recordEvent(account.id, "signed_in", eventDetails);
    currentSession.start(account.id, sessionId, account.email, account.displayName);
    return true;
}

// A real Argon2id hash of a random secret nobody knows. Checking a password
// against it costs the same time as checking a real account.
const std::string &AuthService::dummyPasswordHash()
{
    if (dummyHash.empty()) {
        std::string secret = PasswordHasher::newRecoveryCode() + PasswordHasher::newRecoveryCode();
        passwordHasher.hash(secret, dummyHash);
        PasswordHasher::wipe(secret);
    }
    return dummyHash;
}

int AuthService::secondsToWait(const std::string &waitUntilText) const
{
    if (waitUntilText.empty()) {
        return 0;
    }
    QDateTime waitUntil = QDateTime::fromString(QString::fromStdString(waitUntilText), Qt::ISODate);
    if (!waitUntil.isValid()) {
        return 0;
    }
    long long seconds = QDateTime::currentDateTimeUtc().secsTo(waitUntil);
    if (seconds <= 0) {
        return 0;
    }
    return static_cast<int>(seconds);
}

QString AuthService::tooManyAttemptsMessage(int seconds) const
{
    if (seconds >= 120) {
        int minutes = (seconds + 59) / 60;   // rounded up
        return "Too many attempts. Please wait " + QString::number(minutes) + " minutes and try again.";
    }
    return "Too many attempts. Please wait " + QString::number(seconds) + " seconds and try again.";
}

bool AuthService::fail(const QString &message)
{
    errorMessage = message;
    return false;
}
