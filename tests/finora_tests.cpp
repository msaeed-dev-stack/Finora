// =====================================================================
//  finora_tests.cpp
//  Automated checks for Finora's core logic (plain C++, no Qt).
//
//  Every check prints PASS or FAIL. The program returns 0 only when all
//  checks pass, so CTest (and build.bat) can report the result.
// =====================================================================
#include <climits>
#include <iostream>
#include <string>
#include <vector>

#include "core/AccountRules.h"
#include "core/Budget.h"
#include "core/CsvExport.h"
#include "core/Greeting.h"
#include "core/Money.h"
#include "core/PasswordPolicy.h"
#include "core/Plans.h"
#include "core/Session.h"
#include "core/Subscription.h"
#include "core/Totals.h"
#include "core/Transaction.h"
#include "core/TransactionFilter.h"

// Counts how many checks ran and how many of them failed.
struct TestCounter {
    int checksRun = 0;
    int checksFailed = 0;
};

// Compares a true/false result with the expected answer.
void checkBool(TestCounter &counter, const std::string &testName, bool actual, bool expected)
{
    counter.checksRun++;

    if (actual == expected) {
        std::cout << "PASS  " << testName << "\n";
    } else {
        counter.checksFailed++;
        // std::boolalpha prints a bool as the word true or false instead of 1 or 0.
        std::cout << "FAIL  " << testName << "   expected " << std::boolalpha << expected
                  << " but got " << actual << "\n";
    }
}

// Compares a whole-number result with the expected number.
void checkInt(TestCounter &counter, const std::string &testName, int actual, int expected)
{
    counter.checksRun++;

    if (actual == expected) {
        std::cout << "PASS  " << testName << "\n";
    } else {
        counter.checksFailed++;
        std::cout << "FAIL  " << testName << "   expected " << expected
                  << " but got " << actual << "\n";
    }
}

// Compares the text a function returned with the text we expected.
void checkText(TestCounter &counter, const std::string &testName,
               const std::string &actual, const std::string &expected)
{
    counter.checksRun++;

    if (actual == expected) {
        std::cout << "PASS  " << testName << "\n";
    } else {
        counter.checksFailed++;
        std::cout << "FAIL  " << testName << "   expected \"" << expected
                  << "\" but got \"" << actual << "\"\n";
    }
}

// Compares an amount in minor units. Amounts need a long long, because
// they can be far larger than the biggest int (about 2.1 x 10^9).
void checkAmount(TestCounter &counter, const std::string &testName, long long actual, long long expected)
{
    counter.checksRun++;

    if (actual == expected) {
        std::cout << "PASS  " << testName << "\n";
    } else {
        counter.checksFailed++;
        std::cout << "FAIL  " << testName << "   expected " << expected
                  << " but got " << actual << "\n";
    }
}

// One test case: an input hour and the greeting it should produce.
struct GreetingCase {
    int hour;
    std::string expected;
};

// Tests greetingForHour() at the edges of every time range, because
// mistakes usually happen at the boundaries (for example < versus <=).
void testGreetingForHour(TestCounter &counter)
{
    const int CASE_COUNT = 11;
    const GreetingCase cases[CASE_COUNT] = {
        {5,  "Good Morning"},     // first morning hour
        {9,  "Good Morning"},     // normal morning hour
        {11, "Good Morning"},     // last morning hour
        {12, "Good Afternoon"},   // first afternoon hour
        {16, "Good Afternoon"},   // last afternoon hour
        {17, "Good Evening"},     // first evening hour
        {23, "Good Evening"},     // last hour of the day
        {0,  "Good Evening"},     // midnight
        {4,  "Good Evening"},     // last hour before morning
        {-1, "Hello"},            // invalid: below 0
        {24, "Hello"}             // invalid: above 23
    };

    for (int i = 0; i < CASE_COUNT; i++) {
        std::string testName = "greetingForHour(" + std::to_string(cases[i].hour) + ")";
        checkText(counter, testName, greetingForHour(cases[i].hour), cases[i].expected);
    }
}

void testLeapYearsAndMonths(TestCounter &counter)
{
    checkBool(counter, "2024 is a leap year", isLeapYear(2024), true);
    checkBool(counter, "2026 is not a leap year", isLeapYear(2026), false);
    checkBool(counter, "2000 is a leap year (divisible by 400)", isLeapYear(2000), true);
    checkBool(counter, "1900 is not a leap year (century)", isLeapYear(1900), false);
    checkBool(counter, "2100 is not a leap year (century)", isLeapYear(2100), false);

    checkInt(counter, "January has 31 days", daysInMonth(2026, 1), 31);
    checkInt(counter, "April has 30 days", daysInMonth(2026, 4), 30);
    checkInt(counter, "February 2026 has 28 days", daysInMonth(2026, 2), 28);
    checkInt(counter, "February 2024 has 29 days", daysInMonth(2024, 2), 29);
    checkInt(counter, "December has 31 days", daysInMonth(2026, 12), 31);
    checkInt(counter, "month 0 does not exist", daysInMonth(2026, 0), 0);
    checkInt(counter, "month 13 does not exist", daysInMonth(2026, 13), 0);
}

// One date test case: the text and whether it is a real YYYY-MM-DD date.
struct DateCase {
    std::string text;
    bool expected;
};

void testIsValidIsoDate(TestCounter &counter)
{
    const int CASE_COUNT = 17;
    const DateCase cases[CASE_COUNT] = {
        {"2026-10-28", true},    // a normal date
        {"2026-12-31", true},    // last day of the year
        {"2026-01-01", true},    // first day of the year
        {"2024-02-29", true},    // leap day in a leap year
        {"2000-02-29", true},    // leap day in a century leap year
        {"2026-02-29", false},   // no leap day in 2026
        {"1900-02-29", false},   // 1900 was not a leap year
        {"2026-02-30", false},   // February never has 30 days
        {"2026-04-31", false},   // April has only 30 days
        {"2026-13-01", false},   // month 13
        {"2026-00-10", false},   // month 0
        {"2026-01-00", false},   // day 0
        {"2026-1-5", false},     // missing leading zeros
        {"28/10/2026", false},   // wrong format
        {"2026-10-2x", false},   // a letter instead of a digit
        {"1899-12-31", false},   // before 1900
        {"", false}              // empty text
    };

    for (int i = 0; i < CASE_COUNT; i++) {
        checkBool(counter, "isValidIsoDate(\"" + cases[i].text + "\")",
                  isValidIsoDate(cases[i].text), cases[i].expected);
    }
}

void testCurrenciesAndBlankText(TestCounter &counter)
{
    checkBool(counter, "PKR is supported", isSupportedCurrency("PKR"), true);
    checkBool(counter, "USD is supported", isSupportedCurrency("USD"), true);
    checkBool(counter, "EUR is supported", isSupportedCurrency("EUR"), true);
    checkBool(counter, "lowercase pkr is not supported", isSupportedCurrency("pkr"), false);
    checkBool(counter, "GBP is not supported", isSupportedCurrency("GBP"), false);
    checkBool(counter, "empty currency is not supported", isSupportedCurrency(""), false);

    checkBool(counter, "empty text is blank", isBlank(""), true);
    checkBool(counter, "spaces only is blank", isBlank("   "), true);
    checkBool(counter, "text with a letter is not blank", isBlank(" a "), false);
}

