# Finora Milestone 4: Demonstration Guide

A 12-minute script for showing Phase 4A to classmates or to the instructor: the dashboard, budgets, the light and dark themes, and the safe database upgrade. Each step says what to do, what the audience should see, and what to say.

Phase 4B (accounts) is described at the end of this guide. Phase 4C (subscription plans) is **planned and not yet implemented**. Do not show or describe it as working.

## Before the demonstration

1. Run `build.bat` and check that the last lines say **100% tests passed, 0 tests failed out of 3**.
2. Close every Finora window. Only one copy can run at a time.
3. Decide which database to show:
   - **To show the upgrade:** keep your Milestone 3 `finora.db`. The first start of the new Finora will ask to upgrade it (step 1).
   - **To start empty:** close Finora, press **Win + R**, type `%LOCALAPPDATA%\Finora`, and move `finora.db` to a safe place. A new, empty file is created at the next start.
4. Open `src/core/Budget.cpp`, `src/core/Totals.cpp`, `src/ui/Page.h` and `src/database/DatabaseManager.cpp` in your editor, ready to show.

## The script

| # | Do this | The audience sees | Say this |
|---|---|---|---|
| 1 | Start `build\debug\Finora.exe` with a Milestone 3 database | "This version of Finora stores your records in a new layout", with the backup folder | "Finora asks before changing my data. It saves a complete backup and checks it first. If any step fails, everything is rolled back." |
| 2 | Click **Upgrade and continue** | The dashboard. Any transactions from Milestone 3 are already counted in the wallet cards | "Every record kept its id and amount. The tests prove a failed upgrade leaves the file byte-for-byte unchanged." |
| 3 | Point at the three wallet cards | One card each for PKR, USD and EUR | "Finora never adds rupees and dollars together. Each currency has its own balance." |
| 4 | Click **+ Add Transaction** and save an **Income** of `120,000` PKR, category Salary | The PKR card's balance and "Income in" this month go up; a toast says "Transaction added." | "The dashboard uses the same form as the Transactions page. Every number comes from SQLite." |
| 5 | Add two **Expenses** in PKR, category Food: `16,500` and `1,580.50` | "Spent in" this month shows PKR 18,080.50 | "Totals use checked addition: before adding, Finora checks that the result fits in a `long long`." |
| 6 | Move the mouse over this month's bars in the chart | A dark bubble with income, spent and net | "I drew this chart myself with QPainter. `paintEvent` is a virtual function that I override." |
| 7 | Press **Tab** until the chart has a ring, then press **Left** | The highlight moves to last month; the sentence under the chart changes | "Keyboard users get the same information, in words." |
| 8 | Click **USD**, then **12 months** | Only dollar amounts, twelve months | "The chart shows one currency at a time." |
| 9 | Open **Budgets** and click **Create your first budget**: Food, PKR, `20,000` | A card with a ring at 90% and an amber "Nearing limit" chip; "Left PKR 1,919.50" | "18,080.50 of 20,000 is 90%. The 80% rule is `5 × spent >= 4 × limit`, so it uses whole numbers only." |
| 10 | Click **+ New Budget** again and choose `food`, PKR | A red message: "There is already a Food budget in PKR." The form stays open | "One budget per category and currency. The database has a UNIQUE index as a second guard." Click **Cancel** |
| 11 | Back on the dashboard, add a Food expense of `2,500` PKR. Open **Budgets** | The Food ring is full at 102%, "Over budget", "Over by PKR 580.50" | "Exactly the limit is 'Limit reached'; more than the limit is 'Over budget'. The words always say the status, not only the colour." |
| 12 | Click the **<** arrow next to the month | Last month: Food is back at 0% | "A budget repeats every month; spending is counted again for each month." |
| 13 | Open the **Dashboard** | "Budget alerts" lists Food | "MainWindow calls `refresh()` on every page it shows. That is polymorphism: each page has its own `refresh()`." |
| 14 | Click **Dark mode** at the bottom of the sidebar | The whole program turns dark; a toast says "Dark theme on" | "Every colour has a name, such as `surface`. The style sheet uses the names, and Finora fills in the light or dark values." |
| 15 | Open **Settings**, tick **Reduce motion**, then click **Light** | Changes appear at once, without fading | "This helps people who find animations uncomfortable." |
| 16 | Click **View font licences** | The full SIL Open Font License texts of Inter and Manrope | "Both fonts are open source. Their licences ship with the program." |
| 17 | Close Finora and start it again | Finora opens in the theme you left it in | "The theme is saved in the `app_settings` table." |

## Phase 4B: accounts (about 8 minutes)

