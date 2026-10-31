# Milestone 4 Progress

A running record of Milestone 4: what is done, what is in progress, the files each step changed, and the test results. It is updated after every implementation stage.

Last updated: 31 October 2026. Milestone 4 is complete: Phases 4A, 4B and 4C are reviewed and committed locally.

## Overview

| Part | Status | Commit |
|---|---|---|
| Phase 4A: design system, dashboard, budgets, schema version 2 | Done and reviewed | `44fddfa`, local only (not pushed) |
| libsodium 1.0.22 dependency | Integrated and verified | `1e855f5`, local only |
| Phase 4B: accounts and security | Done and reviewed | `6d9f395`, local only |
| Phase 4C: subscription plans and billing | Done and reviewed (stages 4C.1 to 4C.5) | The commit after `6d9f395` (see `git log`), local only |

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

**Phase 4B is complete, was reviewed, and is committed locally as `6d9f395`.**

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

## Phase 4C: subscription plans and billing

Four plans: Finora Free ($0), Plus ($5.99), Pro ($12.99) and Business ($24.99) a month. These are proposed prices for the project and have not been tested on the market. **All payments are simulated**: Finora collects no card details and moves no money. Team members, shared workspaces and roles in Business are planned features that need Finora Online (a server); they are not available in this desktop version.

| Stage | Contents | Status |
|---|---|---|
| 4C.1 | Subscription data model: schema version 3, the Free plan for every account | Done |
| 4C.2 | Plans and the entitlement engine; the Free limit of 3 active budgets | Done |
| 4C.3 | The Subscription page | Done |
| 4C.4 | Simulated checkout and the subscription lifecycle | Done |
| 4C.5 | CSV export, PDF summary and audit history; integration, security review and documents | Done |

### Stage 4C.1 (31 October 2026): the subscription data model

**Schema version 3** adds two tables. No existing table, row or password hash is changed.

| Table | What it holds |
|---|---|
| `subscriptions` | One row per account: plan, status, start, renewal, end and cancellation dates, a smaller plan waiting for the renewal date, whether the Pro trial was used, and the last payment's reference and status |
| `payments` | The billing history: every payment attempt with its plan, reason (new, upgrade or renewal), amount in US cents, status (pending, succeeded or failed), provider (`simulated` only), a unique reference and the paid period. **There is no column for card details** |

The database's own rules, a second line of defence behind the C++ code:
- **Plan and status agree:** Free and Expired go only with the Free plan; paid statuses only with paid plans.
- **Dates required:** an active plan needs a renewal date, and Trial, Active, Past due and Cancelled need the time their access ends.
- **One subscription per account**, enforced by `UNIQUE user_id`.
- **A payment and its subscription belong to the same account,** enforced by a two-column foreign key on `(subscription_id, user_id)`.
- **Payment rules:**
  - the amount is above zero;
  - never for the Free plan;
  - the provider can only be `simulated`;
  - every reference is unique;
  - a succeeded payment needs its paid period, and a failed one needs its reason.

**Upgrade from version 2 to 3.** It runs in the same single transaction as the earlier upgrade:
1. A backup copy, `finora-v2-backup-<time>.db`, is saved and checked. The check now also counts the accounts, not only the transactions.
2. The two tables and an index are created.
3. Every existing account gets Finora Free, counted from the day the account was created.
4. Finora checks one subscription per account and that every foreign key is valid. It writes an audit event, then sets version 3.

A file from Milestones 2 and 3 goes from version 1 to 2 to 3 in the same transaction. The upgrade question in `main.cpp` now names what the new version adds.

**New accounts.** `UserStore::createAccount` starts the Free plan in the same transaction as the account itself. If that step fails, nothing of the account is kept.

New files:

| File | Purpose |
|---|---|
| `src/core/Plans.h/.cpp` | `enum PlanId` (Free < Plus < Pro < Business), the four plans as an array of structs (code, name, price in US cents), plan codes and the price text |
| `src/core/Subscription.h/.cpp` | `enum SubscriptionStatus` (six states), `struct Subscription`, status codes and names |
| `src/database/SubscriptionStore.h/.cpp` | The SQL that reads the signed-in person's subscription and billing history. The owner always comes from the Session |
| `tests/finora_billing_tests.cpp` | The fifth test program: subscriptions and billing |