// A valid transaction that each test copies and then breaks in one way.
Transaction makeValidTransaction()
{
    Transaction transaction;
    transaction.type = TYPE_EXPENSE;
    transaction.amountMinor = 120050;   // 1,200.50
    transaction.currencyCode = "PKR";
    transaction.category = "Food";
    transaction.description = "Groceries";
    transaction.date = "2026-10-28";
    return transaction;
}

void testValidateTransaction(TestCounter &counter)
{
    Transaction valid = makeValidTransaction();
    checkText(counter, "a valid transaction has no problems", validateTransaction(valid), "");

    Transaction income = makeValidTransaction();
    income.type = TYPE_INCOME;
    income.description = "";   // the description is optional
    checkText(counter, "income with an empty description is valid", validateTransaction(income), "");

    Transaction largest = makeValidTransaction();
    largest.amountMinor = MAX_AMOUNT_MINOR;
    checkText(counter, "the maximum amount is allowed", validateTransaction(largest), "");

    Transaction wrongType = makeValidTransaction();
    wrongType.type = "transfer";
    checkText(counter, "type 'transfer' is rejected", validateTransaction(wrongType),
              "Type must be income or expense.");

    Transaction zero = makeValidTransaction();
    zero.amountMinor = 0;
    checkText(counter, "amount 0 is rejected", validateTransaction(zero),
              "Amount must be greater than zero.");

    Transaction negative = makeValidTransaction();
    negative.amountMinor = -500;
    checkText(counter, "a negative amount is rejected", validateTransaction(negative),
              "Amount must be greater than zero.");

    Transaction tooLarge = makeValidTransaction();
    tooLarge.amountMinor = MAX_AMOUNT_MINOR + 1;
    checkText(counter, "an amount above the maximum is rejected", validateTransaction(tooLarge),
              "Amount is too large. The maximum is 9,999,999,999.99.");

    Transaction wrongCurrency = makeValidTransaction();
    wrongCurrency.currencyCode = "XYZ";
    checkText(counter, "currency XYZ is rejected", validateTransaction(wrongCurrency),
              "Currency must be PKR, USD or EUR.");

    Transaction blankCategory = makeValidTransaction();
    blankCategory.category = "   ";
    checkText(counter, "a blank category is rejected", validateTransaction(blankCategory),
              "Category must not be empty.");

    Transaction badDate = makeValidTransaction();
    badDate.date = "2026-02-29";
    checkText(counter, "29 February 2026 is rejected", validateTransaction(badDate),
              "Date must be a real date written as YYYY-MM-DD.");
}

// One amount test: the typed text and the minor units it should give.
// -1 means the text must be rejected.
struct AmountCase {
    std::string text;
    long long expected;
};

void testParseAmount(TestCounter &counter)
{
    const int CASE_COUNT = 22;
    const AmountCase cases[CASE_COUNT] = {
        {"1200", 120000},                      // whole number
        {"1,200.50", 120050},                  // thousands comma and paisa
        {"1,20,000", 12000000},                // Pakistani lakh grouping
        {"0.5", 50},                           // one decimal digit means 50 paisa
        {"0.05", 5},                           // smallest amount
        {".75", 75},                           // no digit before the point
        {"12.", 1200},                         // point with no digits after it
        {"  250.00 ", 25000},                  // spaces around the text
        {"9,999,999,999.99", 999999999999LL},  // the maximum
        {"", -1},                              // empty
        {"   ", -1},                           // only spaces
        {"0", -1},                             // zero
        {"0.00", -1},                          // zero with decimals
        {"-50", -1},                           // negative
        {"12.345", -1},                        // three decimal digits
        {"1.2.3", -1},                         // two decimal points
        {"12a", -1},                           // a letter
        {"1,,200", -1},                        // two commas together
        {",100", -1},                          // comma at the start
        {"100,", -1},                          // comma at the end
        {"10,000,000,000.00", -1},             // above the maximum
        {"1.5,0", -1}                          // comma after the decimal point
    };

    for (int i = 0; i < CASE_COUNT; i++) {
        long long amount = -1;   // stays -1 when the text is rejected
        std::string problem = parseAmount(cases[i].text, amount);

        if (cases[i].expected == -1) {
            checkBool(counter, "parseAmount rejects \"" + cases[i].text + "\"",
                      !problem.empty() && amount == -1, true);
        } else {
            checkBool(counter, "parseAmount(\"" + cases[i].text + "\") gives "
                      + std::to_string(cases[i].expected),
                      problem.empty() && amount == cases[i].expected, true);
        }
    }

    long long amount = 0;
    checkText(counter, "message for three decimal digits", parseAmount("12.345", amount),
              "Use at most two digits after the decimal point.");
    checkText(counter, "message for a letter", parseAmount("12a", amount),
              "An amount may contain only digits, commas and one decimal point.");
    checkText(counter, "message for zero", parseAmount("0.00", amount),
              "Amount must be greater than zero.");
    checkText(counter, "message for an empty amount", parseAmount("", amount),
              "Please enter an amount.");
}

void testFormatAmount(TestCounter &counter)
{
    checkText(counter, "formatAmount(120050)", formatAmount(120050), "1,200.50");
    checkText(counter, "formatAmount(0)", formatAmount(0), "0.00");
    checkText(counter, "formatAmount(5)", formatAmount(5), "0.05");
    checkText(counter, "formatAmount(100)", formatAmount(100), "1.00");
    checkText(counter, "formatAmount(99999)", formatAmount(99999), "999.99");
    checkText(counter, "formatAmount(100000)", formatAmount(100000), "1,000.00");
    checkText(counter, "formatAmount(12000000)", formatAmount(12000000), "120,000.00");
    checkText(counter, "formatAmount(maximum)", formatAmount(MAX_AMOUNT_MINOR), "9,999,999,999.99");
    checkText(counter, "formatAmount(-120050)", formatAmount(-120050), "-1,200.50");
    checkText(counter, "formatMoney(\"USD\", 500)", formatMoney("USD", 500), "USD 5.00");

    checkText(counter, "trimSpaces removes outer spaces", trimSpaces("  a b  "), "a b");
    checkText(counter, "trimSpaces of empty text", trimSpaces(""), "");
    checkText(counter, "trimSpaces of spaces only", trimSpaces("   "), "");
}

// Builds one transaction for the filter tests.
Transaction makeTransaction(const std::string &type, long long amount, const std::string &currency,
                            const std::string &category, const std::string &description,
                            const std::string &date)
{
    Transaction transaction;
    transaction.type = type;
    transaction.amountMinor = amount;
    transaction.currencyCode = currency;
    transaction.category = category;
    transaction.description = description;
    transaction.date = date;
    return transaction;
}

