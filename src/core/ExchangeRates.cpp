// =====================================================================
//  ExchangeRates.cpp
// =====================================================================
#include "ExchangeRates.h"

#include <climits>

#include "Money.h"   // trimSpaces

std::string parseRate(const std::string &text, long long &rateScaled)
{
    rateScaled = 0;
    std::string value = trimSpaces(text);
    if (value.empty()) {
        return "Please enter a rate.";
    }

    long long whole = 0;
    long long fraction = 0;
    int decimals = 0;
    bool seenPoint = false;
    bool seenDigit = false;
    int length = static_cast<int>(value.length());
    for (int i = 0; i < length; i++) {
        char c = value[i];
        if (c == '.') {
            if (seenPoint) {
                return "A rate has only one decimal point.";
            }
            seenPoint = true;
        } else if (c >= '0' && c <= '9') {
            seenDigit = true;
            int digit = c - '0';
            if (seenPoint) {
                decimals++;
                if (decimals > 4) {
                    return "A rate can have at most 4 decimal places.";
                }
                fraction = fraction * 10 + digit;
            } else {
                if (whole > MAX_RATE / RATE_SCALE) {
                    return "The rate is too large.";
                }
                whole = whole * 10 + digit;
            }
        } else {
            return "A rate can contain only digits and one decimal point, for example 278.5.";
        }
    }
    if (!seenDigit) {
        return "Please enter a rate.";
    }

    // Pad the decimals to 4 places: "278.5" means 278.5000.
    for (int i = decimals; i < 4; i++) {
        fraction = fraction * 10;
    }
    long long result = whole * RATE_SCALE + fraction;
    if (result <= 0) {
        return "The rate must be more than zero.";
    }
    if (result > MAX_RATE) {
        return "The rate is too large.";
    }
    rateScaled = result;
    return "";
}

std::string formatRate(long long rateScaled)
{
    long long whole = rateScaled / RATE_SCALE;
    long long fraction = rateScaled % RATE_SCALE;
    std::string fractionText = std::to_string(fraction);
    while (fractionText.length() < 4) {
        fractionText = "0" + fractionText;   // 50 -> "0050"
    }
    return std::to_string(whole) + "." + fractionText;
}

long long rateInPkr(const ExchangeRates &rates, const std::string &currencyCode)
{
    if (currencyCode == "PKR") {
        return RATE_SCALE;   // 1 PKR = 1.0000 PKR
    }
    if (currencyCode == "USD") {
        return rates.usdInPkr;
    }
    if (currencyCode == "EUR") {
        return rates.eurInPkr;
    }
    return 0;
}

bool convertAmount(long long amountMinor, const std::string &fromCurrency, const std::string &toCurrency,
                   const ExchangeRates &rates, long long &resultMinor)
{
    resultMinor = 0;
    long long fromRate = rateInPkr(rates, fromCurrency);
    long long toRate = rateInPkr(rates, toCurrency);
    if (fromRate <= 0 || toRate <= 0) {
        return false;   // a rate has not been entered
    }
    if (fromRate > MAX_RATE || toRate > MAX_RATE) {
        return false;   // never true for rates read with parseRate()
    }

    // Work with the size of the amount and put the sign back at the end.
    bool negative = amountMinor < 0;
    long long size = amountMinor;
    if (negative) {
        size = -size;
    }

    // Checked multiplication: 2 * size * fromRate + toRate, the largest
    // number used below, must fit in a long long. The test divides instead
    // of multiplying, so the test itself can never overflow.
    if (size > ((LLONG_MAX - toRate) / 2) / fromRate) {
        return false;
    }
    long long product = size * fromRate;

    // Rounding half away from zero with whole numbers only:
    // (2 * product + toRate) / (2 * toRate) is product / toRate rounded.
    long long rounded = (2 * product + toRate) / (2 * toRate);

    resultMinor = rounded;
    if (negative) {
        resultMinor = -rounded;
    }
    return true;
}

bool addSignedChecked(long long &total, long long amount)
{
    // The test is written so that it can never overflow itself:
    // total + amount > LLONG_MAX  is the same as  total > LLONG_MAX - amount.
    if (amount > 0 && total > LLONG_MAX - amount) {
        return false;
    }
    if (amount < 0 && total < LLONG_MIN - amount) {
        return false;
    }
    total = total + amount;
    return true;
}
