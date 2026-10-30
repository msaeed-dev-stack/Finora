// =====================================================================
//  finora_auth_tests.cpp
//  Automated checks for accounts and security (Milestone 4, Phase 4B):
//  Argon2id password hashing with libsodium, its memory use, recovery
//  codes and wiping secrets from memory.
//
//  Every database lives in a temporary folder that QTemporaryDir deletes
//  at the end, so the tests never touch the real Finora database.
// =====================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlField>
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
#include "core/AccountRules.h"
#include "core/Budget.h"
#include "core/Session.h"
#include "core/Transaction.h"
#include "database/BudgetStore.h"
#include "database/DatabaseManager.h"

// Counts how many checks ran and how many of them failed.
struct TestCounter {
    int checksRun = 0;
    int checksFailed = 0;
};

void check(TestCounter &counter, const std::string &testName, bool passed)
{
    counter.checksRun++;
    if (passed) {
        std::cout << "PASS  " << testName << "\n";
    } else {
        counter.checksFailed++;
        std::cout << "FAIL  " << testName << "\n";
    }
}

const std::size_t MEBIBYTE = 1024 * 1024;

// The most memory this program has used so far, and what it uses now (Windows).
std::size_t peakMemoryBytes()
{
    PROCESS_MEMORY_COUNTERS counters;
    GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters);
    return counters.PeakWorkingSetSize;
}

std::size_t currentMemoryBytes()
{
    PROCESS_MEMORY_COUNTERS counters;
    GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters);
    return counters.WorkingSetSize;
}

// ---- Argon2id with the settings Finora uses for passwords -----------------

// Runs FIRST, while the program still uses little memory, so the jump in
// peak memory shows the 256 MiB that one MODERATE hash really needs.
void testModerateHashingMemory(TestCounter &counter)
{
    PasswordHasher moderate(HASH_MODERATE);
    std::size_t peakBefore = peakMemoryBytes();

    QElapsedTimer timer;
    timer.start();
    std::string stored;
    bool hashed = moderate.hash("Correct Horse 9!", stored);
    qint64 hashMs = timer.elapsed();
    std::size_t peakAfter = peakMemoryBytes();
    std::size_t currentAfter = currentMemoryBytes();

    timer.restart();
    bool verified = moderate.verify(stored, "Correct Horse 9!");
    qint64 verifyMs = timer.elapsed();

    check(counter, "a MODERATE hash succeeds", hashed);
    check(counter, "the stored string records Argon2id, 256 MiB (m=262144 KiB), 3 passes, 1 lane",
          stored.rfind("$argon2id$v=19$m=262144,t=3,p=1$", 0) == 0);
    check(counter, "the MODERATE hash verifies", verified);
    check(counter, "peak memory rose by at least 240 MiB while hashing (256 MiB are needed)",
          peakAfter >= peakBefore + 240 * MEBIBYTE);
    check(counter, "the memory is given back after hashing", currentAfter + 200 * MEBIBYTE < peakAfter);
    std::cout << "        memory: peak " << peakBefore / MEBIBYTE << " MiB before, " << peakAfter / MEBIBYTE
              << " MiB during, " << currentAfter / MEBIBYTE << " MiB after; time: hash " << hashMs
              << " ms, verify " << verifyMs << " ms\n";
}

void testPasswordHasher(TestCounter &counter)
{
    check(counter, "libsodium version is 1.0.22", PasswordHasher::libraryVersion() == "1.0.22");

    PasswordHasher fast(HASH_INTERACTIVE);
    PasswordHasher moderate(HASH_MODERATE);
    check(counter, "MODERATE is 3 passes over 256 MiB",
          moderate.passes() == 3 && moderate.memoryBytes() == 256 * MEBIBYTE);
    check(counter, "INTERACTIVE is 2 passes over 64 MiB", fast.passes() == 2 && fast.memoryBytes() == 64 * MEBIBYTE);

    std::string first;
    std::string second;
    fast.hash("Correct Horse 9!", first);
    fast.hash("Correct Horse 9!", second);
    check(counter, "two hashes of the same password differ: every hash has its own random salt", first != second);
    check(counter, "the stored string contains no part of the password",
          first.find("Horse") == std::string::npos && first.find("Correct") == std::string::npos);
    check(counter, "the right password verifies against both", fast.verify(first, "Correct Horse 9!")
                                                                    && fast.verify(second, "Correct Horse 9!"));
    check(counter, "a wrong password is refused", !fast.verify(first, "correct Horse 9!"));
    check(counter, "an empty password is refused", !fast.verify(first, ""));
    check(counter, "a damaged stored string never verifies", !fast.verify("not a hash", "Correct Horse 9!"));

    check(counter, "an INTERACTIVE hash needs a rehash for the MODERATE settings", moderate.needsRehash(first));
    check(counter, "but not for the INTERACTIVE settings", !fast.needsRehash(first));

    // More memory than any normal computer can give: the hash must fail
    // cleanly, without crashing and without leaving a half-made value.
    PasswordHasher impossible(3, static_cast<std::size_t>(512) * 1024 * MEBIBYTE);   // 512 GiB
    std::string failed = "old value";
    bool hashed = impossible.hash("Correct Horse 9!", failed);
    check(counter, "a hash that needs 512 GiB fails safely and returns no hash", !hashed && failed.empty());
}