void testFilters(TestCounter &counter)
{
    std::vector<Transaction> list;
    list.push_back(makeTransaction(TYPE_EXPENSE, 120050, "PKR", "Food", "Groceries from Ali's shop", "2026-10-28"));
    list.push_back(makeTransaction(TYPE_INCOME, 50000, "USD", "Freelance", "Website for a client", "2026-10-15"));
    list.push_back(makeTransaction(TYPE_EXPENSE, 8000, "PKR", "Transport", "Bus fare", "2026-09-30"));
    list.push_back(makeTransaction(TYPE_INCOME, 12000000, "PKR", "Salary", "October salary", "2026-10-01"));
    list.push_back(makeTransaction(TYPE_EXPENSE, 450, "EUR", "Food", "Coffee", "2026-08-20"));

    TransactionFilter filter;   // every field empty: no filtering
    checkInt(counter, "an empty filter keeps all 5", static_cast<int>(filterTransactions(list, filter).size()), 5);

    filter = TransactionFilter();
    filter.type = TYPE_EXPENSE;
    checkInt(counter, "type expense keeps 3", static_cast<int>(filterTransactions(list, filter).size()), 3);

    filter = TransactionFilter();
    filter.currencyCode = "PKR";
    checkInt(counter, "currency PKR keeps 3", static_cast<int>(filterTransactions(list, filter).size()), 3);

    filter = TransactionFilter();
    filter.category = "Food";
    checkInt(counter, "category Food keeps 2", static_cast<int>(filterTransactions(list, filter).size()), 2);

    filter = TransactionFilter();
    filter.searchText = "GROC";
    checkInt(counter, "search GROC ignores case and keeps 1", static_cast<int>(filterTransactions(list, filter).size()), 1);

    filter = TransactionFilter();
    filter.searchText = "food";
    checkInt(counter, "search food also matches the category: 2", static_cast<int>(filterTransactions(list, filter).size()), 2);

    filter = TransactionFilter();
    filter.fromDate = "2026-10-01";
    checkInt(counter, "from 2026-10-01 includes that day: 3", static_cast<int>(filterTransactions(list, filter).size()), 3);

    filter = TransactionFilter();
    filter.toDate = "2026-09-30";
    checkInt(counter, "up to 2026-09-30 includes that day: 2", static_cast<int>(filterTransactions(list, filter).size()), 2);

    filter = TransactionFilter();
    filter.fromDate = "2026-10-01";
    filter.toDate = "2026-10-31";
    filter.type = TYPE_EXPENSE;
    checkInt(counter, "October expenses only: 1", static_cast<int>(filterTransactions(list, filter).size()), 1);

    filter = TransactionFilter();
    filter.type = TYPE_EXPENSE;
    filter.currencyCode = "PKR";
    filter.searchText = "bus";
    checkInt(counter, "expense + PKR + search bus: 1", static_cast<int>(filterTransactions(list, filter).size()), 1);

    filter = TransactionFilter();
    filter.searchText = "xyz";
    checkInt(counter, "a search with no match keeps 0", static_cast<int>(filterTransactions(list, filter).size()), 0);

    filter = TransactionFilter();
    filter.type = TYPE_INCOME;
    std::vector<Transaction> income = filterTransactions(list, filter);
    checkText(counter, "filtering keeps the original order", income[0].description, "Website for a client");

    std::vector<std::string> categories = distinctCategories(list);
    checkInt(counter, "4 different categories", static_cast<int>(categories.size()), 4);
    checkText(counter, "categories are sorted A to Z",
              categories[0] + "," + categories[1] + "," + categories[2] + "," + categories[3],
              "Food,Freelance,Salary,Transport");

    checkText(counter, "toLowerText", toLowerText("ABC xyz 123"), "abc xyz 123");
    checkBool(counter, "containsIgnoringCase finds SHOP in shop",
              containsIgnoringCase("Ali's shop", "SHOP"), true);
    checkBool(counter, "containsIgnoringCase does not find bank",
              containsIgnoringCase("Ali's shop", "bank"), false);
}

// =====================================================================
//  Milestone 4: totals for the dashboard and budget rules
// =====================================================================

void testCheckedAddition(TestCounter &counter)
{
    long long total = 100;
    checkBool(counter, "addChecked adds two normal amounts", addChecked(total, 250), true);
    checkAmount(counter, "the total is now 350", total, 350);

    long long nearlyFull = LLONG_MAX - 10;
    checkBool(counter, "addChecked refuses an addition that would overflow", addChecked(nearlyFull, 11), false);
    checkAmount(counter, "after a refused addition the total is unchanged", nearlyFull, LLONG_MAX - 10);
    checkBool(counter, "addChecked allows an addition that exactly reaches the limit",
              addChecked(nearlyFull, 10), true);

    long long small = 5;
    checkBool(counter, "addChecked refuses a negative amount", addChecked(small, -1), false);
}

void testMonths(TestCounter &counter)
{
    checkBool(counter, "2026-10 is a valid month", isValidMonth("2026-10"), true);
    checkBool(counter, "2026-13 is not a valid month", isValidMonth("2026-13"), false);
    checkBool(counter, "2026-1 (one digit) is not a valid month", isValidMonth("2026-1"), false);
    checkBool(counter, "1899-12 is before the supported years", isValidMonth("1899-12"), false);
    checkText(counter, "the month of 2026-10-28 is 2026-10", monthOfDate("2026-10-28"), "2026-10");
    checkText(counter, "an impossible date has no month", monthOfDate("2026-02-30"), "");
    checkText(counter, "one month before January 2026 is December 2025", shiftMonth("2026-01", -1), "2025-12");
    checkText(counter, "12 months after 2026-10 is 2027-10", shiftMonth("2026-10", 12), "2027-10");
    checkText(counter, "25 months before 2026-10 is 2024-09", shiftMonth("2026-10", -25), "2024-09");
    checkText(counter, "going past the year 2100 gives no month", shiftMonth("2100-12", 1), "");
}

// Transactions used by several Milestone 4 tests.
std::vector<Transaction> makeDashboardList()
{
    std::vector<Transaction> list;
    list.push_back(makeTransaction(TYPE_INCOME, 12000000, "PKR", "Salary", "October salary", "2026-10-01"));
    list.push_back(makeTransaction(TYPE_EXPENSE, 120050, "PKR", "Food", "Groceries", "2026-10-28"));
    list.push_back(makeTransaction(TYPE_EXPENSE, 30000, "PKR", "food", "Lunch", "2026-10-05"));
    list.push_back(makeTransaction(TYPE_EXPENSE, 8000, "PKR", "Transport", "Bus fare", "2026-10-07"));
    list.push_back(makeTransaction(TYPE_EXPENSE, 500000, "PKR", "Rent", "September rent", "2026-09-30"));
    list.push_back(makeTransaction(TYPE_INCOME, 50000, "USD", "Freelance", "Website", "2026-10-15"));
    list.push_back(makeTransaction(TYPE_EXPENSE, 5000, "USD", "Food", "Dinner", "2026-10-16"));
    list.push_back(makeTransaction(TYPE_EXPENSE, 450, "EUR", "Food", "Coffee", "2026-08-20"));
    list.push_back(makeTransaction(TYPE_EXPENSE, 999, "GBP", "Food", "Not a supported currency", "2026-10-02"));
    return list;
}

