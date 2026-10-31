# Finora Milestone 5: Demonstration Guide

A 10-minute script for showing Milestone 5: **savings goals** and **exchange rates that you enter**. Each step says what to do, what the audience should see, and what to say.

Two things to say clearly:
- **Savings goals are part of Finora Plus,** and the payment for Plus is **simulated**. Never describe it as a real payment.
- **Finora never downloads exchange rates.** Every rate on the Currency page was typed by the person, and every converted total is an estimate.

## Before the demonstration

1. Run `build.bat` and check that the last lines say **100% tests passed, 0 tests failed out of 5**.
2. Close every Finora window. Only one copy can run at a time.
3. Start Finora as `Finora.exe --billing-demo --database "%USERPROFILE%\Documents\FinoraDemo\finora.db"` from the `build\release` folder. The Subscription page then offers "Demo: end this period now", and a separate demonstration file is used instead of your real data (see `docs/DEMO_FINAL.md`).
4. Sign in with a demonstration account on **Finora Free** that has a few transactions in PKR and at least one in USD.
5. Open `src/core/Goal.cpp`, `src/core/ExchangeRates.cpp`, `src/database/GoalStore.cpp` and `DatabaseManager::upgradeToVersion4` in your editor, ready to show.

## Upgrading an older file

If your data file was made by Phase 4C (layout 3), the first start asks: "Your data file uses layout 3. Finora will upgrade it to layout 4, which adds savings goals."

- Say: "Finora asks first, saves a complete backup in the `backups` folder and checks it. The upgrade only adds two empty tables and a trigger; no existing row changes. If any step fails, everything is rolled back."
- Click **Upgrade and continue**.

## The script

| # | Do this | The audience sees | Say this |
|---|---|---|---|
| 1 | Open **Goals** on Finora Free | A violet banner: "Savings goals" is part of Finora Plus; "No savings goals yet"; a small lock on **New Goal** | "The page never pretends. On Free, goals are locked, and the banner says exactly what Plus would allow." |
| 2 | Click **+ New Goal** | "Savings goals" explanation with **See plans** and **Not now** | "This lock only saves a wasted form. `GoalStore` itself refuses a new goal on Free, and the tests call it directly." Click **See plans** |
| 3 | On the Subscription page, subscribe to **Plus** with the simulated checkout | "Simulated payment: Finora collects no card details and no money moves"; then the sidebar says "Finora Plus" | "The plan switches on only with a succeeded payment of this same person for this same plan." |
| 4 | Open **Goals**, click **+ New Goal**: "New laptop", PKR, `250,000`, tick "Reach it by a date" and choose a date 5 months ahead | A card: "In progress", 0% saved, "Save about PKR 41,666.67 a month ... (6 months, counting this one)" | "250,000 over 6 months is 41,666.666..., rounded UP to the next paisa, so saving it each month really reaches the goal. That is ceiling division: `(a + b - 1) / b`." |
| 5 | Click **Add money**, type `100,000`, add a note | "Saved afterwards: PKR 100,000.00"; after saving, 40% and "Save about PKR 25,000.00 a month" | "Reading the saved amount, checking it and saving happen in one database transaction." |
| 6 | Click **Take out** and type `150,000` | The form says "You can take out at most what is saved." and stays open | "The rule is in `validateContribution`, the store checks it again, and a trigger inside SQLite is the third guard." Click **Cancel** |
| 7 | Click **Take out**, type `20,000`, save | PKR 80,000.00 saved; a toast "Money taken out of the goal." | "A goal is money I set aside, not a transaction, so my balance on the dashboard does not change." |
| 8 | Click **History** | Two entries, newest first, with their notes | "The history is never edited. A mistake is corrected with a new entry, so it always explains the saved amount." |
| 9 | Click **Edit** and try to change the currency | The currency box cannot be changed | "The saved PKR 80,000 is in rupees, so the currency is fixed once money is in the goal." Click **Cancel** |
| 10 | Open **Currency** | "Finora never downloads exchange rates"; empty rate boxes; "No rates saved yet." | "The rates are mine. Finora shows the date I saved them." |
| 11 | Type `278.5` for USD and `301.2` for EUR, click **Save rates** | The boxes show `278.5000` and `301.2000`; "Saved on ..." | "A rate has 4 decimal places and is stored as a whole number: 278.5 is 2785000. No `double` anywhere." |
| 12 | In **Convert an amount**, type `100`, from USD to PKR | "USD 100.00 = PKR 27,850.00", "At 1 USD = 278.5000 PKR. Rounded to the nearest paisa or cent; an exact half is rounded away from zero." | "The formula is amount times rate(from) divided by rate(to), with checked multiplication before it." |
| 13 | Choose EUR to USD with `10` | "EUR 10.00 = USD 10.82" | "Through PKR, but rounded only once." |
| 14 | Point at **All your balances in one currency** and switch between PKR and USD | One line per currency and "≈" totals | "This is only an estimate. My records stay in their own currencies and are never changed." |
| 15 | Type `27a` for USD and click **Save rates** | "USD rate: A rate can contain only digits and one decimal point, for example 278.5." | "Invalid input never reaches the database." |
| 16 | On **Subscription**, click "Demo: end this period now" twice | The plan goes to "payment due", then back to Finora Free | "This button exists only with `--billing-demo`." |
| 17 | Open **Goals** | The goal and its PKR 80,000.00 are still there; locks on Add money, Take out and Edit; History and Delete still work | "Nothing is deleted when a plan ends. The person can still see and delete their own goal." |

## What to show in the code

- `validateContribution` in `src/core/Goal.cpp`: the withdrawal rule and the checked addition.
- `GoalStore::addContribution`: `db.transaction()`, the saved amount, the rules, the `INSERT`, `commit()`.
- `DatabaseManager::upgradeToVersion4`: the two tables and the trigger.
- `convertAmount` in `src/core/ExchangeRates.cpp`: the checked multiplication and the rounding.

## Questions to expect

- "What if two copies of Finora add money at the same time?" Only one copy can run (the lock from Milestone 3), each contribution is checked in one transaction, and the trigger refuses any sum outside the allowed range.
- "Why is 41,666.666... shown as 41,666.67 and not 41,666.66?" Because the amount per month is rounded up, so the goal is really reached.
- "Can a Free user add money by another screen?" No. `GoalStore` checks the plan itself; the screens only explain it.
