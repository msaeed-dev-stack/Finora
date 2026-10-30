// =====================================================================
//  Money.h
//  Reading amounts that the user types, and writing amounts as text.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_MONEY_H
#define FINORA_MONEY_H

#include <string>

// Returns text without spaces or tabs at the start and the end.
std::string trimSpaces(const std::string &text);

// Reads an amount typed by the user, such as "1,200.50", and stores it in
// minor units (paisa or cents): amountMinor becomes 120050.
// Allowed: digits, commas between digits (they are ignored, so both
// 1,200 and 1,20,000 work), and one decimal point with at most two digits
// after it. The decimal separator is always a point.
// Returns an empty string on success. Otherwise it returns a message for
// the user and leaves amountMinor unchanged.
std::string parseAmount(const std::string &text, long long &amountMinor);

// Writes minor units as text with thousands separators:
// 120050 gives "1,200.50", 5 gives "0.05", -120050 gives "-1,200.50".
std::string formatAmount(long long amountMinor);

// Puts the currency code in front: ("PKR", 120050) gives "PKR 1,200.50".
std::string formatMoney(const std::string &currencyCode, long long amountMinor);

#endif // FINORA_MONEY_H
