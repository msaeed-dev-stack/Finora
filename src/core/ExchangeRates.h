// =====================================================================
//  ExchangeRates.h
//  Exchange rates that the person enters (Milestone 5), and converting
//  amounts between PKR, USD and EUR with them. Finora never downloads
//  rates: every number here comes from the person.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_EXCHANGERATES_H
#define FINORA_EXCHANGERATES_H

#include <string>

// A rate says how many Pakistani Rupees one unit of a currency is worth:
// "1 USD = 278.5000 PKR". Like money, a rate is a whole number, here with
// 4 decimal places: 278.5000 is stored as 2785000. So no rate is ever a
// double, and converting is exact up to the final rounding.
const long long RATE_SCALE = 10000;               // 4 decimal places
const long long MAX_RATE = 1000000LL * RATE_SCALE; // at most 1,000,000.0000 PKR per unit

struct ExchangeRates {
    long long usdInPkr = 0;   // 1 USD in PKR, times RATE_SCALE; 0 = not entered yet
    long long eurInPkr = 0;   // 1 EUR in PKR, times RATE_SCALE; 0 = not entered yet
};

// Reads a typed rate such as "278.5" or "278.5000". Returns "" and sets
// rateScaled, or a message: empty, not a number, more than 4 decimals,
// zero or below, or larger than MAX_RATE.
std::string parseRate(const std::string &text, long long &rateScaled);

// 2785000 as "278.5000".
std::string formatRate(long long rateScaled);

// The rate of one currency against PKR, times RATE_SCALE: RATE_SCALE for
// PKR itself, the entered rate for USD and EUR, and 0 when it is missing.
long long rateInPkr(const ExchangeRates &rates, const std::string &currencyCode);

// Converts an amount in minor units from one currency to another:
//     result = amount x rate(from) / rate(to)
// The result is rounded to the nearest minor unit (paisa or cent); an
// exact half is rounded away from zero, so 0.5 becomes 1 and -0.5
// becomes -1. Returns false if a rate is missing or if the numbers would
// be too large for a long long (checked multiplication).
bool convertAmount(long long amountMinor, const std::string &fromCurrency, const std::string &toCurrency,
                   const ExchangeRates &rates, long long &resultMinor);

// Adds `amount`, which may be negative (a balance can be), to `total`.
// If the result would not fit in a long long, nothing is added and the
// function returns false. (addChecked() in Totals.h accepts only amounts
// of 0 or more.)
bool addSignedChecked(long long &total, long long amount);

#endif // FINORA_EXCHANGERATES_H