Changed:
- `src/database/DatabaseManager.h/.cpp`: version 3, `upgradeToVersion3()`, and the backup check that also counts accounts.
- `src/database/UserStore.h/.cpp`: the Free plan at sign-up.
- `src/main.cpp`: the upgrade question names what the new version adds.
- `CMakeLists.txt`: the new files and `finora_billing_tests`.
- `tests/finora_db_tests.cpp`: 8 checks now name the newest version and its tables (version 2 → 3, 9 → 11 tables). Each still checks the same thing.

Test results (debug and release, 0 compiler warnings):

| Program | Checks |
|---|---|
| `finora_tests` | 241 of 241 |
| `finora_db_tests` | 157 of 157 |
| `finora_ui_tests` | 164 of 164 |
| `finora_auth_tests` | 106 of 106 |
| `finora_billing_tests` | 54 of 54 (new) |
| **Total** | **722** |

The release `Finora.exe` was not relinked, because it was open on the development PC. Every other release target was built and tested.

What the 54 new checks prove, among others:

- **Upgrade from version 2:**
  - The test builds a real version 2 file: 2 accounts made by Finora's own classes, with sessions, recovery codes, transactions, budgets and a record from before accounts.
  - After the upgrade, every row of the 9 older tables is exactly as before, including every Argon2id password hash and recovery-code hash.
  - Each account has one Free subscription.
  - A checked backup exists.
  - Amna can still sign in with her old password and sees her own transactions.
- **Failed upgrade:** when it fails half-way, the file stays at version 2, byte for byte. The backup is kept, and no new table is left behind.
- **Sign-up is all or nothing:** if the Free plan cannot be started, no user, code, setting or event remains.
- **Isolation:** signed in as Bilal, only Bilal's subscription can be read. Signed out, or without a Session, nothing can be read.
- **The database's own rules** refuse:
  - an unknown plan;
  - Active on Free;
  - Active without a renewal date;
  - a trial without an end;
  - a second subscription;
  - a payment of zero, for Free, through an unknown provider, without its period, with a repeated reference, or pointing to another person's subscription.

### Stage 4C.2 (31 October 2026): plans and the entitlement engine

**One place for the rules.** `src/core/Plans.h` holds the four plans and a table of all 28 features. For each feature it gives the smallest plan that includes it, and whether it is:
- built (`FEATURE_AVAILABLE`);
- planned (`FEATURE_PLANNED`, shown as "Soon");
- for Finora Online (`FEATURE_NEEDS_ONLINE`): team members, shared workspaces, role-based access and team reports.

The plans are an ordered enum, so "Pro includes everything in Plus" is one comparison: `plan >= FEATURES[feature].lowestPlan`.

**The lifecycle as a state machine.** `src/core/Subscription.h` holds plain functions that take the time as text:
- `subscriptionAt(stored, now)`:
  - Active becomes Past due at the renewal date. A smaller plan chosen earlier starts then.
  - Trial, Past due and Cancelled become Expired (Free) when their access ends.
- `planAt(stored, now)`: the plan whose features may be used. It applies the deadlines itself, so a period that has ended no longer counts, even while the database row still says Active.
- `planChangeFor(current, wanted)`: what choosing a plan means. It is one of subscribe, upgrade, renew, downgrade at the renewal date, cancel, resume or nothing. The screens and the checkout of stage 4C.4 use it.

**The single gate.**
- `DatabaseManager::currentPlan()` reads the signed-in person's own subscription row and applies `planAt()`. It fails closed: no Session, nobody signed in, or a missing or unreadable row all mean Free.
- `requireFeature(feature)` is the check every protected operation calls. It refuses features of larger plans and features that are not built yet, with a message that names the plan needed.

