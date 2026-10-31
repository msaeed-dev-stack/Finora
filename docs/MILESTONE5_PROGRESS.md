# Milestone 5 Progress Record

Savings goals and exchange rates. Requirements checklist, section 2:

- "Savings goals with safe contributions and progress" (Milestone 5)
- "PKR, USD and EUR with manual exchange rates and documented rounding" (Milestone 5)

Date: 31 October 2026. Version number: 0.5.0 (was 0.4.0).

## 1. Decisions

| # | Decision | Why | How to change it |
|---|---|---|---|
| 1 | Savings goals are a **Finora Plus** feature | The plan table approved in Phase 4C lists "Savings goals" under Plus | Making goals free is one line in `src/core/Plans.h` (`PLAN_PLUS` to `PLAN_FREE`) |
| 2 | Exchange rates work on **every plan** | "PKR, USD and EUR" is a Free feature | None needed |
| 3 | A rate is measured in PKR, with 4 decimal places, stored as a whole number | Exact, no `double`; PKR is the home currency | `RATE_SCALE` in `ExchangeRates.h` |
| 4 | Rounding: to the nearest paisa or cent, an exact half **away from zero**, once per conversion | A common, easy-to-explain rule; one rounding avoids errors adding up | `convertAmount` |
| 5 | Rates belong to each account (user settings) and are saved with the date | One person's rates must not change another person's estimates | `saveExchangeRates` |
| 6 | Contributions are never edited or deleted one by one; a mistake is corrected with a new entry | The history always explains the saved amount | — |
| 7 | A goal's currency is fixed once money is in it | The saved amount is in that currency | `GoalStore::updateGoal` |
| 8 | Money in a goal is **not** a transaction, and the screens say so | It is money set aside, not spent; a transaction would count it twice | — |
| 9 | On Free (for example after a downgrade), goals are shown and can be deleted, but not changed | "Never delete financial records when a user downgrades" | `GoalStore::checkPlan` |
| 10 | The money form allows dates up to today | Money cannot be set aside on a future day. The core rule accepts any real date | `ContributionDialog` |
| 11 | No upgrade without the person's agreement, also after a crash in the middle of an upgrade | Approved after the verification found the gap (section 6) | `DatabaseManager::setUpgradeAllowed`, `openDatabaseAtStartup` |

## 2. What was built

| Area | Files |
|---|---|
| Core rules | `src/core/Goal.h/.cpp` (`validateGoal`, `validateContribution`, `monthsUntil`, `goalProgressFor`), `src/core/ExchangeRates.h/.cpp` (`parseRate`, `formatRate`, `rateInPkr`, `convertAmount`, `addSignedChecked`) |
| Entitlements | `src/core/Plans.h`: "Savings goals" changed from planned to built |
| Database | `DatabaseManager`: `CURRENT_SCHEMA_VERSION = 4`, `upgradeToVersion4`, `exchangeRates`, `saveExchangeRates`, `exchangeRatesSavedOn`, `setUpgradeAllowed`, `upgradeNeeded`. New `src/database/GoalStore.h/.cpp` |
| Screens | New `src/ui/GoalsPage.h/.cpp`, `src/ui/GoalDialogs.h/.cpp` (`GoalDialog`, `ContributionDialog`, `GoalHistoryDialog`), `src/ui/CurrencyPage.h/.cpp`. `Sidebar.cpp`: Goals and Currency lose the "Soon" badge. `MainWindow.cpp`: the two pages. `finora.qss`: four styles |
| Start-up | New `src/ui/DatabaseStartup.h/.cpp` (`openDatabaseAtStartup`, with `askToUpgrade` and `upgradeChangesText` moved there from `main.cpp`); `main.cpp` calls it |
| Tests | `finora_tests` (goals, rates in every direction), `finora_billing_tests` (upgrade 3 to 4, rollback, the store, balances unchanged, privacy, downgrade, trigger, rates), `finora_db_tests` (version 4, 13 tables, no upgrade without agreement, an interrupted upgrade), `finora_ui_tests` (both pages, the dashboard balance, the Amount box, the start-up question after an interrupted upgrade). New helper `tests/InterruptedUpgrade.h` |
| Documents | `README.md`, `docs/DESIGN.md` (sections 1, 2, 7.4 and 13), `docs/VIVA_PREPARATION.md` (section O), `docs/REQUIREMENTS_CHECKLIST.md`, `docs/CPP_CONCEPTS_MAP.md`, `docs/DEMO_MILESTONE5.md`, this file |

