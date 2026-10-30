// =====================================================================
//  Money.cpp
// =====================================================================
#include "Money.h"

#include "Transaction.h"   // MAX_AMOUNT_MINOR

// True if c is one of the characters '0' to '9'.
bool isDigitCharacter(char c)
{
    return c >= '0' && c <= '9';
}

std::string trimSpaces(const std::string &text)
{
    int start = 0;
    int end = static_cast<int>(text.length()) - 1;

    while (start <= end && (text[start] == ' ' || text[start] == '\t')) {
        start++;
    }
    while (end >= start && (text[end] == ' ' || text[end] == '\t')) {
        end--;
    }
    return text.substr(start, end - start + 1);
}

std::string parseAmount(const std::string &text, long long &amountMinor)
{
    std::string cleaned = trimSpaces(text);
    if (cleaned.empty()) {
        return "Please enter an amount.";
    }
    if (cleaned[0] == '-') {
        return "Amount must be greater than zero.";
    }

    long long wholeUnits = 0;   // the number before the decimal point
    int fraction = 0;           // the digits after the decimal point, as a number
    int fractionDigits = 0;     // how many digits came after the point
    bool seenPoint = false;
    bool seenDigit = false;
    int length = static_cast<int>(cleaned.length());

    // Look at every character once, from left to right.
    for (int i = 0; i < length; i++) {
        char c = cleaned[i];

        if (isDigitCharacter(c)) {
            seenDigit = true;
            int digit = c - '0';
            if (seenPoint) {
                fractionDigits++;
                if (fractionDigits > 2) {
                    return "Use at most two digits after the decimal point.";
                }
                fraction = fraction * 10 + digit;
            } else {
                wholeUnits = wholeUnits * 10 + digit;
                // Checked after every digit, so the number can never overflow.
                if (wholeUnits > MAX_AMOUNT_MINOR / 100) {
                    return "Amount is too large. The maximum is 9,999,999,999.99.";
                }
            }
        } else if (c == '.') {
            if (seenPoint) {
                return "An amount can have only one decimal point.";
            }
            seenPoint = true;
        } else if (c == ',') {
            // A comma may only stand between two digits of the whole number.
            bool digitBefore = i > 0 && isDigitCharacter(cleaned[i - 1]);
            bool digitAfter = i + 1 < length && isDigitCharacter(cleaned[i + 1]);
            if (seenPoint || !digitBefore || !digitAfter) {
                return "Commas may only separate digits, as in 1,200.50.";
            }
        } else {
            return "An amount may contain only digits, commas and one decimal point.";
        }
    }

    if (!seenDigit) {
        return "Please enter an amount.";
    }
    if (fractionDigits == 1) {
        fraction = fraction * 10;   // "0.5" means 50 paisa, not 5
    }

    long long result = wholeUnits * 100 + fraction;
    if (result <= 0) {
        return "Amount must be greater than zero.";
    }
    if (result > MAX_AMOUNT_MINOR) {
        return "Amount is too large. The maximum is 9,999,999,999.99.";
    }

    amountMinor = result;
    return "";
}

std::string formatAmount(long long amountMinor)
{
    bool negative = amountMinor < 0;
    long long value = amountMinor;
    if (negative) {
        value = -value;   // amounts stay far below the long long limit
    }

    long long whole = value / 100;
    int cents = static_cast<int>(value % 100);

    // Walk from the last digit to the first and put a comma after every
    // third digit: "1200" becomes "1,200".
    std::string digits = std::to_string(whole);
    std::string grouped;
    int count = 0;
    for (int i = static_cast<int>(digits.length()) - 1; i >= 0; i--) {
        grouped = digits[i] + grouped;
        count++;
        if (count % 3 == 0 && i > 0) {
            grouped = "," + grouped;
        }
    }

    std::string centsText = std::to_string(cents);
    if (cents < 10) {
        centsText = "0" + centsText;
    }

    std::string result = grouped + "." + centsText;
    if (negative) {
        result = "-" + result;
    }
    return result;
}

std::string formatMoney(const std::string &currencyCode, long long amountMinor)
{
    return currencyCode + " " + formatAmount(amountMinor);
}

std::string formatSignedMoney(const std::string &currencyCode, long long amountMinor)
{
    if (amountMinor > 0) {
        return "+" + currencyCode + " " + formatAmount(amountMinor);
    }
    if (amountMinor < 0) {
        return "-" + currencyCode + " " + formatAmount(-amountMinor);
    }
    return currencyCode + " " + formatAmount(0);
}
