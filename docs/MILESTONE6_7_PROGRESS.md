# Milestones 6 and 7 Progress Record

Milestone 6: reports and export. Milestone 7: final testing and delivery. Date: 31 October 2026.

## 1. Milestone 6: what the roadmap asked for, and what was missing

| Roadmap item | Before Milestone 6 | Done in Milestone 6 |
|---|---|---|
| Financial summaries by period | Only the current month, on the dashboard | Any month or any year (`periodBounds`, `totalsBetween`) |
| Summaries by category | Expenses of one month on the dashboard | Income and/or expense categories of any month or year, with shares and counts (`categoriesBetween`) |
| Summaries by transaction type | Income and expenses of this month | Income, expenses and net for the period, and a category filter by type |
| Summaries by currency | One wallet card per currency | Every report is for one currency; other currencies of the period are named, never added |
| Monthly and yearly comparisons | A 6-month chart on the dashboard | The change from the previous month or year, and month by month (6 months, or the 12 months of a year) |
| CSV export with safe quoting | All transactions (Plus), with `csvField` | The report itself as a CSV file, through the same `csvField` |

**Reused, not rebuilt:** `csvField`, `plainAmount`, `monthlyFlow`, `addChecked`, `isValidMonth`, `shiftMonth`, `daysInMonth`, `toLowerText`, `percentUsed`, `requireFeature`, `QSaveFile` writing, the plan-lock pattern of the Goals page, `MonthSwitcher`. The transactions CSV (Plus), the monthly PDF (Pro) and the dashboard are unchanged.

## 2. Milestone 6: what was built

| Part | Files |
|---|---|
| Core rules | New `src/core/Reports.h/.cpp`: `periodBounds`, `previousPeriodOf`, `totalsBetween`, `netOf`, `categoriesBetween`, `buildPeriodReport`, `reportCsv` |
| Plan check and data | `ReportExporter::buildReport` and `exportReportCsv` (`src/reports`): `requireFeature(FEATURE_REPORTS)` (and `FEATURE_CSV_EXPORT` for the file) before any record is read; only the signed-in person's records; the audit line `report_exported` |
| Entitlements | `src/core/Plans.h`: "Monthly and yearly reports" (Finora Plus) changed from planned to built |
| Screen | New `src/ui/ReportsPage.h/.cpp`; `Sidebar.cpp` (no "Soon" badge left); `MainWindow.cpp`; one style in `finora.qss` |
| Tests | `finora_tests` (31 checks), `finora_billing_tests` (12), `finora_ui_tests` (11, and one older check made stronger: no sidebar page says "coming soon") |

Design details: `docs/DESIGN.md`, section 14.

## 3. Milestone 7: what was done

1. **`Finora.exe --database <file>`** (`src/main.cpp`): opens another data file, so the packaged program can be started and demonstrated without the real data file. Only active when the option is given.
2. **Clean builds** of debug and release, every test program in both, CTest, the DLL check, and the release test programs with only Windows on the PATH (section 4).
3. **The packaged `Finora.exe`** was started from the release folder with only Windows on the PATH and a separate data file: it started, showed its sign-in window and closed normally (section 5).
4. **The real data file** was never opened by any test; its SHA-256 was compared before and after (section 6).
5. **Documents:** `README.md`, `docs/DESIGN.md` (sections 1, 14 and 15), `docs/VIVA_PREPARATION.md` (section P), `docs/REQUIREMENTS_CHECKLIST.md` (rows for Milestones 6 and 7), `docs/CPP_CONCEPTS_MAP.md`, `docs/DEMO_MILESTONE5.md`, and three new ones: this record, `docs/DEMO_FINAL.md` (the final demonstration) and `docs/PRESENTATION_NOTES.md` (the technical material for the presentation). `THIRD_PARTY_NOTICES.md` needed no change: no new library was added.

## 4. Final verification