**Free: at most 3 active budgets.**
- `BudgetStore::addBudget` refuses a fourth budget on Free, and `planLimitReached()` tells the screen why. The check sits below every screen.
- `BudgetStore::loadBudgets` marks the budgets over the limit as paused, through `markPausedBudgets` in `Budget.h`:
  - the 3 oldest stay active;
  - the others are kept and shown, but not tracked;
  - nothing is ever deleted, and after an upgrade every budget is active again.
- The same rule covers every other way the limit could be exceeded: a downgrade, a trial or paid period that ends, and moving older records into an account.
- While nobody is signed in, no budget can be added at all.
- Without a Session there are no accounts and so no plans, and no limit applies. Only the tests of Milestones 2 to 4A run like this; Finora always attaches the Session in `main()`.

**A problem found and fixed during this stage.**
- The first version also applied the Free limit when no Session was attached.
- A Phase 4A interface test creates 4 budgets without accounts. The fourth was refused, and the error box waited for an answer until the test's 120-second timeout ended it.
- Plans belong to accounts, so the limit now applies only when a Session is attached (`DatabaseManager::budgetLimit()`). Phase 4A behaves exactly as before.

Changed and new:
- `src/core/Plans.h/.cpp`: features, limits, entitlement functions and messages.
- `src/core/Subscription.h/.cpp`: the lifecycle rules.
- `src/core/Budget.h/.cpp`: `paused`, `markPausedBudgets` and `countPausedBudgets`.
- `src/database/DatabaseManager.h/.cpp`: `currentPlan`, `requireFeature` and `budgetLimit`.
- `src/database/BudgetStore.h/.cpp`: the limit and the paused budgets.
- `tests/finora_tests.cpp`: 78 new checks.
- `tests/finora_billing_tests.cpp`: 27 new checks.

Test results (debug and release, 0 compiler warnings):

| Program | Checks |
|---|---|
| `finora_tests` | 319 of 319 (78 new) |
| `finora_db_tests` | 157 of 157 |
| `finora_ui_tests` | 164 of 164 |
| `finora_auth_tests` | 106 of 106 |
| `finora_billing_tests` | 81 of 81 (27 new) |
| **Total** | **827** |

What the new checks prove, among others:
- **The plan table:**
  - every plan includes everything of the smaller plans, and each brings 7 features of its own;
  - the prices are 0, 599, 1299 and 2499 cents;
  - every Free feature is built, and the team features are marked for Finora Online.
- **The time rules, at their exact boundaries:** one second before and exactly at the renewal date, the end of the grace period, the end of the trial and the end of a cancelled period.
- **Every plan change and the pause rule.** The 3 oldest budgets by id stay active, whatever order the screen shows them in.
- **The gate:**
  - an active Plus row gives Plus, and Plus stays during the grace period;
  - a row whose period has ended gives Free, even though it still says Active;
  - a missing row gives Free;
  - Bilal stays on Free while Amna has Business.
- **The budget limit:**
  - `BudgetStore` refuses the fourth budget with no screen involved, and a duplicate still gets its own message;
  - back on Free, all 5 budgets remain and 2 are paused;
  - deleting an active budget makes the next oldest active;
  - moving older budgets into a Free account pauses the extra ones;
  - while nobody is signed in, nothing can be added.

### Stage 4C.3 (31 October 2026): the Subscription page

A new **Subscription** item in the sidebar, before Settings, so there are now 8 pages. From top to bottom, the page shows:

1. **A banner:** "Simulated billing: Finora collects no card details and never charges real money."
2. **Your plan:** the plan and its status chip, the price, and what the status means in words. For example: "Renews on 30 Nov 2026", "Your free Pro trial ends on 14 Nov 2026 (13 days left)", "Cancelled on ... Pro stays until ..., then Finora Free applies. Nothing is deleted." The last simulated payment and its reference also appear here.
3. **Active budgets:** "2 of 3" with a bar, how many budgets are paused, and a "Manage budgets" link.
4. **The four plan cards:** four side by side at the normal 1280-pixel window, two by two at the 1024-pixel minimum. Each card has:
   - the price per month and a one-line summary;
   - "Everything in Finora Plus, plus:" followed by the plan's own features;
   - a green tick **only** for features that are built. Planned features get a hollow circle and "Soon"; team features get "Online".
   - The person's own plan has a violet edge and a "Current plan" badge, or "Your trial" during the trial.
   - Business adds a note that team members, shared workspaces, roles and team reports need Finora Online and do not work in this desktop version.