## 3. Schema version 4

- `goals` and `goal_contributions`, the indexes `goals_one_name_per_owner` and `goal_contributions_by_goal`, and the trigger `goal_saved_amount_stays_valid`. Details: `docs/DESIGN.md`, section 13.2.
- No existing table or row changes. The upgrade uses the same verified backup, single transaction, foreign key check, audit event and rollback as before.

## 4. Safety of contributions

1. `validateContribution`: not zero, within the largest amount, a real date, a note of at most 100 characters, a withdrawal never more than what is saved, checked addition for money put in.
2. `GoalStore::addContribution`: plan check, owner from the Session, then read, check and insert in one database transaction.
3. The trigger refuses any saved amount below zero or above 999,999,999,999, even from plain SQL.
4. Only one copy of Finora can run (the lock from Milestone 3).
5. A contribution never creates or changes a transaction, so balances and budgets stay the same (tested at the store and on the dashboard).

## 5. Final verification

| Check | Result |
|---|---|
| Clean rebuild, debug (`--clean-first`) | 104 files compiled, **0 warnings**, 0 errors |
| Clean rebuild, release (`--clean-first`), including `Finora.exe` | 104 files compiled, **0 warnings**, 0 errors |
| Debug tests | 392 core, 170 database, 256 interface, 106 account, 244 billing: **1,168 checks, 0 failed** |
| Release tests | **1,168 checks, 0 failed** |
| `ctest --preset release` | 5 of 5 passed |
| Packaged program | Every DLL that `Finora.exe`, `libsodium-26.dll` and the billing tests load was followed through the release folder: 31 DLLs, 0 missing. `platforms/qwindows.dll` and `sqldrivers/qsqlite.dll` are deployed. With only Windows on the PATH (no Qt, no compiler, no plugin path), the core, database, account and billing tests pass: 392, 170, 106 and 244 checks, including the database tests' interrupted upgrade, whose second copy also starts from the release folder alone |
| Upgrade on copies of the real file | **103 checks, 0 failed**, run again with the final libraries (section 6) |
| Exchange rates against an independent calculation | **200,095 conversions, 0 differences**, run again with the final libraries (section 7) |

## 6. The upgrade tested on copies of the real database

A scratch program (outside the repository) linked Finora's own release `DatabaseManager` and ran it on copies of the real `finora.db`. The copy was made while Finora was closed and no journal existed; its SHA-256 matched the real file exactly. The program refuses any path inside `%LOCALAPPDATA%\Finora`, and it compared every table row by row, every column, and SQLite's `AUTOINCREMENT` counters.

The real file at that time: layout 3, 3 accounts, 30 recovery codes, 0 transactions, 0 budgets, 1 installation setting, 4 personal settings, 3 subscriptions, 1 payment, 4 sessions and 20 audit lines.

| Scenario | What happened | Result |
|---|---|---|
| A. Clean upgrade | The copy upgraded to layout 4 | All 11 old tables identical, including every password hash, recovery code, setting, subscription and payment; exactly one new audit line ("Schema version 3 to 4. Backup: ..."); the new tables empty; trigger and indexes present; integrity "ok"; no broken foreign key. The backup is layout 3, passes the integrity check and holds every table, row and counter of the file before. Opening again makes no second backup |
| B. With records | A second copy got 4 transactions (PKR, USD, EUR and one from before accounts), 2 budgets and a setting, then upgraded | The same checks, all identical |
| C. Failed upgrade, then retry | A copy with a table called `goal_contributions` (as if made by another tool) | The upgrade was refused with "nothing in it was changed" and the backup's path; the file stayed **byte-for-byte the same**; no journal left; the backup of the attempt was complete. After the other table was removed, the next start upgraded the file and every old row was identical |
| D. Restoring a backup | The backup from A copied over `finora.db`, as a person would restore it | The restored file was the original data (layout 3, every row and counter), and it upgraded again correctly |
| E. A crash during the upgrade | A helper program began a transaction, made the layout-4 changes, set the layout to 4, changed an account's name and wrote 3,000 rows, so that changed pages reached the file itself; then it was killed before `COMMIT` | A hot journal was left. On the next open, SQLite rolled the transaction back by itself: no row of the crash survived, every old row was identical, and the upgrade ran again with a new, complete backup |