void testRecoveryCodesAndWiping(TestCounter &counter)
{
    std::vector<std::string> codes;
    bool allValid = true;
    for (int i = 0; i < 200; i++) {
        std::string code = PasswordHasher::newRecoveryCode();
        allValid = allValid && code.length() == 12 && normalizeRecoveryCode(code) == code;
        codes.push_back(code);
    }
    bool allDifferent = true;
    for (int i = 0; i < static_cast<int>(codes.size()); i++) {
        for (int j = i + 1; j < static_cast<int>(codes.size()); j++) {
            allDifferent = allDifferent && codes[i] != codes[j];
        }
    }
    check(counter, "200 new recovery codes are 12 characters from the recovery alphabet", allValid);
    check(counter, "and all 200 are different", allDifferent);

    std::string secret = "a password to forget";
    std::string::size_type length = secret.length();
    const char *bytes = secret.data();
    PasswordHasher::wipe(secret);
    // GCC's std::string keeps its memory after clear(), so the old bytes
    // can still be looked at through the saved pointer.
    bool allZero = true;
    for (std::string::size_type i = 0; i < length; i++) {
        allZero = allZero && bytes[i] == 0;
    }
    check(counter, "wipe() empties the string", secret.empty());
    check(counter, "wipe() overwrote every byte of the old password with zero", allZero);
}

// =====================================================================
//  Accounts on real SQLite files
// =====================================================================

// The password every test account uses, unless a test says otherwise.
const std::string GOOD_PASSWORD = "Correct Horse 9!";

int numberFromSql(const QString &connection, const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connection, false));
    if (!query.exec(sql) || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

QString textFromSql(const QString &connection, const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connection, false));
    if (!query.exec(sql) || !query.next()) {
        return QString();
    }
    return query.value(0).toString();
}

bool runSql(const QString &connection, const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(connection, false));
    return query.exec(sql);
}

// True if `secret` appears anywhere in the database: in any column of any
// row of any table.
bool appearsInAnyTable(const QString &connection, const std::string &secret)
{
    QSqlDatabase db = QSqlDatabase::database(connection, false);
    QStringList tables = db.tables();
    for (int t = 0; t < tables.size(); t++) {
        QSqlRecord columns = db.record(tables[t]);
        for (int c = 0; c < columns.count(); c++) {
            QSqlQuery query(db);
            query.prepare("SELECT COUNT(*) FROM \"" + tables[t] + "\" WHERE CAST(\"" + columns.fieldName(c)
                          + "\" AS TEXT) LIKE '%' || :secret || '%'");
            query.bindValue(":secret", QString::fromStdString(secret));
            if (query.exec() && query.next() && query.value(0).toInt() > 0) {
                return true;
            }
        }
    }
    return false;
}

// True if the bytes of `secret` appear anywhere in the database FILE,
// including space that SQLite keeps for later use.
bool fileContains(const QString &path, const std::string &secret)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return true;   // cannot prove the opposite
    }
    QByteArray bytes = file.readAll();
    file.close();
    return bytes.contains(QByteArray::fromStdString(secret));
}

Transaction makeTransaction(const std::string &type, long long amountMinor, const std::string &category,
                            const std::string &description)
{
    Transaction transaction;
    transaction.type = type;
    transaction.amountMinor = amountMinor;
    transaction.currencyCode = "PKR";
    transaction.category = category;
    transaction.description = description;
    transaction.date = "2026-10-15";
    return transaction;
}

// A database for one test: its own file, a Session attached to it, and
// an AuthService with the given hashers.
struct TestAccounts {
    DatabaseManager database;
    Session session;
    AuthService auth;
    QString connection;
    QString path;