5. **"Compare every feature":** a table with 27 rows (the two budget features share one row) and the columns Free, Plus, Pro and Business. Each cell is ✓, Soon, Online, — or the number of budgets. It is hidden until "Show the comparison" is clicked.
6. **The billing history:** date, plan, reason, amount, status and reference. With no payments yet, the card says so.
7. **A footnote:** the prices are proposed and have not been tested on the market.

**Other screens:**
- **Sidebar:** the plan appears under the person's name ("Finora Free", "Pro trial", "Plus · payment due") and opens the Subscription page. It is read again whenever a page is shown, so it always matches the database.
- **Budgets page:**
  - A line under the header, such as "Finora Free: 3 of 3 active budgets, 2 paused.", with a "Compare plans" link.
  - At the limit, "+ New Budget" explains the plans instead of opening the form. The buttons are "See plans" and "Not now", and "Not now" is the default.
  - If a budget is saved some other way, `BudgetStore` still refuses it, and the same explanation appears.
  - A paused budget's card says "Paused on Finora Free" and "Not tracked while paused", and can still be edited or deleted. The currency totals count only active budgets.
- **Dashboard:** paused budgets give no budget alerts.
- **Both themes:** every new colour comes from the existing theme tokens. Screenshots were checked in the light theme, the dark theme and at 1024 pixels.

In this stage the page only shows information. The buttons that change a plan arrive with the checkout in stage 4C.4, so no button on the page is ever decorative.

New: `src/ui/SubscriptionPage.h/.cpp` and `resources/icons/subscription.svg`.

Changed:
- `src/ui/Sidebar.h/.cpp`: the page numbers, the new item and the plan chip.
- `src/ui/MainWindow.h/.cpp`: the page, the "see plans" connections and `showPlanIndicator`.
- `src/ui/BudgetsPage.h/.cpp`, `src/ui/DashboardPage.cpp`.
- `src/ui/UiHelpers.h/.cpp`: dates, days left, the plan chip text, and `askToSeePlans`.
- `src/database/DatabaseManager.h/.cpp`: `accountsInUse`.
- `resources/styles/finora.qss`, `CMakeLists.txt`.
- `tests/finora_ui_tests.cpp`: 28 new checks. The two descriptions "there are 7 pages" and "there are 7 sidebar buttons" now say 8; their checks compare with `PAGE_COUNT` and did not change. The navigation loop clicks every page, so the new page adds 2 checks by itself.

Test results (debug and release, 0 compiler warnings):

| Program | Checks |
|---|---|
| `finora_tests` | 319 of 319 |
| `finora_db_tests` | 157 of 157 |
| `finora_ui_tests` | 194 of 194 (30 new) |
| `finora_auth_tests` | 106 of 106 |
| `finora_billing_tests` | 81 of 81 |
| **Total** | **857** |

### Stage 4C.4 (31 October 2026): the simulated checkout and the subscription lifecycle

**How the parts work together:**

| Part | Layer | Job |
|---|---|---|
| `PaymentGateway` | billing | An **abstract class**: four pure virtual functions (`providerName`, `isSimulated`, `newReference`, `charge`). `PaymentRequest` has **no field for card details** |
| `SimulatedPaymentGateway` | billing | Approves or declines at once and moves no money. No network, no secret keys. References look like `SIM-7KQ4M2TX9RHC` |
| `SubscriptionService` | billing | The lifecycle in action. It is the **only friend class of `SubscriptionStore`**, so no screen can change a plan or a payment by itself. The person comes from the Session (through `DatabaseManager`), and the price from the plan table |
| `SubscriptionStore` (changing functions) | database | Each change checks the current state in its `WHERE` clause and writes its audit event in the same transaction. If the state does not fit, no row changes and nothing is saved |
| `CheckoutDialog` | ui | Shows the plan, the price, the period and "Simulated payment: no card details, no money moves". Its buttons are "Confirm simulated payment of $5.99", the link "Simulate a declined payment", and Cancel |

