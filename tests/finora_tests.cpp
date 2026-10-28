// =====================================================================
//  finora_tests.cpp
//  Automated checks for Finora's core logic (plain C++, no Qt).
//
//  Every check prints PASS or FAIL. The program returns 0 only when all
//  checks pass, so CTest (and build.bat) can report the result.
// =====================================================================
#include <iostream>
#include <string>

#include "core/Greeting.h"
#include "core/Transaction.h"

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

    std::cout << "-----------------\n";
    std::cout << counter.checksRun << " checks, " << counter.checksFailed << " failed\n";

    if (counter.checksFailed > 0) {
        return 1;   // a non-zero exit code tells CTest that tests failed
    }
    return 0;
}