void testCurrencySummaries(TestCounter &counter)
{
    std::vector<CurrencySummary> summaries = summarizeByCurrency(makeDashboardList(), "2026-10");

    checkInt(counter, "there is one summary per supported currency", static_cast<int>(summaries.size()), 3);
    checkText(counter, "the summaries are in the order PKR, USD, EUR",
              summaries[0].currencyCode + summaries[1].currencyCode + summaries[2].currencyCode, "PKRUSDEUR");
    checkAmount(counter, "PKR income in October", summaries[0].monthIncomeMinor, 12000000);
    checkAmount(counter, "PKR spending in October leaves out the September rent",
                summaries[0].monthExpenseMinor, 158050);
    checkAmount(counter, "PKR net in October", monthNetOf(summaries[0]), 11841950);
    checkAmount(counter, "the PKR balance over all time includes September",
                balanceOf(summaries[0]), 12000000 - 158050 - 500000);
    checkAmount(counter, "USD is added up on its own, never mixed with PKR",
                summaries[1].monthIncomeMinor + summaries[1].monthExpenseMinor, 55000);
    checkAmount(counter, "EUR has no October spending", summaries[2].monthExpenseMinor, 0);
    checkAmount(counter, "a balance can be negative", balanceOf(summaries[2]), -450);
    checkInt(counter, "the GBP transaction is left out of every total",
             summaries[0].transactionCount + summaries[1].transactionCount + summaries[2].transactionCount, 8);
    checkBool(counter, "normal totals do not overflow",
              summaries[0].overflow || summaries[1].overflow || summaries[2].overflow, false);

    // Amounts this large are refused by validateTransaction, but the
    // totals must still never wrap around to a wrong number.
    std::vector<Transaction> huge;
    huge.push_back(makeTransaction(TYPE_INCOME, LLONG_MAX / 2 + 1, "PKR", "Test", "", "2026-10-01"));
    huge.push_back(makeTransaction(TYPE_INCOME, LLONG_MAX / 2 + 1, "PKR", "Test", "", "2026-10-02"));
    std::vector<CurrencySummary> hugeSummaries = summarizeByCurrency(huge, "2026-10");
    checkBool(counter, "a total that would overflow is marked instead of wrapping around",
              hugeSummaries[0].overflow, true);
    checkAmount(counter, "the overflowing amount is not added", hugeSummaries[0].totalIncomeMinor, LLONG_MAX / 2 + 1);
}

void testMonthlyFlow(TestCounter &counter)
{
    std::vector<Transaction> list = makeDashboardList();
    std::vector<MonthFlow> flows = monthlyFlow(list, "PKR", "2026-10", 6);

    checkInt(counter, "6 months of money flow are returned", static_cast<int>(flows.size()), 6);
    checkText(counter, "the oldest month comes first", flows[0].month, "2026-05");
    checkText(counter, "the chosen month comes last", flows[5].month, "2026-10");
    checkAmount(counter, "October PKR income", flows[5].incomeMinor, 12000000);
    checkAmount(counter, "September PKR spending", flows[4].expenseMinor, 500000);
    checkAmount(counter, "a month without transactions is zero", flows[0].incomeMinor + flows[0].expenseMinor, 0);

    std::vector<MonthFlow> dollars = monthlyFlow(list, "USD", "2026-10", 6);
    checkAmount(counter, "the USD chart shows only USD", dollars[5].incomeMinor, 50000);

    std::vector<MonthFlow> acrossNewYear = monthlyFlow(list, "PKR", "2026-02", 3);
    checkText(counter, "the months cross into the previous year",
              acrossNewYear[0].month + " " + acrossNewYear[1].month, "2025-12 2026-01");
    checkInt(counter, "an invalid month gives no data",
             static_cast<int>(monthlyFlow(list, "PKR", "2026-13", 6).size()), 0);
}

void testSpendingByCategory(TestCounter &counter)
{
    bool overflow = true;
    std::vector<CategoryTotal> totals = spendingByCategory(makeDashboardList(), "PKR", "2026-10", overflow);

    checkInt(counter, "October PKR spending has 2 categories", static_cast<int>(totals.size()), 2);
    checkText(counter, "the largest category comes first", totals[0].category, "Food");
    checkAmount(counter, "Food and food are counted as one category", totals[0].amountMinor, 150050);
    checkAmount(counter, "Transport is second", totals[1].amountMinor, 8000);
    checkBool(counter, "the overflow flag is cleared", overflow, false);
}

void testChartHelpers(TestCounter &counter)
{
    checkAmount(counter, "a largest value of 120,000.00 gives a scale up to 120,000.00",
                chartScaleMaximum(12000000), 12000000);
    checkAmount(counter, "87.00 gives a scale up to 100.00", chartScaleMaximum(8700), 10000);
    checkAmount(counter, "1,234.56 gives a scale up to 1,600.00 (steps of 400)", chartScaleMaximum(123456), 160000);
    checkAmount(counter, "no money at all gives the smallest scale, 4.00", chartScaleMaximum(0), 400);

    checkText(counter, "120,000.00 is written 120k", formatCompactAmount(12000000), "120k");
    checkText(counter, "1,250.00 is written 1.2k", formatCompactAmount(125000), "1.2k");
    checkText(counter, "1,000.00 is written 1k", formatCompactAmount(100000), "1k");
    checkText(counter, "45,000,000.00 is written 45M", formatCompactAmount(4500000000LL), "45M");
    checkText(counter, "850.00 is written 850", formatCompactAmount(85000), "850");
    checkText(counter, "a negative amount keeps its sign", formatCompactAmount(-250000), "-2.5k");
}