**A payment, step by step** (`SubscriptionService::payForPlan`):
1. Bring the subscription up to date, then check with `planChangeFor()` that this choice needs a payment.
2. Take the price from `PLANS[plan]`, never from the screen.
3. Record the attempt as `pending`, before the provider is asked.
4. Ask the gateway, through a virtual call.
5. **If it was declined:** the payment becomes `failed` with its reason, and the plan does not change.
6. **If it succeeded:** in ONE transaction, the payment becomes `succeeded` and the plan is switched on. The `UPDATE` of `subscriptions` changes the row only `WHERE EXISTS` a succeeded payment of **this same person for this same plan**.
7. If the plan cannot be switched on, the attempt is marked as failed, so it is never left pending.

**The lifecycle in the program:**

| Action | Effect | Payment |
|---|---|---|
| Choose a plan from Free, from Expired or during the trial | A new monthly period from today | Full price |
| Upgrade | The larger plan from today, with a new period. The unused part is not refunded, and the checkout says so | Full price of the larger plan |
| Pay to renew (past due) | The plan continues with a new period | Full price |
| Switch at renewal | A smaller plan starts at the renewal date. After a cancellation, the cancellation is withdrawn first | None now |
| Cancel | Active: the plan stays until the end of the paid period. Trial or past due: Free applies at once | None |
| Resume, or keep the plan | Withdraws a cancellation or a smaller plan | None |
| Try Pro free for 14 days | Once per account, from Free or Expired | None |
| Time passes | Saved at sign-in and on every visit to the Subscription page, each with an audit event | None |
| A crash during a payment | At the next start, the pending payment is marked as failed ("Finora closed before this payment finished") | None |

**`--billing-demo`.**
- `Finora.exe --billing-demo` shows the button "Demo: end this period now" on the Subscription page. It moves only this account's deadlines, so a renewal, a missed payment or an expiry can be shown live.
- `SubscriptionService` itself refuses this outside the demo mode, so no screen can use it in normal running.

**The page's buttons.**
- Each plan card says what choosing it would do: Choose Plus, Upgrade to Pro, Pay to renew, Switch at renewal, Switch to Free, Keep Pro, or Current plan (which cannot be clicked).
- "Your plan" shows the actions that fit the status.
- Every change that is not a payment asks first, and the safe answer is the default button. For example: "Cancel Finora Plus? You keep Finora Plus until 1 Dec 2026 ... Nothing is deleted."
- After each change, the page, the sidebar chip and the billing history update, and a toast confirms it.

**Wiring:**
- `main.cpp` creates `SimulatedPaymentGateway` and `SubscriptionService`.
- `AppController::setSubscriptionService()` passes the service on. At each sign-in it saves what time has done to the plan.
- `MainWindow` gives the service to the Subscription page.
- The interface tests of Phase 4B create `AppController` without a service, exactly as before.

New:
- `src/billing/PaymentGateway.h`
- `src/billing/SimulatedPaymentGateway.h/.cpp`
- `src/billing/SubscriptionService.h/.cpp`
- `src/ui/CheckoutDialog.h/.cpp`
- the library `finora_billing`

Changed:
- `src/database/SubscriptionStore.h/.cpp`: the changing functions and the friend class.
- `src/ui/SubscriptionPage.h/.cpp`: the actions.
- `src/ui/MainWindow.h/.cpp`, `src/ui/AppController.h/.cpp`, `src/main.cpp`, `CMakeLists.txt`.

