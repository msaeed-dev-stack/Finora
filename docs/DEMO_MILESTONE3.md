# Finora Milestone 3: Demonstration Guide

A 10-minute script for showing the Transactions page to classmates or to the instructor. Each step says what to do, what the audience should see, and what to say.

## Before the demonstration

1. Run `build.bat` and check the last lines say **100% tests passed, 0 tests failed out of 3**.
2. If your real database already holds records you care about, back it up first: close Finora, press **Win + R**, type `%LOCALAPPDATA%\Finora`, and copy `finora.db` somewhere safe.
3. Close every Finora window. Only one copy can run at a time.
4. Open `src/core/Money.cpp` and `src/ui/TransactionsPage.cpp` in your editor, ready to show.

## The script

| # | Do this | The audience sees | Say this |
|---|---|---|---|
| 1 | Start `build\debug\Finora.exe` and click **Transactions** | An empty table with "No transactions yet" | "This page reads every transaction from the SQLite database when Finora starts." |
| 2 | Click **+ Add Transaction** | The form opens with Expense selected and today's date | "One form is used for both adding and editing. It is the `TransactionDialog` class." |
| 3 | Type `12.345` as the amount and click **Save Transaction** | Red text: "Use at most two digits after the decimal point." | "The form refuses bad input and stays open. Money is never rounded silently." |
| 4 | Change the amount to `1,200.50`, choose Food, type a description such as `Groceries from Ali's shop`, and save | "Will be saved as PKR 1,200.50" while typing; then a new row and a green "Transaction added." | "1,200.50 is stored as the whole number 120050 paisa, so adding amounts is always exact." |
| 5 | Add an **Income** of `1,20,000` PKR, category Salary | A green row "PKR 120,000.00" | "The Pakistani lakh style with commas works too. Income is green and expenses are red." |
| 6 | Add an **Income** of `500` **USD**, category Freelance | A row "USD 500.00" | "Each transaction keeps its own currency. Finora never adds rupees and dollars together." |
| 7 | Type `groc` in the search box | Only the groceries row remains; "Showing 1 of 3 transactions" | "Search ignores upper and lower case and looks in the description and the category." |
| 8 | Clear the search, then set Type to **Income** and Currency to **USD** | Only the USD row | "Filters combine. They loop over the list in memory with a plain `for` loop." |
| 9 | Click **Clear filters**, select the groceries row, click **Edit**, change the amount to `1,500` and save | The row shows "PKR 1,500.00" and "Transaction updated." | "Edit runs an SQL `UPDATE ... WHERE id = :id`, so only this row changes." |
| 10 | Select a row, click **Delete**, answer **No** | The row is still there | "Deleting always asks first, and No is the default button." |
| 11 | Click **Delete** again and answer **Yes** | The row disappears; "Transaction deleted." | "Only Yes runs the SQL `DELETE`." |
| 12 | Close Finora and start it again, then open Transactions | The same rows are still there | "The data lives in `finora.db`, so it survives closing the program." |
| 13 | With Finora open, start `Finora.exe` a second time | A box: "Finora is already open" | "Only one copy can use the database, so two copies can never save over each other." |

## What to show in the code

1. **`parseAmount` in `src/core/Money.cpp`.** Point at the `for` loop and the line `wholeUnits = wholeUnits * 10 + digit`. Explain that "1200" becomes 1, 12, 120, then 1200.
2. **`onAddClicked` in `src/ui/TransactionsPage.cpp`.** Show `dialog.exec() == QDialog::Accepted`, then `saveNewTransaction`.
3. **`updateTransaction` in `src/database/DatabaseManager.cpp`.** Show the placeholders such as `:amount` and explain that values are bound separately, so an apostrophe in "Ali's" can never break the SQL.

## How the data travels, in one picture

```text
Button click ──signal──> TransactionsPage slot
      │
      ▼
TransactionDialog form ──> parseAmount + validateTransaction  (plain C++ rules)
      │ valid
      ▼
DatabaseManager::addTransaction ──> prepared INSERT ──> finora.db (SQLite)
      │
      ▼
loadTransactions ──> std::vector<Transaction> ──> filterTransactions ──> table on screen
```

## Questions classmates often ask

- **"Why not store money as a `double`?"** A `double` cannot hold 0.10 exactly, so totals drift. Whole paisa in a `long long` never drift.
- **"Where is the database?"** In `%LOCALAPPDATA%\Finora\finora.db`, one file per Windows user.
- **"What if I type letters in the amount?"** The form says "An amount may contain only digits, commas and one decimal point." and stays open.
- **"Is it tested?"** Yes. Three test programs with 284 automated checks, including tests that click the real buttons and answer the pop-ups.
- **"Did you use any outside help?"** Answer honestly, as described in `docs/VIVA_PREPARATION.md`, question J5.

## If something goes wrong during the demo

| Problem | Fix |
|---|---|
| "Finora is already open" when you did not expect it | Another Finora window is open, maybe minimised. Close it from the taskbar |
| The build fails with "Permission denied" | Finora is still running. Close it and build again |
| You want to start over with an empty list | Close Finora and delete `finora.db` in `%LOCALAPPDATA%\Finora`. This deletes all records, so back it up first if needed |
