// =====================================================================
//  AccountRules.h
//  Rules for accounts: email addresses, names, recovery codes, the delay
//  after wrong passwords, and how long a sign-in lasts.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_ACCOUNTRULES_H
#define FINORA_ACCOUNTRULES_H

#include <string>

// ---- Email addresses and names ---------------------------------------

// The email as Finora stores it: without spaces at the ends, and with
// capital letters A to Z made small. "  Ali@Mail.PK " gives "ali@mail.pk".
std::string normalizeEmail(const std::string &email);

// "" if the email looks like name@example.com, otherwise the reason.
std::string validateEmail(const std::string &email);

// "" if the display name has 1 to 60 characters, otherwise the reason.
std::string validateDisplayName(const std::string &displayName);

// The first word of a name, for the greeting: "Muhammad Saeed" gives "Muhammad".
std::string firstNameOf(const std::string &displayName);

// ---- Recovery codes ---------------------------------------------------
// A recovery code replaces the password once, when it is forgotten. It is
// 12 characters from a 32-character alphabet without 0, 1, I and O (they
// are easy to confuse), shown as 7KQ4-M2TX-9RHC. 32^12 = 2^60 possible codes.

const std::string RECOVERY_CODE_ALPHABET = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
const int RECOVERY_CODE_LENGTH = 12;
const int RECOVERY_CODE_COUNT = 10;   // codes made for every account

// "7KQ4M2TX9RHC" gives "7KQ4-M2TX-9RHC".
std::string formatRecoveryCode(const std::string &code);

// Reads a typed code: spaces and dashes are ignored and small letters
// become capitals, so "7kq4 m2tx-9rhc" gives "7KQ4M2TX9RHC". Returns ""
// if the result is not 12 characters from the alphabet.
std::string normalizeRecoveryCode(const std::string &typed);

// ---- Sign-in delays and sessions ----------------------------------------

// After a wrong password, the next try must wait this many seconds:
// failures 1 and 2: no wait; 3: 30 s; 4: 60 s; then double each time,
// at most 15 minutes. Guessing many passwords therefore takes very long.
int signInDelaySeconds(int failedAttempts);

const int SESSION_IDLE_MINUTES = 15;   // signed out after 15 minutes without activity
const int SESSION_MAX_HOURS = 12;      // and in any case 12 hours after signing in

#endif // FINORA_ACCOUNTRULES_H