**Two problems found and fixed:**
- **The checkout was too narrow.** At 480 pixels the button labels were cut off, and the declined-payment message squeezed the order lines. The dialog is now 540 pixels wide, the decline link has its own row, and the dialog grows when the message appears. This was checked in screenshots.
- **One CTest run reported a 4,485-second timeout** for the interface tests, although their limit is 120 seconds and an earlier real hang was cut off at 120.03 seconds. The same command ran them again at once: 210 of 210. Three more direct runs of the release program, with a hard 180-second limit, each finished in about 20 seconds with 210 of 210. The likely cause is the computer sleeping during that run, because the timer counts wall-clock time. If it appears again, it will be investigated.

Test results (debug and release, 0 compiler warnings):

| Program | Checks |
|---|---|
| `finora_tests` | 319 of 319 |
| `finora_db_tests` | 157 of 157 |
| `finora_ui_tests` | 210 of 210 (16 new) |
| `finora_auth_tests` | 106 of 106 |
| `finora_billing_tests` | 138 of 138 (57 new) |
| **Total** | **930** |

What the new checks prove, among others:
- **The gateway:** used through the abstract class, references are unique and in the right format, a decline says that no money moved, and an incomplete request is refused.
- **Paying:**
  - a declined payment changes no plan, but is recorded with its reason;
  - a confirmed one switches the plan on, records 599 cents in USD from the provider `simulated` with the reason "new", sets a renewal date one month later and an access end 7 days after that, and writes an audit event;
  - an upgrade costs the full Pro price;
  - Bilal sees none of Amna's payments;
  - while nobody is signed in, nothing can be paid.
- **Changes:** Pro to Plus waits for the renewal date and charges nothing; "Keep Pro" withdraws it; a cancellation keeps Pro until the paid period ends; resuming keeps the old renewal date; all 7 steps are in the audit history.
- **Time** (the demonstration mode):
  - Active becomes Past due, and the plan still works during the grace period;
  - paying the renewal is recorded as "renewal";
  - an unpaid grace period ends on Free, with all 5 budgets kept and the 2 newest paused;
  - outside the demonstration mode, the deadlines cannot be moved.
- **The trial:** 14 days, once per account, no payment; it ends on Free; paying during the trial keeps Pro; it can also be ended at once.
- **A crash:** a pending payment is marked as failed at the next start and switches on no plan.
- **The guard:** a test gateway says "succeeded" but first tampers with the payment, in three ways: it moves the payment to Bilal, changes its plan, or marks it failed. Each time the plan stays Free, and no payment ends up "succeeded".
- **The screens:**
  - the checkout has no text field;
  - a decline keeps the checkout open with the reason, and the confirmation then switches on Plus;
  - the history shows both attempts, and the sidebar and a toast show the reference;
  - cancelling asks first, and "Keep Finora Plus" changes nothing;
  - the demonstration button ends the period, after which a paused budget card says "Paused on Finora Free";
  - the Pro trial starts from the page.

### Stage 4C.5 (31 October 2026): exports, audit history, integration and final verification

**Three paid features are now real.** Each checks the plan inside its own operation, before it reads any record:

| Feature | Plan | Where | How |
|---|---|---|---|
| CSV export of every transaction | Plus | "Export CSV" on the Transactions page; `ReportExporter::exportTransactionsCsv` | `transactionsCsv()` in `src/core/CsvExport.cpp` builds the text: quotes and doubled quotes, plain amounts, and a `'` in front of `=`, `+`, `-` and `@` (protection against CSV injection). `QSaveFile` writes it atomically with the UTF-8 mark |
| A monthly PDF summary | Pro | "PDF summary" on the dashboard, for the month on screen; `ReportExporter::exportMonthlyPdf` | Income, expenses and net per currency, spending by category, and the tracked budgets. Every text from the records is escaped. Drawn by `QTextDocument` onto Qt's own `QPdfWriter` (A4), so no new library is needed |
| The account's audit history | Business | Settings > Security > "View"; `DatabaseManager::loadAuditHistory`; `AuditHistoryDialog` | The signed-in person's own events, newest first: sign-ins, plan changes, payments, exports |

Below the needed plan, each button shows a small lock. Clicking it explains the plans with "See plans" and "Not now".