void testBudgetRules(TestCounter &counter)
{
    // The status at the edge of every range.
    checkBool(counter, "7,999 of 10,000 (79.99%) is on track",
              budgetStatusFor(7999, 10000) == BUDGET_ON_TRACK, true);
    checkBool(counter, "8,000 of 10,000 (exactly 80%) is a warning",
              budgetStatusFor(8000, 10000) == BUDGET_WARNING, true);
    checkBool(counter, "9,999 of 10,000 is still a warning", budgetStatusFor(9999, 10000) == BUDGET_WARNING, true);
    checkBool(counter, "10,000 of 10,000 is limit reached", budgetStatusFor(10000, 10000) == BUDGET_REACHED, true);
    checkBool(counter, "10,001 of 10,000 is over budget", budgetStatusFor(10001, 10000) == BUDGET_OVER, true);
    checkBool(counter, "the 80% rule is exact without rounding: 4 of 5 is a warning",
              budgetStatusFor(4, 5) == BUDGET_WARNING, true);
    checkBool(counter, "and 3 of 4 (75%) is on track", budgetStatusFor(3, 4) == BUDGET_ON_TRACK, true);
    checkBool(counter, "nothing spent is on track", budgetStatusFor(0, 10000) == BUDGET_ON_TRACK, true);

    checkInt(counter, "79.99% is shown as 79%", percentUsed(7999, 10000), 79);
    checkInt(counter, "exactly 80% is shown as 80%", percentUsed(8000, 10000), 80);
    checkInt(counter, "250% is shown as 250%", percentUsed(25000, 10000), 250);
    checkInt(counter, "an enormous amount is capped at 999% instead of overflowing",
             percentUsed(LLONG_MAX, 10000), 999);
    checkInt(counter, "nothing spent is 0%", percentUsed(0, 10000), 0);

    checkText(counter, "status words: on track", budgetStatusName(BUDGET_ON_TRACK), "On track");
    checkText(counter, "status words: warning", budgetStatusName(BUDGET_WARNING), "Nearing limit");
    checkText(counter, "status words: reached", budgetStatusName(BUDGET_REACHED), "Limit reached");
    checkText(counter, "status words: over", budgetStatusName(BUDGET_OVER), "Over budget");

    Budget budget;
    budget.category = "Food";
    budget.currencyCode = "PKR";
    budget.limitMinor = 200000;   // PKR 2,000.00
    checkText(counter, "a complete budget is valid", validateBudget(budget), "");
    Budget broken = budget;
    broken.category = "  ";
    checkText(counter, "a budget needs a category", validateBudget(broken), "Please choose a category.");
    broken = budget;
    broken.currencyCode = "GBP";
    checkText(counter, "a budget needs a supported currency", validateBudget(broken), "Please choose PKR, USD or EUR.");
    broken = budget;
    broken.limitMinor = 0;
    checkText(counter, "a limit of 0 is refused", validateBudget(broken), "The monthly limit must be greater than zero.");
    broken.limitMinor = MAX_AMOUNT_MINOR + 1;
    checkText(counter, "a limit above the maximum is refused", validateBudget(broken), "The monthly limit is too large.");

    // Only October PKR expenses of Food (in any case) count: 1,200.50 + 300.00.
    std::vector<Transaction> list = makeDashboardList();
    BudgetProgress progress = budgetProgressFor(budget, list, "2026-10");
    checkAmount(counter, "only Food expenses in PKR in October are counted", progress.spentMinor, 150050);
    checkAmount(counter, "the remaining amount is the limit minus the spending", progress.remainingMinor, 49950);
    checkInt(counter, "75% of the budget is used", progress.percentUsed, 75);
    checkBool(counter, "75% is on track", progress.status == BUDGET_ON_TRACK, true);

    budget.limitMinor = 150050;
    progress = budgetProgressFor(budget, list, "2026-10");
    checkBool(counter, "spending exactly the limit is limit reached, with 0 left",
              progress.status == BUDGET_REACHED && progress.remainingMinor == 0, true);

    budget.limitMinor = 100000;
    progress = budgetProgressFor(budget, list, "2026-10");
    checkBool(counter, "spending more than the limit is over budget", progress.status == BUDGET_OVER, true);
    checkAmount(counter, "the remaining amount is negative when over budget", progress.remainingMinor, -50050);

    progress = budgetProgressFor(budget, list, "2026-09");
    checkAmount(counter, "a new month starts again from zero", progress.spentMinor, 0);

    std::vector<Budget> budgets;
    budgets.push_back(budget);
    budget.currencyCode = "USD";
    budgets.push_back(budget);
    std::vector<BudgetProgress> all = allBudgetProgress(budgets, list, "2026-10");
    checkBool(counter, "allBudgetProgress keeps the order: PKR first, then USD (50.00 spent)",
              all.size() == 2 && all[0].spentMinor == 150050 && all[1].spentMinor == 5000, true);
}

// =====================================================================
//  Milestone 4, Phase 4B: password policy and account rules
// =====================================================================

void testPasswordPolicy(TestCounter &counter)
{
    checkInt(counter, "characterCount counts an Urdu letter as one character", characterCount("پ"), 1);
    checkInt(counter, "characterCount of a mixed password",
             characterCount("Passپاس 2026"), 12);

    checkBool(counter, "Password123 is a common password (case ignored)", isCommonPassword("Password123"), true);
    checkBool(counter, "Correct Horse 9! is not on the common list", isCommonPassword("Correct Horse 9!"), false);

    checkText(counter, "9 characters is very weak", passwordStrengthName(passwordStrength("abcdefg12")), "Very weak");
    checkText(counter, "a common password is very weak", passwordStrengthName(passwordStrength("qwertyuiop")),
              "Very weak");
    checkText(counter, "one character repeated is very weak", passwordStrengthName(passwordStrength("aaaaaaaaaaaa")),
              "Very weak");
    checkText(counter, "12 small letters only is weak", passwordStrengthName(passwordStrength("correcthorse")), "Weak");
    checkText(counter, "letters and a digit, 13 characters, is fair",
              passwordStrengthName(passwordStrength("correcthorse7")), "Fair");
    checkText(counter, "3 kinds of characters, 11 long, is strong",
              passwordStrengthName(passwordStrength("Sunflower22")), "Strong");
    checkText(counter, "4 kinds and 16 characters is very strong",
              passwordStrengthName(passwordStrength("Correct Horse 9!")), "Very strong");

    checkText(counter, "the tip for a short password", passwordTip("short1"), "Use at least 10 characters.");
    checkText(counter, "the tip for letters only", passwordTip("correcthorse"), "Mix letters with numbers or symbols.");
    checkText(counter, "a very strong password needs no tip", passwordTip("Correct Horse 9!"), "");

    checkText(counter, "a new password needs 10 characters",
              validateNewPassword("Abc12345!", "ali@example.com", "Ali Khan"), "Use at least 10 characters.");
    checkText(counter, "a common password is refused",
              validateNewPassword("Password123", "ali@example.com", "Ali Khan"),
              "This password is too common. Please choose another one.");
    checkText(counter, "a password containing the email name is refused",
              validateNewPassword("saeedkhan2026!", "saeedkhan@example.com", "Ali"),
              "Do not use your name or email address in the password.");
    checkText(counter, "a password containing a word of the name is refused",
              validateNewPassword("Muhammad#2026x", "ali@example.com", "Muhammad Saeed"),
              "Do not use your name or email address in the password.");
    checkText(counter, "a weak password is refused",
              validateNewPassword("correcthorse", "ali@example.com", "Ali Khan"),
              "This password is too easy to guess. Make it longer, or mix letters, numbers and symbols.");
    checkText(counter, "a fair password is accepted",
              validateNewPassword("correcthorse7", "ali@example.com", "Ali Khan"), "");
    checkText(counter, "10 spaces are refused",
              validateNewPassword("          ", "ali@example.com", "Ali Khan"), "A password cannot be only spaces.");
}

void testAccountRules(TestCounter &counter)
{
    checkText(counter, "an email is stored trimmed and in small letters", normalizeEmail("  Ali@Mail.PK "), "ali@mail.pk");
    checkText(counter, "a normal email is valid", validateEmail("muhammad.saeed+finora@student.muet.edu.pk"), "");
    const std::string invalid = "Please enter a valid email address, for example name@example.com.";
    checkText(counter, "an email without @ is refused", validateEmail("ali.example.com"), invalid);
    checkText(counter, "an email with two @ is refused", validateEmail("ali@@example.com"), invalid);
    checkText(counter, "an email without a dot in the domain is refused", validateEmail("ali@localhost"), invalid);
    checkText(counter, "an email with a space is refused", validateEmail("ali khan@example.com"), invalid);
    checkText(counter, "an empty email asks for one", validateEmail("   "), "Please enter your email address.");

    checkText(counter, "an empty name is refused", validateDisplayName("  "), "Please enter your name.");
    checkText(counter, "an Urdu name is accepted", validateDisplayName("محمد"), "");
    checkText(counter, "the first name of a full name", firstNameOf("Muhammad Saeed"), "Muhammad");

    checkText(counter, "a recovery code is shown in three groups", formatRecoveryCode("7KQ4M2TX9RHC"), "7KQ4-M2TX-9RHC");
    checkText(counter, "a typed code ignores spaces, dashes and small letters",
              normalizeRecoveryCode(" 7kq4 m2tx-9rhc "), "7KQ4M2TX9RHC");
    checkText(counter, "a code with the letter O is not a recovery code", normalizeRecoveryCode("7KQ4-M2TX-9RHO"), "");
    checkText(counter, "a code that is too short is refused", normalizeRecoveryCode("7KQ4-M2TX"), "");
    checkInt(counter, "the recovery alphabet has 32 characters", static_cast<int>(RECOVERY_CODE_ALPHABET.length()), 32);

    checkInt(counter, "no wait after 2 wrong passwords", signInDelaySeconds(2), 0);
    checkInt(counter, "30 seconds after 3", signInDelaySeconds(3), 30);
    checkInt(counter, "60 seconds after 4", signInDelaySeconds(4), 60);
    checkInt(counter, "480 seconds after 7", signInDelaySeconds(7), 480);
    checkInt(counter, "never more than 15 minutes", signInDelaySeconds(50), 900);

    Session nobody;
    checkBool(counter, "a new Session is signed out, with user id 0", !nobody.isSignedIn() && nobody.userId() == 0, true);
}