| Check | Debug | Release |
|---|---|---|
| Clean rebuild (`--clean-first`) | 107 files, **0 warnings**, 0 errors | 107 files, **0 warnings**, 0 errors |
| `finora_tests` (core) | 423 checks, 0 failed | 423 checks, 0 failed |
| `finora_db_tests` | 170 checks, 0 failed | 170 checks, 0 failed |
| `finora_ui_tests` | 267 checks, 0 failed | 267 checks, 0 failed |
| `finora_auth_tests` | 106 checks, 0 failed | 106 checks, 0 failed |
| `finora_billing_tests` | 256 checks, 0 failed | 256 checks, 0 failed |
| **Total** | **1,222 checks, 0 failed** | **1,222 checks, 0 failed** |

- `ctest --preset release`: 100% passed, 5 of 5.
- DLL check: every DLL that `Finora.exe`, `libsodium-26.dll` and the billing tests load was followed through the release folder: 31 DLLs, 0 missing; `platforms/qwindows.dll` and `sqldrivers/qsqlite.dll` are deployed.
- The release core, database, account and billing tests also pass with only Windows on the PATH (no Qt, no compiler, no plugin path): 423, 170, 106 and 256 checks.

## 5. The packaged program

The release `Finora.exe` was started from `build\release` with only Windows on the PATH and `--database` pointing to a new file in a temporary folder:

- it started, and loaded `Qt6Core`, `Qt6Gui`, `Qt6Widgets`, `Qt6Sql`, `Qt6Svg`, `libsodium-26.dll`, `qwindows.dll` and `qsqlite.dll` from its own folder;
- it created the new data file (layout 4) and showed "Welcome to Finora" with the note "Local demonstration accounts";
- it closed normally when asked (exit code 0) and left no lock file behind.

Every screen and workflow (sign-up, sign-in, sign-out, recovery, dashboard, transactions, budgets, goals, currency, reports, exports, settings, subscription, both themes) is exercised on the real windows of the same release build by `finora_ui_tests`, which also saved the screenshots of section 7. The packaged program itself was not clicked through by hand.

## 6. The real data file

- **No test opened it.** Every test program works in a temporary folder, and the packaged program was started with `--database` on a separate file.
- Its SHA-256 did change during Milestone 6, and Finora's own audit history shows why: between 21:23 and 21:30 the real Finora was used: four failed sign-ins, then a new (fourth) account was created, then Finora was closed. No test or tool of this work was running against it at that time.
- Read-only check afterwards: layout 4, SQLite's integrity check "ok", no broken foreign key, 4 accounts; its three backups (`finora-v1-...`, `finora-v2-...`, `finora-v3-backup-2026-10-31_18-07-08.db`) are untouched. No migration, reset or overwrite was run on it.

## 7. Screenshots

`finora_ui_tests --screenshots <folder>` on the release build saved 65 pictures of the real windows, and the packaged-program check saved one more (66 in all):

- every page (`page_0` to `page_7`), the dashboard, transactions, budgets and settings in light and dark (`m4_*`);
- the sign-in window, sign-up, recovery codes and reset (`b_auth_*`); the Subscription page, the checkout and the audit history (`c_*`);
- the Goals page on Free, Plus (light and dark) and after a downgrade, the goal and money forms and a goal's history, the Currency page in light and dark (`m5_*`);
- the Reports page on Free, a month, a year, and a year in dark (`m6_*`);
- the packaged program's welcome window (`m7_packaged_signin`).

They are working files outside the repository; run the same command to make them again.

## 8. Remaining limitations

- **Simulated payments** and **local accounts**: no server. Real payments, cloud sync, online sign-in and the Business team features need Finora Online, which is planned.
- **Exchange rates** are typed by the person; Finora never downloads them. Converted totals are estimates.
- **Reports** cover one currency and a whole month or year; there is no custom date range. The PDF is still the monthly summary of Phase 4C.
- **Planned extras** are labelled "Soon" in the program: recurring transactions, advanced filters, better insights (Plus); multiple accounts, advanced analytics, custom categories, net worth, a financial calendar, advanced planning (Pro).
- **Plans are enforced on this computer.** Whoever owns the PC can edit the data file or the clock; real enforcement needs Finora Online.