    TestAccounts(const QString &name, const QString &folder, const PasswordHasher &passwords,
                 const PasswordHasher &codes)
        : database(name), auth(database, session, passwords, codes), connection(name),
          path(folder + "/" + name + ".db")
    {
        database.open(path);
        database.attachSession(session);
    }
};

// Creates an account and signs out again. Returns the recovery codes.
std::vector<std::string> createAccount(AuthService &auth, const std::string &name, const std::string &email)
{
    std::vector<std::string> codes;
    std::string password = GOOD_PASSWORD;
    auth.signUp(name, email, password, codes);
    auth.signOut("signed_out");
    return codes;
}

bool signIn(AuthService &auth, const std::string &email, const std::string &passwordText)
{
    std::string password = passwordText;
    return auth.signIn(email, password);
}

void testSignUp(TestCounter &counter, const QString &folder, const PasswordHasher &fast)
{
    TestAccounts accounts("auth_signup", folder, fast, fast);
    AuthService &auth = accounts.auth;
    check(counter, "a new database has no account yet", !auth.hasAnyAccount());
    accounts.database.saveSetting("theme", "dark");   // nobody signed in: the installation's theme

    std::vector<std::string> codes;
    std::string common = "Password123";
    check(counter, "sign-up refuses a common password with a clear reason",
          !auth.signUp("Muhammad Saeed", "saeed@example.com", common, codes)
              && auth.lastError() == "This password is too common. Please choose another one.");
    check(counter, "the refused password was wiped from memory", common.empty());
    check(counter, "nothing was created for the refused sign-up",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM users") == 0 && !accounts.session.isSignedIn());

    std::string password = GOOD_PASSWORD;
    bool created = auth.signUp("  Muhammad Saeed ", "  Saeed@Example.COM ", password, codes);
    check(counter, "a valid sign-up creates the account and signs the person in",
          created && accounts.session.isSignedIn() && auth.hasAnyAccount());
    check(counter, "the password variable was wiped right after hashing", password.empty());
    check(counter, "the email is stored trimmed and in small letters",
          accounts.session.email() == "saeed@example.com");
    check(counter, "the name is trimmed, and the first name is used for greetings",
          accounts.session.displayName() == "Muhammad Saeed" && accounts.session.firstName() == "Muhammad");

    bool allFormatted = codes.size() == 10;
    bool allDifferent = true;
    for (int i = 0; i < static_cast<int>(codes.size()); i++) {
        allFormatted = allFormatted && codes[i].length() == 14 && codes[i][4] == '-' && codes[i][9] == '-'
                       && normalizeRecoveryCode(codes[i]).length() == 12;
        for (int j = i + 1; j < static_cast<int>(codes.size()); j++) {
            allDifferent = allDifferent && codes[i] != codes[j];
        }
    }
    check(counter, "10 recovery codes are shown once, as XXXX-XXXX-XXXX", allFormatted);
    check(counter, "the 10 recovery codes are all different", allDifferent);

    check(counter, "the stored password is an Argon2id hash",
          textFromSql(accounts.connection, "SELECT password_hash FROM users").startsWith("$argon2id$v=19$"));
    check(counter, "the password appears in no table and no column of the database",
          !appearsInAnyTable(accounts.connection, GOOD_PASSWORD));
    check(counter, "the password bytes appear nowhere in the database file either",
          !fileContains(accounts.path, GOOD_PASSWORD));
    bool noPlainCode = true;
    for (int i = 0; i < static_cast<int>(codes.size()); i++) {
        noPlainCode = noPlainCode && !appearsInAnyTable(accounts.connection, codes[i])
                      && !appearsInAnyTable(accounts.connection, normalizeRecoveryCode(codes[i]))
                      && !fileContains(accounts.path, normalizeRecoveryCode(codes[i]));
    }
    check(counter, "no recovery code is stored in plain text, in any form", noPlainCode);
    check(counter, "10 unused recovery code hashes are stored",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM recovery_codes "
                                             "WHERE used_at IS NULL AND code_hash LIKE '$argon2id$%'") == 10);
    check(counter, "the new account starts with the theme that was in use (dark)",
          accounts.database.setting("theme", "light") == "dark");
    check(counter, "the sign-up is in the audit history, and a session is open",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM audit_events WHERE event_type = 'account_created'") == 1
              && numberFromSql(accounts.connection, "SELECT COUNT(*) FROM login_sessions WHERE ended_at IS NULL") == 1);

    auth.signOut("signed_out");
    std::string samePassword = GOOD_PASSWORD;
    auth.signUp("Ayesha Khan", "ayesha@example.com", samePassword, codes);
    check(counter, "two accounts with the same password get different hashes (unique salts)",
          numberFromSql(accounts.connection, "SELECT COUNT(DISTINCT password_hash) FROM users") == 2);

    auth.signOut("signed_out");
    std::string another = "Another Pass 77!";
    check(counter, "the same email again, in other letters, is refused",
          !auth.signUp("Someone", "SAEED@example.com", another, codes)
              && auth.lastError().startsWith("An account with this email address already exists"));
}