// ---- Milestone 4, Phase 4C: plans, entitlements and the lifecycle ----

void testPlansAndEntitlements(TestCounter &counter)
{
    checkInt(counter, "there are 4 plans", PLAN_COUNT, 4);
    checkText(counter, "the code of Free is \"free\"", planCode(PLAN_FREE), "free");
    checkText(counter, "the code of Business is \"business\"", planCode(PLAN_BUSINESS), "business");
    PlanId found = PLAN_FREE;
    checkBool(counter, "the code \"pro\" finds Finora Pro", planFromCode("pro", found) && found == PLAN_PRO, true);
    checkBool(counter, "an unknown code is refused", planFromCode("gold", found), false);
    checkBool(counter, "codes are exact: \"PRO\" is refused", planFromCode("PRO", found), false);
    checkText(counter, "the name of Pro", planName(PLAN_PRO), "Finora Pro");

    checkAmount(counter, "Free costs 0 cents", PLANS[PLAN_FREE].priceMinor, 0);
    checkAmount(counter, "Plus costs 599 cents a month", PLANS[PLAN_PLUS].priceMinor, 599);
    checkAmount(counter, "Pro costs 1299 cents a month", PLANS[PLAN_PRO].priceMinor, 1299);
    checkAmount(counter, "Business costs 2499 cents a month", PLANS[PLAN_BUSINESS].priceMinor, 2499);
    checkText(counter, "the price text of Free", formatPlanPrice(PLAN_FREE), "$0");
    checkText(counter, "the price text of Pro", formatPlanPrice(PLAN_PRO), "$12.99");
    bool rising = true;
    for (int i = 1; i < PLAN_COUNT; i++) {
        if (PLANS[i].priceMinor <= PLANS[i - 1].priceMinor) {
            rising = false;
        }
    }
    checkBool(counter, "every larger plan costs more than the one before", rising, true);

    checkInt(counter, "Free allows 3 active budgets", activeBudgetLimit(PLAN_FREE), 3);
    checkInt(counter, "Plus has no budget limit", activeBudgetLimit(PLAN_PLUS), UNLIMITED_BUDGETS);
    checkInt(counter, "Business has no budget limit", activeBudgetLimit(PLAN_BUSINESS), UNLIMITED_BUDGETS);
    checkBool(counter, "Free includes transactions", planIncludes(PLAN_FREE, FEATURE_TRANSACTIONS), true);
    checkBool(counter, "Free does not include CSV exports", planIncludes(PLAN_FREE, FEATURE_CSV_EXPORT), false);
    checkBool(counter, "Plus includes CSV exports", planIncludes(PLAN_PLUS, FEATURE_CSV_EXPORT), true);
    checkBool(counter, "Plus does not include PDF reports", planIncludes(PLAN_PLUS, FEATURE_PDF_REPORTS), false);
    checkBool(counter, "Pro includes PDF reports", planIncludes(PLAN_PRO, FEATURE_PDF_REPORTS), true);
    checkBool(counter, "Pro does not include the audit history", planIncludes(PLAN_PRO, FEATURE_AUDIT_HISTORY), false);
    checkBool(counter, "Business includes the audit history", planIncludes(PLAN_BUSINESS, FEATURE_AUDIT_HISTORY), true);

    // Every plan includes everything of every smaller plan, and every
    // plan brings exactly 7 features of its own.
    bool nested = true;
    int ownFeatures[PLAN_COUNT] = {0, 0, 0, 0};
    for (int f = 0; f < FEATURE_COUNT; f++) {
        ownFeatures[FEATURES[f].lowestPlan]++;
        for (int p = 1; p < PLAN_COUNT; p++) {
            Feature feature = static_cast<Feature>(f);
            if (planIncludes(static_cast<PlanId>(p - 1), feature) && !planIncludes(static_cast<PlanId>(p), feature)) {
                nested = false;
            }
        }
    }
    checkBool(counter, "every plan includes all features of the smaller plans", nested, true);
    checkBool(counter, "each plan brings 7 features of its own (28 in total)",
              ownFeatures[0] == 7 && ownFeatures[1] == 7 && ownFeatures[2] == 7 && ownFeatures[3] == 7, true);

    // Honest labels: nothing unbuilt is presented as working.
    bool freeAllBuilt = true;
    for (int f = 0; f < FEATURE_COUNT; f++) {
        if (FEATURES[f].lowestPlan == PLAN_FREE && FEATURES[f].state != FEATURE_AVAILABLE) {
            freeAllBuilt = false;
        }
    }
    checkBool(counter, "every Free feature is built and available", freeAllBuilt, true);
    checkBool(counter, "unlimited budgets are available on Plus", featureAvailable(FEATURE_UNLIMITED_BUDGETS), true);
    checkBool(counter, "savings goals are marked as planned, not available", featureAvailable(FEATURE_SAVINGS_GOALS), false);
    checkBool(counter, "team members, shared workspaces, roles and team reports need Finora Online",
              FEATURES[FEATURE_TEAM_MEMBERS].state == FEATURE_NEEDS_ONLINE
                  && FEATURES[FEATURE_SHARED_WORKSPACES].state == FEATURE_NEEDS_ONLINE
                  && FEATURES[FEATURE_ROLES].state == FEATURE_NEEDS_ONLINE
                  && FEATURES[FEATURE_TEAM_REPORTS].state == FEATURE_NEEDS_ONLINE, true);

    checkText(counter, "the message for a feature of a larger plan",
              lockedFeatureMessage(FEATURE_UNLIMITED_BUDGETS, PLAN_FREE),
              "\"Unlimited active budgets\" is part of Finora Plus. Your plan: Finora Free.");
    checkText(counter, "the message for a planned feature",
              lockedFeatureMessage(FEATURE_SAVINGS_GOALS, PLAN_PLUS),
              "\"Savings goals\" is planned and not available in this version of Finora yet.");
    checkText(counter, "the message for a team feature",
              lockedFeatureMessage(FEATURE_TEAM_MEMBERS, PLAN_BUSINESS),
              "\"Team members\" will need Finora Online, which is planned. "
              "It is not available in this desktop version.");
    checkText(counter, "the message when the Free budget limit is reached", budgetLimitMessage(PLAN_FREE),
              "Finora Free includes 3 active budgets. Upgrade to Finora Plus for unlimited budgets, "
              "or delete a budget to make room.");
}

// A subscription for the lifecycle tests: renews on 30 November 2026 at
// 10:00 UTC, and its access ends 7 days later without payment.
Subscription activeSubscription(PlanId plan)
{
    Subscription subscription;
    subscription.id = 1;
    subscription.plan = plan;
    subscription.status = STATUS_ACTIVE;
    subscription.startedAt = "2026-10-30T10:00:00Z";
    subscription.renewsAt = "2026-11-30T10:00:00Z";
    subscription.expiresAt = "2026-12-07T10:00:00Z";
    return subscription;
}