**Documents updated:**
- `README.md`: status, features, the plans and the `--billing-demo` mode, limitations.
- `docs/DESIGN.md`: section 1 brought up to date (5 layers, 5 test programs, all SQL in `src/database`), and a new section 12 on Phase 4C, including the production path and the note on payment providers.
- `docs/VIVA_PREPARATION.md`: section N with 15 questions, and the quick revision.
- `docs/DEMO_MILESTONE4.md`: a Phase 4C script, C1 to C12.
- `docs/REQUIREMENTS_CHECKLIST.md`, `docs/CPP_CONCEPTS_MAP.md`: 15 new concepts.
- `docs/DEMO_MILESTONE3.md`: now expects 5 test programs.

**Payment providers usable from Pakistan (decision D7).** Checked on 31 October 2026 on the providers' own pages; the details and links are in `docs/DESIGN.md`, section 12.10:
- **Stripe:** Pakistan is not on its supported list.
- **Paddle:** Pakistan is not on its unsupported list, and it acts as merchant of record.
- **Safepay:** offers subscriptions and local methods.
- **Easypaisa:** has a merchant gateway.
- **JazzCash and PayFast:** their official pages could not be read automatically, so the note says to confirm them directly.
- **Raast:** has supported person-to-merchant payments since December 2023.

**Security review** (each point was checked by searching the code, not from memory):

| Check | Result |
|---|---|
| Network code in `src/billing`, `src/reports` and the subscription store | None |
| Card fields, provider keys or secrets in the code | None. A search found only ordinary password variables from Phase 4B |
| Owner of every subscription and payment operation | All 13 reading and changing functions of `SubscriptionStore` take the owner from the Session (`signedInOwner`). The only exception is the start-up cleanup of interrupted payments, which is global by design |
| A paid plan without a matching payment | Impossible: the `EXISTS` guard in `activatePaidPlan`, proven by 3 tampering tests |
| Plan checks below the screens | Inside `BudgetStore`, `ReportExporter` (CSV and PDF) and `loadAuditHistory`; tested without any screen |
| SQL outside `src/database` | None: no interface, billing or report file uses SQL classes |
| The demonstration mode | Refused by `SubscriptionService` itself unless Finora was started with `--billing-demo` |
| Text from records in files | CSV: injection protected. PDF: HTML-escaped. Both tested |

**Packaged program check (release folder):**
- Every DLL that `Finora.exe`, `libsodium-26.dll` and the billing tests load was followed: **31 DLLs, 0 missing**. The platform plugin `qwindows.dll` and the SQLite driver are deployed.
- With only `C:\Windows\System32;C:\Windows` on the PATH and no Qt settings, the deployed database tests (157), account tests (106) and billing tests (156, including the PDF) all pass.
- `Finora.exe` itself was not started, because it would open the real database.

New:
- `src/core/CsvExport.h/.cpp`
- `src/reports/ReportExporter.h/.cpp` and the library `finora_reports`
- `src/ui/AuditHistoryDialog.h/.cpp`
- `resources/icons/lock.svg`

Changed:
- `src/core/Plans.h`: CSV, PDF and the audit history marked as built.
- `src/database/DatabaseManager.h/.cpp`: `loadAuditHistory`.
- `src/ui/TransactionsPage.h/.cpp`, `src/ui/DashboardPage.h/.cpp`, `src/ui/SettingsPage.h/.cpp`, `src/ui/MainWindow.cpp`.
- `src/ui/UiHelpers.h/.cpp`: `showLockIfNeeded`.
- `CMakeLists.txt`: the billing tests are now a `QGuiApplication`, run with the offscreen platform.
- The tests and the documents listed above.

**Test results at the end of stage 4C.5** (before the final review): 972 checks in each build.

### The final review before the commit (31 October 2026)

The project owner's checklist was gone through point by point. The evidence comes from the code, from searches and from tests.

