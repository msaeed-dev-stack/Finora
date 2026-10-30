# Milestone 4 Progress

A running record of Milestone 4: what is done, what is in progress, the files each step changed, and the test results. It is updated after every implementation stage.

Last updated: 30 October 2026, Phase 4B complete and waiting for review.

## Overview

| Part | Status | Commit |
|---|---|---|
| Phase 4A: design system, dashboard, budgets, schema version 2 | Done and reviewed | `44fddfa`, local only (not pushed) |
| libsodium 1.0.22 dependency | Integrated and verified | Not committed yet |
| Phase 4B: accounts and security | Done, waiting for review | Not committed yet |
| Phase 4C: plans and checkout | Not started. Waits until Phase 4B is reviewed | |

## Part A: libsodium 1.0.22 integration

| Item | Result |
|---|---|
| Files | `third_party/libsodium-1.0.22/`: the 64-bit DLL, its import library and the 77 headers, copied unchanged from the signature-verified package; the ISC `LICENSE`; the original `.minisig`; `VERIFICATION.md` with sources, checksums and the signature record |
| Build | `CMakeLists.txt`: an imported target `finora_sodium` and `finora_copy_sodium_dll()`, which copies `libsodium-26.dll` next to `Finora.exe` and the test programs after each build. Only the new library `finora_auth` links libsodium |
| Git | `.gitignore` keeps ignoring `*.dll` and `*.a` everywhere except under `third_party/`; `.gitattributes` marks `*.dll`, `*.a` and `*.ttf` as binary |
| Notices | `THIRD_PARTY_NOTICES.md` lists libsodium with its ISC licence text |
| Debug and release builds | Both build with 0 compiler warnings. In `build/release`, `libsodium-26.dll` is byte-identical to the vendored file |

Rule: libsodium is not upgraded or replaced without the project owner's approval.

## Phase 4B: steps

| # | Step | Status |
|---|---|---|
| 1 | User database schema and safe migration | Done (stage 2): no new migration needed; existing records move into an account only through a confirmed, verified claim |
| 2 | Argon2id password hashing and verification | Done (stage 1) |
| 3 | Registration and sign-in | Done (stages 2 and 3) |
| 4 | Sign-out and session management | Done (stages 2 and 3) |
| 5 | User profiles | Done (stages 2 and 3) |
| 6 | Password recovery with single-use recovery codes | Done (stages 2 and 3) |
| 7 | Strict per-user financial data isolation | Done (stage 2) |
| 8 | Authentication and security tests | Done: 106 account checks and 33 interface checks |

**Phase 4B is complete and waits for review.** Phase 4C has not been started.

### Argon2id configuration

| Secret | Setting | Meaning | Time on the development PC (release build) |
|---|---|---|---|
| Account passwords | `HASH_MODERATE`: `crypto_pwhash_OPSLIMIT_MODERATE` (3 passes), `crypto_pwhash_MEMLIMIT_MODERATE` (256 MiB) | Every guess costs an attacker 256 MiB of memory and three passes over it | Hash about 650 ms, verify about 640 ms |
| Recovery codes | `HASH_INTERACTIVE`: 2 passes, 64 MiB | The codes are random with 60 bits each, so they need less extra work; signing in with a code checks up to 10 hashes | About 100 ms per code |

- **Stored string.** `crypto_pwhash_str` stores everything needed to check a password later, and nothing else: `$argon2id$v=19$m=262144,t=3,p=1$<16-byte random salt>$<32-byte hash>`. Every hash gets a new random salt.
- **Changing the settings later.** A stored hash made with older settings is recognised with `crypto_pwhash_str_needs_rehash`, and will be replaced at the next successful sign-in.
- **When memory runs out.** If the computer cannot provide the 256 MiB, `PasswordHasher::hash` returns `false` and gives no hash; no account is created half-way.
- **Memory measured.** The test `finora_auth_tests` reads the process's own memory counters:
  - peak memory went from 8 MiB to 265 MiB during one MODERATE hash;
  - it fell back to 9 MiB afterwards, so the memory is returned.

### Stage 1 (30 October 2026): account rules and Argon2id hashing

New files:

| File | Purpose |
|---|---|
| `src/core/PasswordPolicy.h/.cpp` | Minimum 10 and maximum 128 characters; a list of common passwords; no name or email in the password; the strength meter (very weak to very strong) and tips |
| `src/core/AccountRules.h/.cpp` | Email and name rules; the recovery code alphabet and format (`7KQ4-M2TX-9RHC`); the sign-in delay after wrong passwords; how long a session lasts |
| `src/core/Session.h/.cpp` | Who is signed in. Only `AuthService` (a friend class) can change it |
| `src/auth/PasswordHasher.h/.cpp` | Argon2id hash and verify, `needsRehash`, new recovery codes, and `wipe()` to overwrite a secret with zeros |
| `tests/finora_auth_tests.cpp` | The fourth test program: accounts and security |