void testSubscriptionLifecycle(TestCounter &counter)
{
    SubscriptionStatus status = STATUS_FREE;
    checkBool(counter, "the status code \"past_due\" finds Past due",
              statusFromCode(statusCode(STATUS_PAST_DUE), status) && status == STATUS_PAST_DUE, true);
    checkBool(counter, "an unknown status code is refused", statusFromCode("paused", status), false);
    checkText(counter, "status names are words for the screen", statusName(STATUS_PAST_DUE), "Past due");
    checkBool(counter, "UTC times compare as text: 09:00 is before 10:00",
              std::string("2026-11-30T09:00:00Z") < std::string("2026-11-30T10:00:00Z"), true);

    Subscription free;
    checkBool(counter, "Free stays Free at any time", subscriptionAt(free, "2099-01-01T00:00:00Z").status == STATUS_FREE,
              true);

    Subscription plus = activeSubscription(PLAN_PLUS);
    checkBool(counter, "one second before the renewal date, Plus is still active",
              subscriptionAt(plus, "2026-11-30T09:59:59Z").status == STATUS_ACTIVE, true);
    checkBool(counter, "at the renewal date, an unpaid renewal makes it past due",
              subscriptionAt(plus, "2026-11-30T10:00:00Z").status == STATUS_PAST_DUE, true);
    checkBool(counter, "during the 7 days of grace, Plus features stay",
              planAt(plus, "2026-12-07T09:59:59Z") == PLAN_PLUS, true);
    Subscription ended = subscriptionAt(plus, "2026-12-07T10:00:00Z");
    checkBool(counter, "when the grace ends unpaid, it expires and Free applies",
              ended.status == STATUS_EXPIRED && ended.plan == PLAN_FREE && ended.renewsAt.empty(), true);
    checkBool(counter, "months later, both deadlines are applied at once",
              subscriptionAt(plus, "2027-03-01T00:00:00Z").status == STATUS_EXPIRED, true);

    Subscription downgrading = activeSubscription(PLAN_PRO);
    downgrading.hasScheduledPlan = true;
    downgrading.scheduledPlan = PLAN_PLUS;
    checkBool(counter, "before the renewal date, Pro is still used", planAt(downgrading, "2026-11-29T00:00:00Z") == PLAN_PRO,
              true);
    Subscription switched = subscriptionAt(downgrading, "2026-11-30T10:00:00Z");
    checkBool(counter, "a smaller plan chosen earlier starts at the renewal date",
              switched.plan == PLAN_PLUS && switched.status == STATUS_PAST_DUE && !switched.hasScheduledPlan, true);

    Subscription trial;
    trial.plan = PLAN_PRO;
    trial.status = STATUS_TRIAL;
    trial.expiresAt = "2026-11-14T08:00:00Z";
    trial.trialUsed = true;
    checkBool(counter, "during the trial, Pro features may be used", planAt(trial, "2026-11-14T07:59:59Z") == PLAN_PRO, true);
    checkBool(counter, "when the trial ends, it expires and Free applies",
              subscriptionAt(trial, "2026-11-14T08:00:00Z").status == STATUS_EXPIRED
                  && planAt(trial, "2026-11-14T08:00:00Z") == PLAN_FREE, true);
    checkBool(counter, "an expired trial still remembers that the trial was used",
              subscriptionAt(trial, "2026-12-01T00:00:00Z").trialUsed, true);

    Subscription cancelled = activeSubscription(PLAN_PRO);
    cancelled.status = STATUS_CANCELLED;
    cancelled.renewsAt = "";
    cancelled.expiresAt = "2026-11-30T10:00:00Z";
    checkBool(counter, "a cancelled plan keeps its features until the paid period ends",
              planAt(cancelled, "2026-11-30T09:59:59Z") == PLAN_PRO, true);
    checkBool(counter, "then it expires, without a grace period", planAt(cancelled, "2026-11-30T10:00:00Z") == PLAN_FREE,
              true);
}

void testPlanChanges(TestCounter &counter)
{
    Subscription free;
    checkBool(counter, "Free to Plus: a new subscription, paid now", planChangeFor(free, PLAN_PLUS) == CHANGE_SUBSCRIBE,
              true);
    checkBool(counter, "Free to Free: nothing to do", planChangeFor(free, PLAN_FREE) == CHANGE_NONE, true);

    Subscription plus = activeSubscription(PLAN_PLUS);
    checkBool(counter, "Plus to Pro: an upgrade, paid now", planChangeFor(plus, PLAN_PRO) == CHANGE_UPGRADE, true);

    Subscription pro = activeSubscription(PLAN_PRO);
    checkBool(counter, "Pro to Plus: waits for the renewal date, nothing paid now",
              planChangeFor(pro, PLAN_PLUS) == CHANGE_DOWNGRADE, true);
    checkBool(counter, "Pro to Free: cancels at the end of the period", planChangeFor(pro, PLAN_FREE) == CHANGE_CANCEL,
              true);
    checkBool(counter, "Pro to Pro: nothing to do", planChangeFor(pro, PLAN_PRO) == CHANGE_NONE, true);
    pro.hasScheduledPlan = true;
    pro.scheduledPlan = PLAN_PLUS;
    checkBool(counter, "with Plus already arranged, choosing Plus again changes nothing",
              planChangeFor(pro, PLAN_PLUS) == CHANGE_NONE, true);
    checkBool(counter, "with Plus arranged, choosing Pro keeps Pro", planChangeFor(pro, PLAN_PRO) == CHANGE_RESUME, true);

    Subscription cancelled = activeSubscription(PLAN_PRO);
    cancelled.status = STATUS_CANCELLED;
    checkBool(counter, "a cancelled Pro can be resumed without paying again",
              planChangeFor(cancelled, PLAN_PRO) == CHANGE_RESUME, true);
    checkBool(counter, "a cancelled plan already ends: choosing Free changes nothing",
              planChangeFor(cancelled, PLAN_FREE) == CHANGE_NONE, true);

    Subscription pastDue = activeSubscription(PLAN_PLUS);
    pastDue.status = STATUS_PAST_DUE;
    checkBool(counter, "past due on Plus: paying for Plus is a renewal", planChangeFor(pastDue, PLAN_PLUS) == CHANGE_RENEW,
              true);
    checkBool(counter, "past due: choosing Free stops at once", planChangeFor(pastDue, PLAN_FREE) == CHANGE_CANCEL, true);

    Subscription trial;
    trial.plan = PLAN_PRO;
    trial.status = STATUS_TRIAL;
    checkBool(counter, "during the trial, paying for Pro is a new subscription",
              planChangeFor(trial, PLAN_PRO) == CHANGE_SUBSCRIBE, true);
    checkBool(counter, "during the trial, choosing Free ends the trial at once",
              planChangeFor(trial, PLAN_FREE) == CHANGE_CANCEL, true);

    checkBool(counter, "only subscribe, upgrade and renew need a payment",
              changeNeedsPayment(CHANGE_SUBSCRIBE) && changeNeedsPayment(CHANGE_UPGRADE) && changeNeedsPayment(CHANGE_RENEW)
                  && !changeNeedsPayment(CHANGE_DOWNGRADE) && !changeNeedsPayment(CHANGE_CANCEL)
                  && !changeNeedsPayment(CHANGE_RESUME) && !changeNeedsPayment(CHANGE_NONE), true);
    checkText(counter, "the payment reason of an upgrade", paymentReasonFor(CHANGE_UPGRADE), "upgrade");
    checkText(counter, "the payment reason of a renewal", paymentReasonFor(CHANGE_RENEW), "renewal");
    checkText(counter, "the payment reason of a new subscription", paymentReasonFor(CHANGE_SUBSCRIBE), "new");

    checkBool(counter, "the Pro trial may start on Free", canStartTrial(free), true);
    Subscription afterTrial;
    afterTrial.status = STATUS_EXPIRED;
    afterTrial.trialUsed = true;
    checkBool(counter, "the trial cannot start a second time", canStartTrial(afterTrial), false);
    checkBool(counter, "the trial cannot start while a plan is paid", canStartTrial(plus), false);
}

