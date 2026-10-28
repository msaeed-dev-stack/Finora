// =====================================================================
//  Transaction.cpp
// =====================================================================
#include "Transaction.h"

// Turns `count` digit characters starting at `start` into a number.
// Example: digitsToNumber("2026-10-28", 0, 4) returns 2026.
// Each character is converted with  character - '0',  because the
// digit characters '0' to '9' have consecutive character codes.
int digitsToNumber(const std::string &text, int start, int count)
{
    int number = 0;
    for (int i = start; i < start + count; i++) {
        number = number * 10 + (text[i] - '0');
    }
    return number;
}

bool isSupportedCurrency(const std::string &code)
{
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        if (code == SUPPORTED_CURRENCIES[i].code) {
            return true;
        }
    }
    return false;
}

bool isBlank(const std::string &text)
{
    // length() returns an unsigned size, so it is cast to int once here.
    int length = static_cast<int>(text.length());

    for (int i = 0; i < length; i++) {
        if (text[i] != ' ' && text[i] != '\t') {
            return false;
        }
    }
    return true;
}

bool isLeapYear(int year)
{
    // Every 4th year is a leap year, except century years,
    // unless the century year divides by 400: 2000 was a leap year, 1900 was not.
    if (year % 400 == 0) {
        return true;
    }
    if (year % 100 == 0) {
        return false;
    }
    return year % 4 == 0;
}

int daysInMonth(int year, int month)
{
    switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
        return 31;
    case 4: case 6: case 9: case 11:
        return 30;
    case 2:
        if (isLeapYear(year)) {
            return 29;
        }
        return 28;
    default:
        return 0;   // there is no such month
    }
}

bool isValidIsoDate(const std::string &text)
{
    // The shape must be exactly YYYY-MM-DD: 10 characters, '-' at positions 4 and 7.
    if (text.length() != 10 || text[4] != '-' || text[7] != '-') {
        return false;
    }

    // Every other character must be a digit.
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
    }

    int year = digitsToNumber(text, 0, 4);
    int month = digitsToNumber(text, 5, 2);
    int day = digitsToNumber(text, 8, 2);

    // The year range catches typing mistakes such as 0026 or 20026.
    if (year < 1900 || year > 2100) {
        return false;
    }
    if (month < 1 || month > 12) {
        return false;
    }
    if (day < 1 || day > daysInMonth(year, month)) {
        return false;
    }
    return true;
}

std::string validateTransaction(const Transaction &transaction)
{
    if (transaction.type != TYPE_INCOME && transaction.type != TYPE_EXPENSE) {
        return "Type must be income or expense.";
    }
    if (transaction.amountMinor <= 0) {
        return "Amount must be greater than zero.";
    }
    if (transaction.amountMinor > MAX_AMOUNT_MINOR) {
        return "Amount is too large. The maximum is 9,999,999,999.99.";
    }
    if (!isSupportedCurrency(transaction.currencyCode)) {
        return "Currency must be PKR, USD or EUR.";
    }
    if (isBlank(transaction.category)) {
        return "Category must not be empty.";
    }
    if (!isValidIsoDate(transaction.date)) {
        return "Date must be a real date written as YYYY-MM-DD.";
    }
    return "";   // no problems found
}
