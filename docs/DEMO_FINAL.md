# Finora: Final Demonstration Guide

A 15-minute script for the final presentation: every main feature of Finora, in an order that tells one story. Each step says what to do, what the audience sees and what to say. The earlier guides (`DEMO_MILESTONE3.md`, `DEMO_MILESTONE4.md`, `DEMO_MILESTONE5.md`) have more detail on each part.

Three things to say honestly whenever they come up:
- **Payments are simulated.** No card details are collected and no money moves.
- **Exchange rates are typed by the person.** Finora never downloads them, and converted totals are estimates.
- **Accounts are local.** There is no server; Business team features and cloud sync need Finora Online, which is planned.

## Before the presentation

1. Run `build.bat` and check that the last lines say **100% tests passed, 0 tests failed out of 5**.
2. Close every Finora window, and close DB Browser for SQLite if it has a Finora file open.
3. **Use a separate demonstration file, not your real data.** Start Finora from a command prompt in the `build\release` folder:

   ```bat
   Finora.exe --database "%USERPROFILE%\Documents\FinoraDemo\finora.db" --billing-demo
   ```

   `--database` makes Finora create and use that file instead of `%LOCALAPPDATA%\Finora\finora.db`. `--billing-demo` adds the "Demo: end this period now" button to the Subscription page.
4. Prepare the demonstration file once, the day before:
   - create an account (for example "Amna Siddiqui", `amna@example.com`, password `Correct Horse 9!`) and keep the recovery codes;
   - add about ten transactions over the last three months in PKR, one or two in USD and one in EUR (salary, rent, food, transport, an online course);
   - create two budgets (Food and Transport in PKR).
   Then the demonstration starts with real-looking data and takes no typing time.
5. Have `docs/PRESENTATION_NOTES.md` open for the questions.

## The script

| # | Do this | The audience sees | Say this |
|---|---|---|---|
| 1 | Start Finora with the command above | The sign-in window: "Local demonstration accounts" | "Everything stays on this computer. The password is stored only as an Argon2id hash." |
| 2 | Sign in | The dashboard: one wallet card per currency, the money-flow chart, budget alerts | "PKR, USD and EUR are never added together. Money is stored as whole paisa in a `long long`, and every total uses checked addition." |
| 3 | **Transactions**: add an expense of `1,200.50` PKR, Food; then search for "food" | The new row; the filtered list | "The form checks the amount, the date and the category; SQLite checks them again with CHECK rules." |
| 4 | **Budgets** | The Food budget's ring and status | "80% is a warning, 100% is reached. The rule is `5 x spent >= 4 x limit`: whole numbers only." |
| 5 | **Subscription**: compare the plans, then subscribe to Plus | The plan cards and the simulated checkout | "Four plans. The plan is checked inside the operations, not only on screen. This payment is simulated: no card, no money." |
| 6 | **Goals**: create "New laptop", PKR 250,000, 5 months ahead; Add money 100,000 | 40%, "Save about PKR 25,000.00 a month" | "Rounded up with ceiling division. Adding money is not a transaction, so the dashboard balance does not change." |
| 7 | **Currency**: enter 278.5 (USD) and 301.2 (EUR); convert USD 100 to PKR | "USD 100.00 = PKR 27,850.00"; the estimate of all balances | "Rates have 4 decimals stored as whole numbers. Rounding is half away from zero, done once." |
| 8 | **Reports**: October, PKR; then Year; then Categories "Expenses only"; then USD | Income, expenses, net, "more than September", categories with shares, month by month | "Any month or year, one currency at a time. The totals equal SQLite's own sums; a test checks that." |
| 9 | **Export report**, then open the CSV in Excel | The report as a spreadsheet | "Names with commas, quotes or formulas are made safe, against CSV injection." |
| 10 | **Settings**: switch to the dark theme, then back | The whole program changes theme | "Every colour has a name; one style sheet serves both themes." |
| 11 | **Subscription**: "Demo: end this period now" twice; then open Goals | Back on Free; the goal is still there, locked | "A downgrade never deletes data." |
| 12 | Sign out | The sign-in window | "Sessions also end after 15 minutes without activity and after 12 hours." |

## If something goes wrong

- **"Finora is already open":** another copy is running; close it from the taskbar.
- **A question about upgrading the data file:** the file came from an older Finora. "Upgrade and continue" makes a verified backup first.
- **The screens:** the test program saves pictures of every screen: `build\release\finora_ui_tests.exe --screenshots <folder>`.

## Questions to expect

- "Why not `double` for money?" Most decimal fractions cannot be stored exactly in binary; whole paisa can.
- "What if the power goes off during an upgrade?" SQLite undoes the unfinished change, and Finora asks again before upgrading (`docs/VIVA_PREPARATION.md`, O12).
- "Could a Free user get a Plus feature by another screen?" No: `requireFeature()` is checked inside the stores and services; the tests call them directly.
- "Is the payment real?" No. A real payment needs a server with secret keys and verified webhooks (`docs/DESIGN.md`, section 12.9).