| Check | Result |
|---|---|
| CSV formula injection | **One gap was found and fixed.** `=`, `+`, `-`, `@`, a leading tab and a carriage return were covered, but a leading line feed, and a formula character after leading spaces (`  =SUM(A1)`), were not. `csvField` now looks at the first character that is not a space, tab or line break. 5 new core checks cover these cases, and a dash or `@` inside the text stays unchanged |
| No partial files | `QSaveFile` writes into a temporary file. New tests: replacing a read-only file fails and leaves the old file byte-identical, with no temporary file left behind. An export into a missing folder fails and creates nothing |
| Currencies in the PDF | Every amount stays in its own currency: one row per currency and one spending table per currency, with no grand total. A new test adds a USD expense: "USD 45.99" gets its own row, the PKR total is unchanged, and no mixed sum appears |
| Subscriptions and payments tied to the signed-in person | All 13 reading and changing functions of `SubscriptionStore` take the owner from the Session. The only exception is the start-up cleanup of interrupted payments |
| Failed, pending and interrupted payments | Only two statements can make a subscription Active. `activatePaidPlan` requires a succeeded payment of the same person and plan. `resume` works only for an already-paid cancelled plan whose period has not ended, and never changes the plan. The free 14-day Pro trial is a separate status, once per account |
| Entitlements below the screens | `BudgetStore`, `ReportExporter` (CSV and PDF) and `loadAuditHistory` check by themselves. The demonstration mode is checked in `SubscriptionService` |
| Audit history | Only `user_id = owner`. A new test shows that Bilal's lines are in the same table but none is shown to Amna |
| Downgrades and expiry | No `DELETE`, `DROP` or `TRUNCATE` anywhere in the plan, billing or report code. Budgets are only marked as paused in memory |
| Upgrade from layout 2 to 3 | Tested on a real version 2 file. The project owner's own data file was upgraded at 15:13 through Finora's question. A read-only comparison shows the original account (email, name, Argon2id hash, creation time), its 10 recovery-code hashes and its settings are identical to the verified backup `finora-v2-backup-2026-10-31_15-13-02.db`. Both files pass `PRAGMA integrity_check` |
| Payment-provider note | `docs/DESIGN.md` 12.10 now has separate columns for what an official source confirms and what still needs the provider's own confirmation |

**Final verification:**
- **Clean rebuilds** (`--clean-first`) of debug and release compiled all 93 source files with **0 warnings and 0 errors**. The release build includes `Finora.exe` and its deployment.
- **Packaged check:** 31 DLLs followed, 0 missing. With only Windows on the PATH, the deployed database (157), account (106) and billing (161) tests pass.
- **The final release `Finora.exe`** was started on the development PC, showed its window, and was closed normally. The SHA-256 of the real data file was the same before and after, and no lock file was left.

**Final test results** (debug and release, 0 compiler warnings; `ctest --preset release`: 100% tests passed, 0 tests failed out of 5):

| Program | Checks |
|---|---|
| `finora_tests` | 338 of 338 |
| `finora_db_tests` | 157 of 157 |
| `finora_ui_tests` | 220 of 220 |
| `finora_auth_tests` | 106 of 106 |
| `finora_billing_tests` | 161 of 161 |
| **Total** | **982** in each build |

Phase 4C added 314 checks: core 97, interface 56, billing 161.

### Phase 4C in numbers

- 24 new files and 38 changed files.
- Schema version 3.
- 2 new libraries (`finora_billing`, `finora_reports`) and 1 new test program.
- Decisions D1 to D8 applied as approved.

### Known limitations of Phase 4C

- **Payments are simulated.** Each renewal is confirmed by the person, because no card is stored.
- **Plans are enforced on this computer.** Whoever owns it can edit `finora.db` or set the clock back. Real enforcement needs the server described in `docs/DESIGN.md`, section 12.9.
- **An upgrade starts a new month at the full price** without a refund for the unused part. The checkout says so.
- **Many plan features are still planned** ("Soon"), and Business team features need Finora Online ("Online").
- **Exported files are not encrypted.**

**Phase 4C is complete, reviewed, and committed locally** as "Milestone 4 Phase 4C: subscription plans, entitlements and simulated billing" (not pushed).