void testSignInAndDelays(TestCounter &counter, const QString &folder, const PasswordHasher &fast)
{
    TestAccounts accounts("auth_signin", folder, fast, fast);
    AuthService &auth = accounts.auth;
    createAccount(auth, "Ali Raza", "ali@example.com");

    check(counter, "a wrong password is refused with the general message",
          !signIn(auth, "ali@example.com", "Wrong Horse 9!") && auth.lastError() == SIGN_IN_FAILED_MESSAGE
              && !accounts.session.isSignedIn());
    check(counter, "an unknown email gets exactly the same message",
          !signIn(auth, "nobody@example.com", GOOD_PASSWORD) && auth.lastError() == SIGN_IN_FAILED_MESSAGE);

    signIn(auth, "ali@example.com", "Wrong Horse 8!");
    signIn(auth, "ali@example.com", "Wrong Horse 7!");
    check(counter, "after 3 wrong passwords even the right one has to wait",
          !signIn(auth, "ali@example.com", GOOD_PASSWORD)
              && auth.lastError().startsWith("Too many attempts. Please wait"));
    check(counter, "the 3 failures and the waiting time are saved",
          numberFromSql(accounts.connection, "SELECT failed_login_count FROM users") == 3
              && numberFromSql(accounts.connection, "SELECT COUNT(*) FROM users WHERE locked_until IS NOT NULL") == 1);
    std::cout << "        message: " << auth.lastError().toStdString() << "\n";

    signIn(auth, "nobody@example.com", "x1");
    signIn(auth, "nobody@example.com", "x2");
    check(counter, "an unknown email has to wait after 3 tries too, with the same message",
          !signIn(auth, "nobody@example.com", "x3") && auth.lastError().startsWith("Too many attempts. Please wait"));

    runSql(accounts.connection, "UPDATE users SET locked_until = '2000-01-01T00:00:00Z'");   // the wait is over
    check(counter, "after the waiting time the right password signs in (email typed differently)",
          signIn(auth, " ALI@example.com", GOOD_PASSWORD) && accounts.session.isSignedIn());
    check(counter, "a successful sign-in clears the counter and records the time",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM users WHERE failed_login_count = 0 "
                                             "AND locked_until IS NULL AND last_login_at IS NOT NULL") == 1);
    check(counter, "every wrong password is in the audit history",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM audit_events WHERE event_type = 'sign_in_failed'") == 3);

    // ---- Sessions ----
    int firstSession = accounts.session.sessionId();
    check(counter, "1 minute without activity keeps the session", !auth.endSessionIfIdle(60) && accounts.session.isSignedIn());
    check(counter, "15 minutes without activity signs out", auth.endSessionIfIdle(15 * 60) && !accounts.session.isSignedIn());
    check(counter, "the session is closed with the reason 'idle'",
          textFromSql(accounts.connection, "SELECT end_reason FROM login_sessions WHERE id = "
                                           + QString::number(firstSession)) == "idle");

    signIn(auth, "ali@example.com", GOOD_PASSWORD);
    check(counter, "an active session stays alive", auth.keepSessionAlive() && accounts.session.isSignedIn());
    runSql(accounts.connection, "UPDATE login_sessions SET expires_at = '2000-01-01T00:00:00Z' WHERE ended_at IS NULL");
    check(counter, "a session past its 12-hour limit is ended", !auth.keepSessionAlive() && !accounts.session.isSignedIn()
              && numberFromSql(accounts.connection, "SELECT COUNT(*) FROM login_sessions WHERE end_reason = 'expired'") == 1);

    signIn(auth, "ali@example.com", GOOD_PASSWORD);
    int lastSession = accounts.session.sessionId();
    auth.signOut("signed_out");
    check(counter, "signing out closes this session with the reason 'signed_out'",
          !accounts.session.isSignedIn()
              && textFromSql(accounts.connection, "SELECT end_reason FROM login_sessions WHERE id = "
                                                  + QString::number(lastSession)) == "signed_out");

    // A crash leaves a session open; the next start closes it.
    signIn(auth, "ali@example.com", GOOD_PASSWORD);
    Session afterRestart;
    AuthService restarted(accounts.database, afterRestart, fast, fast);
    check(counter, "a session left open by a crash is closed at the next start ('interrupted')",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM login_sessions WHERE end_reason = 'interrupted'") == 1);
}

void testProfileAndPassword(TestCounter &counter, const QString &folder, const PasswordHasher &fast)
{
    TestAccounts accounts("auth_profile", folder, fast, fast);
    AuthService &auth = accounts.auth;
    createAccount(auth, "Sara Ahmed", "sara@example.com");
    createAccount(auth, "Ali Raza", "ali@example.com");
    signIn(auth, "ali@example.com", GOOD_PASSWORD);

    std::string noPassword;
    check(counter, "the name can change without the password",
          auth.updateProfile("Ali Raza Khan", "ali@example.com", noPassword)
              && accounts.session.displayName() == "Ali Raza Khan");
    std::string wrong = "Not My Pass 1";
    check(counter, "a new email needs the right current password",
          !auth.updateProfile("Ali Raza Khan", "ali.raza@example.com", wrong)
              && auth.lastError() == "The current password is not correct.");
    std::string right = GOOD_PASSWORD;
    check(counter, "an email that another account uses is refused",
          !auth.updateProfile("Ali Raza Khan", "sara@example.com", right)
              && auth.lastError() == "Another account on this computer already uses this email address.");
    right = GOOD_PASSWORD;
    check(counter, "with the right password the email changes",
          auth.updateProfile("Ali Raza Khan", "Ali.Raza@example.com", right)
              && accounts.session.email() == "ali.raza@example.com");
    runSql(accounts.connection, "UPDATE users SET failed_login_count = 0, locked_until = NULL");

    std::string current = "Not My Pass 1";
    std::string newPassword = "Brand New Pass 42!";
    check(counter, "changing the password needs the right current password",
          !auth.changePassword(current, newPassword) && auth.lastError() == "The current password is not correct.");
    current = GOOD_PASSWORD;
    newPassword = "shortpass";
    check(counter, "a too short new password is refused", !auth.changePassword(current, newPassword)
                                                              && auth.lastError() == "Use at least 10 characters.");
    current = GOOD_PASSWORD;
    newPassword = "Brand New Pass 42!";
    check(counter, "the password changes, and both variables are wiped",
          auth.changePassword(current, newPassword) && current.empty() && newPassword.empty());
    auth.signOut("signed_out");
    check(counter, "the old password no longer signs in", !signIn(auth, "ali.raza@example.com", GOOD_PASSWORD));
    check(counter, "the new password signs in", signIn(auth, "ali.raza@example.com", "Brand New Pass 42!"));
}

void testRecovery(TestCounter &counter, const QString &folder, const PasswordHasher &fast)
{
    TestAccounts accounts("auth_recovery", folder, fast, fast);
    AuthService &auth = accounts.auth;
    std::vector<std::string> hinaCodes = createAccount(auth, "Hina Shah", "hina@example.com");
    std::vector<std::string> omarCodes = createAccount(auth, "Omar Farooq", "omar@example.com");
    int left = 0;

    std::string code = "ABCD-EFGH-JKLM";
    std::string newPassword = "Brand New Pass 42!";
    check(counter, "a wrong recovery code gives the general message",
          !auth.resetPasswordWithRecoveryCode("hina@example.com", code, newPassword, left)
              && auth.lastError() == RECOVERY_FAILED_MESSAGE);
    code = hinaCodes[0];
    newPassword = "Brand New Pass 42!";
    check(counter, "an unknown email gives exactly the same message",
          !auth.resetPasswordWithRecoveryCode("nobody@example.com", code, newPassword, left)
              && auth.lastError() == RECOVERY_FAILED_MESSAGE);
    code = omarCodes[0];
    newPassword = "Brand New Pass 42!";
    check(counter, "another account's recovery code does not work",
          !auth.resetPasswordWithRecoveryCode("hina@example.com", code, newPassword, left));
    runSql(accounts.connection, "UPDATE users SET failed_login_count = 0, locked_until = NULL");

    // The code typed in small letters with spaces instead of dashes still works.
    code = hinaCodes[1];
    for (int i = 0; i < static_cast<int>(code.length()); i++) {
        if (code[i] == '-') {
            code[i] = ' ';
        } else if (code[i] >= 'A' && code[i] <= 'Z') {
            code[i] = static_cast<char>(code[i] - 'A' + 'a');
        }
    }
    newPassword = "Brand New Pass 42!";
    bool reset = auth.resetPasswordWithRecoveryCode("hina@example.com", code, newPassword, left);
    check(counter, "a recovery code resets the forgotten password", reset && left == 9);
    check(counter, "the code and the new password were wiped from memory", code.empty() && newPassword.empty());
    check(counter, "the old password no longer works", !signIn(auth, "hina@example.com", GOOD_PASSWORD));
    check(counter, "the new password signs in", signIn(auth, "hina@example.com", "Brand New Pass 42!"));
    auth.signOut("signed_out");

    code = hinaCodes[1];
    newPassword = "Another New Pass 43!";
    check(counter, "the same code never works a second time",
          !auth.resetPasswordWithRecoveryCode("hina@example.com", code, newPassword, left)
              && auth.lastError() == RECOVERY_FAILED_MESSAGE);
    check(counter, "exactly one of Hina's codes is marked as used",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM recovery_codes WHERE used_at IS NOT NULL") == 1);
    check(counter, "the reset is in the audit history",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM audit_events "
                                             "WHERE event_type = 'password_reset_with_recovery_code'") == 1);
    runSql(accounts.connection, "UPDATE users SET failed_login_count = 0, locked_until = NULL");

    signIn(auth, "hina@example.com", "Brand New Pass 42!");
    std::string password = "Brand New Pass 42!";
    std::vector<std::string> newCodes;
    check(counter, "new recovery codes need the password and replace all old ones",
          auth.regenerateRecoveryCodes(password, newCodes) && newCodes.size() == 10);
    int codesLeft = 0;
    auth.recoveryCodesLeft(codesLeft);
    check(counter, "the account now has 10 unused codes", codesLeft == 10);
    auth.signOut("signed_out");
    code = hinaCodes[2];   // an old code that was never used
    newPassword = "Third New Pass 44!";
    check(counter, "an old, unused code stopped working",
          !auth.resetPasswordWithRecoveryCode("hina@example.com", code, newPassword, left));
    code = newCodes[0];
    newPassword = "Third New Pass 44!";
    check(counter, "a new code works", auth.resetPasswordWithRecoveryCode("hina@example.com", code, newPassword, left));
}

void testIsolation(TestCounter &counter, const QString &folder, const PasswordHasher &fast)
{
    TestAccounts accounts("auth_isolation", folder, fast, fast);
    AuthService &auth = accounts.auth;
    DatabaseManager &database = accounts.database;
    BudgetStore budgets(database);
    createAccount(auth, "Amna Javed", "amna@example.com");
    createAccount(auth, "Bilal Khan", "bilal@example.com");

    signIn(auth, "amna@example.com", GOOD_PASSWORD);
    Transaction salary = makeTransaction(TYPE_INCOME, 12000000, "Salary", "Amna's salary");
    Transaction rent = makeTransaction(TYPE_EXPENSE, 4500000, "Rent", "Amna's rent");
    database.addTransaction(salary);
    database.addTransaction(rent);
    Budget food;
    food.category = "Food";
    food.currencyCode = "PKR";
    food.limitMinor = 2000000;
    budgets.addBudget(food);
    database.saveSetting("theme", "dark");
    auth.signOut("signed_out");

    signIn(auth, "bilal@example.com", GOOD_PASSWORD);
    std::vector<Transaction> seen;
    std::vector<Budget> seenBudgets;
    database.loadTransactions(seen);
    budgets.loadBudgets(seenBudgets);
    check(counter, "Bilal sees none of Amna's transactions", seen.empty());
    check(counter, "Bilal sees none of Amna's budgets", seenBudgets.empty());
    Transaction found;
    check(counter, "Bilal cannot read Amna's transaction by its id ('not found')",
          !database.findTransactionById(rent.id, found)
              && database.lastError() == "No transaction with id " + QString::number(rent.id) + " was found.");
    Transaction changed = rent;
    changed.amountMinor = 1;
    check(counter, "Bilal cannot change Amna's transaction", !database.updateTransaction(changed));
    check(counter, "Bilal cannot delete Amna's transaction", !database.deleteTransaction(rent.id));
    Budget changedBudget = food;
    changedBudget.limitMinor = 1;
    check(counter, "Bilal cannot change or delete Amna's budget",
          !budgets.updateBudget(changedBudget) && !budgets.deleteBudget(food.id));
    Budget bilalFood = food;
    bilalFood.id = 0;
    check(counter, "Bilal may have his own Food budget in PKR too", budgets.addBudget(bilalFood));
    check(counter, "Bilal's settings are his own (Amna chose dark)", database.setting("theme", "light") == "light");
    Transaction coffee = makeTransaction(TYPE_EXPENSE, 45000, "Food", "Bilal's coffee");
    database.addTransaction(coffee);
    auth.signOut("signed_out");

    check(counter, "signed out, no transaction can be loaded",
          !database.loadTransactions(seen) && database.lastError() == "Please sign in first.");
    Transaction sneaky = makeTransaction(TYPE_EXPENSE, 100, "Food", "saved while signed out");
    check(counter, "signed out, nothing can be saved", !database.addTransaction(sneaky));
    check(counter, "signed out, budgets are refused too", !budgets.loadBudgets(seenBudgets));

    signIn(auth, "amna@example.com", GOOD_PASSWORD);
    database.loadTransactions(seen);
    budgets.loadBudgets(seenBudgets);
    check(counter, "Amna still has exactly her 2 transactions, unchanged",
          seen.size() == 2 && seen[0].amountMinor + seen[1].amountMinor == 16500000);
    check(counter, "Amna still has her budget with its limit", seenBudgets.size() == 1 && seenBudgets[0].limitMinor == 2000000);
    check(counter, "Amna's own setting is kept", database.setting("theme", "light") == "dark");
    check(counter, "in SQLite, every transaction belongs to an account",
          numberFromSql(accounts.connection, "SELECT COUNT(*) FROM transactions WHERE user_id IS NULL") == 0
              && numberFromSql(accounts.connection, "SELECT COUNT(*) FROM transactions") == 3);
}

void testClaimRecords(TestCounter &counter, const QString &folder, const PasswordHasher &fast)
{
    // Records made before accounts existed: no Session attached.
    QString path = folder + "/auth_claim.db";
    {
        DatabaseManager before("auth_claim_before");
        before.open(path);
        BudgetStore oldBudgets(before);
        Transaction lunch = makeTransaction(TYPE_EXPENSE, 80000, "Food", "Lunch before accounts");
        Transaction pay = makeTransaction(TYPE_INCOME, 9000000, "Salary", "Pay before accounts");
        before.addTransaction(lunch);
        before.addTransaction(pay);
        Budget oldFood;
        oldFood.category = "Food";
        oldFood.currencyCode = "PKR";
        oldFood.limitMinor = 1500000;
        Budget oldRent = oldFood;
        oldRent.category = "Rent";
        oldBudgets.addBudget(oldFood);
        oldBudgets.addBudget(oldRent);
    }

    DatabaseManager database("auth_claim");
    database.open(path);
    Session session;
    database.attachSession(session);
    AuthService auth(database, session, fast, fast);
    BudgetStore budgets(database);

    std::vector<std::string> codes;
    std::string password = GOOD_PASSWORD;
    auth.signUp("Zara Malik", "zara@example.com", password, codes);
    std::vector<Transaction> seen;
    database.loadTransactions(seen);
    check(counter, "a new account does NOT automatically get the old records", seen.empty());

    Budget zaraFood;
    zaraFood.category = "Food";
    zaraFood.currencyCode = "PKR";
    zaraFood.limitMinor = 3000000;
    budgets.addBudget(zaraFood);

    int oldTransactions = 0;
    int oldBudgets = 0;
    auth.countRecordsWithoutOwner(oldTransactions, oldBudgets);
    check(counter, "Finora can tell how many records have no account yet (2 and 2)",
          oldTransactions == 2 && oldBudgets == 2);

    int movedTransactions = 0;
    int movedBudgets = 0;
    int keptBudgets = 0;
    bool claimed = auth.claimRecordsWithoutOwner(movedTransactions, movedBudgets, keptBudgets);
    check(counter, "after the person agrees, the records move into the account",
          claimed && movedTransactions == 2 && movedBudgets == 1 && keptBudgets == 1);
    std::vector<Budget> zaraBudgets;
    database.loadTransactions(seen);
    budgets.loadBudgets(zaraBudgets);
    check(counter, "the moved transactions are complete and unchanged",
          seen.size() == 2 && seen[0].amountMinor + seen[1].amountMinor == 9080000);
    check(counter, "her own Food budget was kept, and the old Rent budget joined it",
          zaraBudgets.size() == 2 && zaraBudgets[0].category == "Food" && zaraBudgets[0].limitMinor == 3000000
              && zaraBudgets[1].category == "Rent");
    check(counter, "the move is in the audit history",
          numberFromSql("auth_claim", "SELECT COUNT(*) FROM audit_events WHERE event_type = 'records_claimed'") == 1);
    auth.countRecordsWithoutOwner(oldTransactions, oldBudgets);
    check(counter, "only the old Food budget is still without an account", oldTransactions == 0 && oldBudgets == 1);
}

void testFinoraSettings(TestCounter &counter, const QString &folder, const PasswordHasher &fast)
{
    // Finora's real settings: MODERATE for passwords, INTERACTIVE for codes.
    PasswordHasher moderate(HASH_MODERATE);
    TestAccounts accounts("auth_moderate", folder, moderate, fast);
    std::vector<std::string> codes;
    std::string password = GOOD_PASSWORD;
    accounts.auth.signUp("Kamran Ali", "kamran@example.com", password, codes);
    check(counter, "with Finora's settings the password hash uses 256 MiB and 3 passes",
          textFromSql(accounts.connection, "SELECT password_hash FROM users").startsWith("$argon2id$v=19$m=262144,t=3,p=1$"));
    check(counter, "and recovery codes use 64 MiB and 2 passes",
          textFromSql(accounts.connection, "SELECT code_hash FROM recovery_codes").startsWith("$argon2id$v=19$m=65536,t=2,p=1$"));

    // An account made with weaker settings is upgraded at its next sign-in.
    {
        Session olderSession;
        AuthService older(accounts.database, olderSession, fast, fast);
        std::string olderPassword = GOOD_PASSWORD;
        older.signUp("Old Settings", "old@example.com", olderPassword, codes);
        older.signOut("signed_out");
    }
    signIn(accounts.auth, "old@example.com", GOOD_PASSWORD);
    check(counter, "a hash with older settings is replaced by a MODERATE hash at sign-in",
          textFromSql(accounts.connection, "SELECT password_hash FROM users WHERE email = 'old@example.com'")
              .startsWith("$argon2id$v=19$m=262144,t=3,p=1$"));

    // Not enough memory: a clear message, and no account half-made.
    PasswordHasher impossible(3, static_cast<std::size_t>(512) * 1024 * MEBIBYTE);
    TestAccounts noMemory("auth_no_memory", folder, impossible, fast);
    password = GOOD_PASSWORD;
    check(counter, "when the memory is missing, sign-up stops with a clear message",
          !noMemory.auth.signUp("Low Memory", "low@example.com", password, codes)
              && noMemory.auth.lastError().startsWith("Finora could not get enough memory"));
    check(counter, "and no account was created", numberFromSql(noMemory.connection, "SELECT COUNT(*) FROM users") == 0
                                                     && !noMemory.session.isSignedIn());
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    TestCounter counter;
    std::cout << "Finora account and security tests\n";
    std::cout << "----------------------------------\n";

    bool started = PasswordHasher::initializeLibrary();
    check(counter, "libsodium starts (sodium_init)", started);
    if (!started) {
        return 1;
    }

    testModerateHashingMemory(counter);
    testPasswordHasher(counter);
    testRecoveryCodesAndWiping(counter);

    // Accounts on real SQLite files in a temporary folder. The tests use
    // the faster INTERACTIVE setting, except where Finora's real MODERATE
    // setting is the point of the test.
    QTemporaryDir temporaryFolder;
    if (!temporaryFolder.isValid()) {
        std::cout << "Could not create a temporary folder.\n";
        return 1;
    }
    QString folder = temporaryFolder.path();
    PasswordHasher fast(HASH_INTERACTIVE);
    std::cout << "---- sign-up ----\n";
    testSignUp(counter, folder, fast);
    std::cout << "---- sign-in, delays and sessions ----\n";
    testSignInAndDelays(counter, folder, fast);
    std::cout << "---- profile and password ----\n";
    testProfileAndPassword(counter, folder, fast);
    std::cout << "---- recovery codes ----\n";
    testRecovery(counter, folder, fast);
    std::cout << "---- one person's data per account ----\n";
    testIsolation(counter, folder, fast);
    testClaimRecords(counter, folder, fast);
    std::cout << "---- Finora's real settings ----\n";
    testFinoraSettings(counter, folder, fast);

    std::cout << "----------------------------------\n";
    std::cout << counter.checksRun << " checks, " << counter.checksFailed << " failed\n";
    if (counter.checksFailed > 0) {
        return 1;
    }
    return 0;
}
