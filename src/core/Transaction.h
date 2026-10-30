// =====================================================================
//  Transaction.h
//  One income or expense record, the currencies Finora supports, and
//  the rules that decide whether a record is valid.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_TRANSACTION_H
#define FINORA_TRANSACTION_H

#include <string>

// ---- Transaction types ----------------------------------------------
const std::string TYPE_INCOME = "income";
const std::string TYPE_EXPENSE = "expense";

// ---- Amounts ---------------------------------------------------------
// Money is stored as a whole number of the smallest unit (paisa or
// cents), never as a double, so adding amounts is always exact.
// Example: PKR 1,200.50 is stored as 120050.
//
// MAX_AMOUNT_MINOR is a validation rule for ONE transaction:
// 999,999,999,999 minor units = 9,999,999,999.99 in the main unit.
// It fits easily in a long long (about 9.2 x 10^18), but adding many
// amounts together could still overflow, so future totals must use
// checked addition instead of relying on this limit.
const long long MAX_AMOUNT_MINOR = 999999999999LL;

// ---- Supported currencies --------------------------------------------
struct CurrencyInfo {
    const char *code;       // ISO 4217 code, for example "PKR"
    const char *name;       // for example "Pakistani Rupee"
    const char *symbol;     // for example "Rs"
    int decimalPlaces;      // digits after the decimal point (2 for all three)
};

const int SUPPORTED_CURRENCY_COUNT = 3;
const CurrencyInfo SUPPORTED_CURRENCIES[SUPPORTED_CURRENCY_COUNT] = {
    {"PKR", "Pakistani Rupee", "Rs", 2},
    {"USD", "US Dollar",       "$",  2},
    {"EUR", "Euro",            "€",  2}
};

// ---- Suggested categories --------------------------------------------
// Shown in the category list of the transaction form. The user may also
// type a category of their own.
const int EXPENSE_CATEGORY_COUNT = 9;
const std::string EXPENSE_CATEGORIES[EXPENSE_CATEGORY_COUNT] = {
    "Food", "Transport", "Bills & Utilities", "Rent", "Shopping",
    "Health", "Education", "Entertainment", "Other"
};

const int INCOME_CATEGORY_COUNT = 5;
const std::string INCOME_CATEGORIES[INCOME_CATEGORY_COUNT] = {
    "Salary", "Freelance", "Business", "Gift", "Other"
};

// ---- One transaction -------------------------------------------------
struct Transaction {
    int id = 0;                  // 0 until the database gives the record its number
    std::string type;            // TYPE_INCOME or TYPE_EXPENSE
    long long amountMinor = 0;   // amount in paisa or cents: 120050 means 1,200.50
    std::string currencyCode;    // "PKR", "USD" or "EUR"
    std::string category;        // for example "Food" or "Salary"
    std::string description;     // optional note, may be empty
    std::string date;            // "YYYY-MM-DD", for example "2026-10-28"
};

// ---- Rules -----------------------------------------------------------

// True if code is exactly one of SUPPORTED_CURRENCIES ("PKR", "USD", "EUR").
bool isSupportedCurrency(const std::string &code);

// True if text is empty or contains only spaces and tabs.
bool isBlank(const std::string &text);

// True if year is a leap year in the Gregorian calendar.
bool isLeapYear(int year);

// Number of days in a month (1 to 12) of a year. Returns 0 for an invalid month.
int daysInMonth(int year, int month);

// True if text is a real calendar date written as YYYY-MM-DD between
// the years 1900 and 2100. "2024-02-29" is valid; "2026-02-29" is not.
bool isValidIsoDate(const std::string &text);

// Checks every field of a transaction. Returns an empty string if the
// transaction is valid, otherwise a message about the first problem found.
std::string validateTransaction(const Transaction &transaction);

#endif // FINORA_TRANSACTION_H