Finora now starts with the sign-in window. If this computer has no account yet, it shows "Welcome to Finora".

| # | Do this | The audience sees | Say this |
|---|---|---|---|
| B1 | Point at the violet panel | "Local demonstration accounts: your account, password and records stay on this computer." | "Accounts are real but local: no server, no email." |
| B2 | Click **Create an account**; type a name and email; type `correcthorse` as the password | The meter: "Strength: Weak · Mix letters with numbers or symbols." | "The meter is plain C++ in `PasswordPolicy.cpp`; the words always say the strength." |
| B3 | Change it to `Correct Horse 9!`, click the eye button, then click it again | "Very strong"; the password shows, then hides | "The eye is a real button, so the Tab key reaches it." |
| B4 | Repeat the password and click **Create account** | A short "Creating your account...", then 10 recovery codes | "The password is hashed with Argon2id: 256 MiB and 3 passes, about two thirds of a second. Only the hash is stored." |
| B5 | Point at the codes, tick "I have saved these codes", click **Continue to Finora** | If older records exist: "Move them into my account?" with "Not now" as the default | "Finora never moves older records without asking." |
| B6 | Show the dashboard and the sidebar | "Good evening, Muhammad"; initials, name and email; Sign out | "The Session knows who is signed in; only AuthService can change it." |
| B7 | Click **Sign out**, then sign in with a wrong password | "The email or password is not correct." | "The same words for an unknown email, so nobody learns which accounts exist." |
| B8 | Click **Forgot your password?** and use one of the codes | "Your password was changed. You have 9 recovery codes left." | "A code works once; it is marked as used in the same transaction." |
| B9 | Create a second account and open its dashboard | Empty wallets and no transactions | "Every query says `user_id IS :owner`; the second person cannot even find the first person's records by id." |
| B10 | Open **Settings** | Profile, Security (codes left, session limits) | "Finora signs out after 15 minutes without activity and after 12 hours." |

What to show in the code for Phase 4B: `Session.h` (the friend class), `AuthService::signIn` (the general message and the dummy hash), `DatabaseManager::currentOwner`, and `third_party/libsodium-1.0.22/VERIFICATION.md`.

## What to show in the code

1. **`budgetStatusFor` in `src/core/Budget.cpp`.** Four `if` statements in a fixed order: over, reached, warning, on track. Explain why `5 * spentMinor >= 4 * limitMinor` cannot overflow at that point.
2. **`addChecked` in `src/core/Totals.cpp`.** `amount > LLONG_MAX - total` is checked before adding.
3. **`Page.h` and `MainWindow::showPage`.** `virtual void refresh()` in the base class, `override` in each page, and `pages[pageNumber]->refresh()`.
4. **`upgradeSchema` in `src/database/DatabaseManager.cpp`.** The backup comes first, then `db.transaction()`, the checks, then `commit()` or `rollback()`.

## How the data travels, in one picture

```text
Dashboard refresh()
      │
      ▼
DatabaseManager::loadTransactions + BudgetStore::loadBudgets   (SQLite)
      │
      ├──> summarizeByCurrency ──> 3 wallet cards (one currency each)
      ├──> monthlyFlow ─────────> money-flow chart (one currency, 6 or 12 months)
      ├──> allBudgetProgress ───> budget alerts (80%, 100%, over)
      └──> spendingByCategory ──> spending by category
```

## Questions classmates often ask

- **"Why not one total for all currencies?"** Rupees and dollars cannot be added without an exchange rate. Exchange rates come in Milestone 5.
- **"Why is 79.99% not a warning?"** The rule is "80% or more", calculated exactly with whole numbers. 7,999 of 10,000 is below 80%.
- **"What if the upgrade fails halfway?"** The upgrade is one database transaction. `rollback()` undoes every step, and the backup copy stays in the `backups` folder.
- **"Did you use a chart library?"** No. The chart is about 300 lines of `QPainter` code in `MoneyFlowChart.cpp`.
- **"Where are the accounts?"** The tables exist since the upgrade, but sign-in is Phase 4B and is **not built yet**.

## If something goes wrong during the demo

| Problem | Fix |
|---|---|
| You clicked **Quit without changes** by mistake | Nothing was changed. Start Finora again and click **Upgrade and continue** |
| "Finora is already open" | Another Finora window is open, maybe minimised. Close it from the taskbar |
| The build fails with "Permission denied" | Finora is still running. Close it and build again |
| You want the old Milestone 3 file back | Close Finora. Copy the file from `%LOCALAPPDATA%\Finora\backups\` over `finora.db`. Milestone 3 builds can open it; this build will ask to upgrade it again |