void testPausedBudgets(TestCounter &counter)
{
    // Five budgets in the order the page shows them (by category), not
    // in the order they were created (by id).
    int ids[5] = {7, 2, 9, 4, 1};
    std::vector<Budget> budgets;
    for (int i = 0; i < 5; i++) {
        Budget budget;
        budget.id = ids[i];
        budgets.push_back(budget);
    }
    markPausedBudgets(budgets, 3);
    checkBool(counter, "on Free, the 3 oldest budgets (ids 1, 2 and 4) stay active",
              !budgets[4].paused && !budgets[1].paused && !budgets[3].paused, true);
    checkBool(counter, "the 2 newer budgets (ids 7 and 9) are paused", budgets[0].paused && budgets[2].paused, true);
    checkBool(counter, "2 budgets are paused and all 5 are kept",
              countPausedBudgets(budgets) == 2 && budgets.size() == 5, true);

    markPausedBudgets(budgets, UNLIMITED_BUDGETS);
    checkInt(counter, "with no limit, no budget is paused", countPausedBudgets(budgets), 0);

    budgets.erase(budgets.begin() + 1);   // delete the budget with id 2
    markPausedBudgets(budgets, 3);
    checkBool(counter, "after deleting an active budget, the next oldest (id 7) becomes active",
              !budgets[0].paused && budgets[1].paused, true);

    std::vector<Budget> exactlyThree(3);
    for (int i = 0; i < 3; i++) {
        exactlyThree[i].id = i + 1;
    }
    markPausedBudgets(exactlyThree, 3);
    checkInt(counter, "exactly 3 budgets on Free: none is paused", countPausedBudgets(exactlyThree), 0);
}

void testCsvExport(TestCounter &counter)
{
    checkBool(counter, "CSV exports, PDF reports and the audit history are built in this version",
              featureAvailable(FEATURE_CSV_EXPORT) && featureAvailable(FEATURE_PDF_REPORTS)
                  && featureAvailable(FEATURE_AUDIT_HISTORY), true);

    checkText(counter, "a plain CSV field stays as it is", csvField("Food"), "Food");
    checkText(counter, "a field with a comma is put in quotes", csvField("Rent, October"), "\"Rent, October\"");
    checkText(counter, "quotes inside a field are doubled", csvField("Ali \"Bhai\""), "\"Ali \"\"Bhai\"\"\"");
    checkText(counter, "a field with a line break is put in quotes", csvField("two\nlines"), "\"two\nlines\"");
    checkText(counter, "a formula is made harmless: =SUM(A1:A9) gets a ' in front", csvField("=SUM(A1:A9)"),
              "'=SUM(A1:A9)");
    checkText(counter, "+, - and @ at the start are made harmless too",
              csvField("+92 300") + csvField("-5") + csvField("@home"), "'+92 300'-5'@home");
    checkText(counter, "a formula with commas gets both the ' and the quotes",
              csvField("=HYPERLINK(\"x\",\"y\")"), "\"'=HYPERLINK(\"\"x\"\",\"\"y\"\")\"");
    checkText(counter, "an empty field stays empty", csvField(""), "");
    checkText(counter, "a formula after leading spaces is made harmless too", csvField("  =SUM(A1:A9)"),
              "'  =SUM(A1:A9)");
    checkText(counter, "a field starting with a tab is made harmless", csvField("\t@cmd"), "'\t@cmd");
    checkText(counter, "a field starting with a line feed gets the ' and quotes", csvField("\n=1+1"),
              "\"'\n=1+1\"");
    checkText(counter, "a field starting with a carriage return gets the ' and quotes", csvField("\r+1"),
              "\"'\r+1\"");
    checkText(counter, "= + - @ later in the text are harmless and stay as they are",
              csvField("Rent - October") + "|" + csvField("ali@example.com"), "Rent - October|ali@example.com");

    checkText(counter, "amounts are plain numbers: 120050 gives 1200.50", plainAmount(120050), "1200.50");
    checkText(counter, "5 minor units give 0.05", plainAmount(5), "0.05");
    checkText(counter, "the largest amount has no thousands separators", plainAmount(MAX_AMOUNT_MINOR), "9999999999.99");

    std::vector<Transaction> list(1);
    list[0].date = "2026-10-28";
    list[0].type = TYPE_EXPENSE;
    list[0].category = "Food";
    list[0].description = "Groceries, Ali's shop";
    list[0].currencyCode = "PKR";
    list[0].amountMinor = 120050;
    checkText(counter, "a CSV file is a header line and one line per transaction, each ending with \\r\\n",
              transactionsCsv(list),
              "Date,Type,Category,Description,Currency,Amount\r\n"
              "2026-10-28,expense,Food,\"Groceries, Ali's shop\",PKR,1200.50\r\n");
    checkText(counter, "without transactions the file has only the header", transactionsCsv(std::vector<Transaction>()),
              "Date,Type,Category,Description,Currency,Amount\r\n");
}

int main()
{
    TestCounter counter;

    std::cout << "Finora core tests\n";
    std::cout << "-----------------\n";

    testGreetingForHour(counter);
    testLeapYearsAndMonths(counter);
    testIsValidIsoDate(counter);
    testCurrenciesAndBlankText(counter);
    testValidateTransaction(counter);
    testParseAmount(counter);
    testFormatAmount(counter);
    testFilters(counter);

    std::cout << "---- Milestone 4: totals and budgets ----\n";
    testCheckedAddition(counter);
    testMonths(counter);
    testCurrencySummaries(counter);
    testMonthlyFlow(counter);
    testSpendingByCategory(counter);
    testChartHelpers(counter);
    testBudgetRules(counter);

    std::cout << "---- Milestone 4, Phase 4B: passwords and accounts ----\n";
    testPasswordPolicy(counter);
    testAccountRules(counter);

    std::cout << "---- Milestone 4, Phase 4C: plans and subscriptions ----\n";
    testPlansAndEntitlements(counter);
    testSubscriptionLifecycle(counter);
    testPlanChanges(counter);
    testPausedBudgets(counter);
    testCsvExport(counter);

    std::cout << "-----------------\n";
    std::cout << counter.checksRun << " checks, " << counter.checksFailed << " failed\n";

    if (counter.checksFailed > 0) {
        return 1;   // a non-zero exit code tells CTest that tests failed
    }
    return 0;
}