Changed: `CMakeLists.txt` (new sources, `finora_auth`, `finora_auth_tests`), `tests/finora_tests.cpp` (42 new checks).

Test results:

| Program | Debug | Release |
|---|---|---|
| `finora_tests` (core) | 241 of 241 | Passed |
| `finora_auth_tests` | 22 of 22 | 22 of 22 |
| `finora_db_tests`, `finora_ui_tests` | Unchanged, passed | Passed |

All 4 CTest programs pass in release. The debug build of `Finora.exe` was not relinked, because the program was open on the development PC; the debug test programs and the complete release build were built instead.

### Stage 2 (30 October 2026): accounts, sessions, recovery and isolation

**Step 1, the schema.** Every column of schema version 2 (made in Phase 4A) was checked against Phase 4B's needs, and nothing is missing:

| Need | Where it is |
|---|---|
| Account, email unique without regard to case, name, hash | `users` |
| Wrong-password counter and waiting time | `users.failed_login_count`, `users.locked_until` |
| Last sign-in | `users.last_login_at` |
| Personal settings | `user_settings` |
| Recovery code hashes, single use | `recovery_codes.code_hash`, `recovery_codes.used_at` |
| Sessions with expiry and end reason | `login_sessions` |
| History | `audit_events` |
| Owner of every transaction and budget | `transactions.user_id`, `budgets.user_id` |

So no schema version 3 is needed, and the existing data file, already at version 2, is not rewritten again. Records made before accounts existed keep `user_id = NULL` until a signed-in person agrees to move them. The move, `UserStore::claimRecordsWithoutOwner`, runs in one database transaction:

1. It moves the transactions and checks that none is left behind.
2. It moves the budgets, but keeps out any budget for which the account already has the same category and currency.
3. It writes an audit event.

**Isolation.** `DatabaseManager::attachSession` connects the database to the program's one `Session`.
- From then on, every transaction and budget query has `user_id IS :owner`, filled from the Session, never from the interface.
- While nobody is signed in, every data function refuses with "Please sign in first."
- Settings are kept per person in `user_settings`.

New files:

| File | Purpose |
|---|---|
| `src/database/UserStore.h/.cpp` | All account SQL: accounts, sign-in counters, recovery codes, sessions, the audit history, the claim of records without an owner |
| `src/auth/AuthService.h/.cpp` | The account rules: sign-up, sign-in with general messages and waiting times, sign-out, idle and 12-hour session limits, profile, password change, recovery codes, claiming records |

Changed:
- `src/database/DatabaseManager.h/.cpp`: `attachSession`, `currentOwner`, `signedInUserId`, owner filters in the 5 transaction functions, per-person settings, and the user id in audit events.
- `src/database/BudgetStore.cpp`: owner filters in all 5 queries.
- `CMakeLists.txt`.
- `tests/finora_auth_tests.cpp`: 84 new checks.

Test results (debug):

| Program | Checks |
|---|---|
| `finora_tests` | 241 of 241 |
| `finora_db_tests` | 157 of 157 (unchanged: without a Session they use the records without an owner, as before) |
| `finora_auth_tests` | 106 of 106, in about 48 seconds |

What the 106 account checks prove, among others:

- **Sign-up and storage:**
  - the password and the recovery codes appear in no table, no column and not even in the bytes of the database file;
  - every hash has its own salt;
  - every password variable is wiped after use.
- **General messages:** a wrong password and an unknown email give the same message; after 3 failures both must wait.
- **Sessions:**
  - sessions end after 15 minutes of inactivity and after 12 hours;
  - sign-out records its reason;
  - a session left open by a crash is closed at the next start.
- **Profile:** a new email needs the current password; an email used by another account is refused.
- **Recovery codes:**
  - a code resets a forgotten password only once;
  - another account's code does not work;
  - new codes replace all old ones.
- **Isolation:** Bilal can neither see, change nor delete Amna's records, even with their ids; signed out, nothing can be loaded or saved.
- **Records made before accounts:**
  - they are not given to a new account automatically;
  - the claim moves them completely;
  - it keeps conflicting budgets apart.
- **Finora's real settings:**
  - passwords use MODERATE (256 MiB, 3 passes) and codes INTERACTIVE;
  - older hashes are upgraded at sign-in;
  - missing memory stops sign-up cleanly.

### Stage 3 (30 October 2026): the screens

How Finora starts now:

1. `main()` starts libsodium.
2. It opens the database and attaches the `Session`.
3. It creates `AuthService` with MODERATE hashing for passwords and INTERACTIVE for recovery codes.
4. It hands everything to `AppController`, which shows the sign-in window first and the main window only after a successful sign-in.

New files:

| File | Purpose |
|---|---|
| `src/ui/AuthWindow.h/.cpp` | The window before signing in. Pages: welcome, sign in, create account, recovery codes (shown once), reset the password with a code. A violet panel says these are **local demonstration accounts** |
| `src/ui/AppController.h/.cpp` | Shows the right window. After sign-up it asks before moving records made before accounts; the default button is "Not now". An event filter notices key presses and mouse actions. After 15 minutes without activity it closes any open dialog, signs out and explains why. Once a minute it marks the session active and enforces the 12-hour limit |
| `src/ui/AccountDialogs.h/.cpp` | Edit profile (a new email needs the password), change password, new recovery codes (needs the password; shows the codes once) |
| `src/ui/components/PasswordField.h/.cpp` | A password box with a show/hide button that the Tab key reaches. An optional live strength meter shows its rating in words ("Strength: Fair", a tip) and in colour. Copying out of the box is switched off |
| `src/ui/components/RecoveryCodesPanel.h/.cpp` | The 10 codes, Copy, and "I have saved these codes somewhere safe", which must be ticked before continuing |
| `resources/icons/eye.svg`, `eye_off.svg`, `log_out.svg` | Show password, hide password, sign out |

Changed:

| File | Change |
|---|---|
| `src/main.cpp` | Starts libsodium (and stops with a clear message if it cannot); `Session`, `attachSession`, `PasswordHasher` x2, `AuthService`, `AppController`; ends the session when Finora closes |
| `src/ui/MainWindow.h/.cpp` | An optional `AuthService *`; the `signOutRequested()` signal; shows the account in the sidebar and the first name in the greeting. Without an AuthService it behaves exactly as in Phase 4A |
| `src/ui/Sidebar.h/.cpp` | Avatar with initials, name, email and Sign out |
| `src/ui/SettingsPage.h/.cpp` | Profile, Security (password, recovery codes left, session limits, Sign out) and "Records from before accounts" cards. The licences dialog now also shows libsodium's ISC licence |
| `src/ui/DashboardPage.h/.cpp` | `setUserName()`: "Good evening, Muhammad" |
| `src/ui/UiHelpers.h/.cpp` | `countText()`: "1 transaction", "2 transactions" |
| `resources/styles/finora.qss` | Styles for the sign-in window, avatar and account labels |
| `CMakeLists.txt` | The new files, the icons, and libsodium's licence built into the program |
| `tests/finora_ui_tests.cpp` | 33 new checks |

How a password travels through the screens:

1. `PasswordField::takePassword()` copies the text into a `std::string` and empties the box at once.
2. `AuthService` overwrites the `std::string` with zeros (`sodium_memzero`) before it returns.
3. The confirmation copy is compared first and then wiped too.

A `QString` that Qt kept internally cannot be wiped reliably; that limit is documented.

Test results (debug and release, both with 0 compiler warnings):

| Program | Checks |
|---|---|
| `finora_tests` | 241 of 241 |
| `finora_db_tests` | 157 of 157 |
| `finora_ui_tests` | 164 of 164 (131 from before, all unchanged and passing) |
| `finora_auth_tests` | 106 of 106 |
| **Total** | **668** |

The 33 new interface checks click through the real `AppController`:
- welcome, then sign-up with the strength meter, the eye button and a password mismatch;
- the recovery codes, with Continue locked until "saved" is ticked;
- the "move older records?" question;
- the main window with the first name, the sidebar account and the welcome toast;
- Settings;
- signing out, where nothing can be loaded any more;
- a wrong password, which shows the general message;
- signing in again;
- a password reset with a recovery code, after which 9 codes are left;
- a second account that sees none of the first account's records;
- automatic sign-out after the idle time.

Packaged program check (release folder, independent of Qt and MinGW):

- Every DLL that `Finora.exe` and `libsodium-26.dll` load was followed through the folder. All of them are in `build/release` or are part of Windows: **0 missing**.
- With `PATH=C:\Windows\System32;C:\Windows` and no `QT_PLUGIN_PATH`, the deployed `finora_db_tests.exe` (157 of 157) and `finora_auth_tests.exe` (106 of 106) pass, loading Qt, the SQLite driver, the MinGW runtime and libsodium only from that folder.
- `Finora.exe` itself was not started for this check, because it would open the real database in `%LOCALAPPDATA%\Finora`.

### Known limitations of Phase 4B

- **Local only.** Accounts exist only on this computer. There is no server, no online account and no email; a forgotten password is reset with a recovery code. The difference from a production system is explained in the README.
- **Account emails are not secret on this computer.** Sign-up says when an email already has an account here. The sign-in and reset messages never do.
- **Passwords in Qt's memory.** Qt's own `QString` copies of a typed password cannot be wiped reliably. Finora keeps them as short as possible: the box is emptied at once, and every `std::string` copy is overwritten with zeros.
- **Short pause while hashing.** A MODERATE hash takes about 0.65 s. During sign-in the window shows a busy pointer and "Signing in...". A sign-up hashes 11 times (the password and 10 codes), about 1.5 s.
- **Narrow windows.** At the minimum window width (1024 px) the dashboard needs a horizontal scroll bar; at the normal size (1280 px) it fits.