**Finding from E, now fixed.** With a hot journal, the read-only layout check before the start-up question could not read the file (it returned -1), so the question was skipped and the upgrade ran straight away. No data was lost, but the person was not asked. The fix (approved on 31 October 2026):

- `DatabaseManager::setUpgradeAllowed(false)` makes `open()` refuse to upgrade a file that holds records; it changes no saved record and makes no backup, and `upgradeNeeded()` says why.
- `openDatabaseAtStartup` (new, `src/ui/DatabaseStartup.cpp`) asks first when the layout can be read. When it cannot, it opens the file without permission, which lets SQLite restore the last saved state; then it reads the layout again, asks, and opens with permission only after "Upgrade and continue".
- **Regression tests with a real interruption** (a second copy of the test program is killed in the middle of an upgrade-like transaction, `tests/InterruptedUpgrade.h`):
  1. `finora_db_tests`: without permission, `open()` refuses, SQLite restores layout 1 with every transaction unchanged, and no backup is made; with permission, the upgrade runs with a verified backup. An older file without a crash is refused byte-for-byte unchanged; new and up-to-date files still open.
  2. `finora_ui_tests`: the real start-up steps ask once, about layout 3; "Quit without changes" keeps the last saved state with no backup; "Upgrade and continue" upgrades with a verified backup of layout 3; an up-to-date file opens without a question.

Scenario E still passes on copies: scratch programs allow upgrades, like the tests, and the upgrade after the crash keeps every old row.

## 7. Exchange rates checked against an independent calculation

A Node.js script (outside the repository) compared Finora's own `convertAmount` with an exact calculation in `BigInt` that uses a quotient and a remainder instead of Finora's formula.

- **200,095 cases:** every pair of currencies in both directions at 1 USD = 278.5 PKR and 1 EUR = 301.2 PKR; exact halves; 200,000 random amounts and rates, including missing rates and the largest values.
- **0 differences.** 187,351 conversions matched to the paisa or cent; the 193 exact halves were all rounded away from zero.
- **Every refusal was justified:** 11,069 for a missing rate, and 1,675 where the working number really would not fit in a `long long`. Every accepted conversion fits, so the guard is exact: never too early, never too late.
- **50,000 round trips** USD or EUR to PKR and back returned the original amount every time (with rates of at least 1 PKR, because PKR is the finer unit).
- **Direction:** "1 USD = 278.5 PKR" multiplies when converting to PKR and divides when converting from PKR. Both directions are also in `finora_tests` and on the Currency page in `finora_ui_tests`.

## 8. The real data file

- **None of the test runs wrote to it.** Before testing, its SHA-256 was recorded (`3DF95260...BADF6E`). DB Browser for SQLite had the file open; nothing was closed.
- **At 18:07:09 the Milestone 5 `Finora.exe` itself upgraded the real file to layout 4**, through its own "Upgrade and continue" question (its audit line: "Schema version 3 to 4. Backup: finora-v3-backup-2026-10-31_18-07-08.db"). Sign-ins and sign-outs followed until 18:31. The scratch runs started only at 20:25, on copies.
- **That real upgrade was checked read-only afterwards:**
  - the backup Finora made at 18:07 is identical, in all 11 tables, to the copy taken before testing;
  - the real file is layout 4, its integrity check says "ok", it has no broken foreign key, 13 tables and the trigger, and no goals yet;
  - every account (including the password hashes), recovery code, transaction, budget, subscription, payment and setting value is unchanged;
  - the only differences are normal use: 2 sign-ins (2 sessions, 1 last sign-in time, 4 audit lines) and the theme setting saved again with the same value.
- **After that, the real file was only fingerprinted**, never opened by Finora's code: its SHA-256 (`CB7728DD...A17E5E`) was the same before and after the fixes and all final tests. Its three backups were left untouched.

## 9. Screenshots

Made by `finora_ui_tests --screenshots` on the real windows: the Goals page on Free, on Plus (light and dark) and after the plan ended; the goal form, the money form and a goal's history; the Currency page (light and dark), with the Amount box on a row of its own.

## 10. Status

Milestone 5 is complete. The Reports page (Milestone 6) and the Business team features (they need Finora Online) are still planned.
