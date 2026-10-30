// =====================================================================
//  PasswordPolicy.h
//  The rules a new password must follow, and the strength shown by the
//  meter under the password box.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_PASSWORDPOLICY_H
#define FINORA_PASSWORDPOLICY_H

#include <string>

const int MIN_PASSWORD_LENGTH = 10;    // characters, not bytes
const int MAX_PASSWORD_LENGTH = 128;

// How hard a password is to guess. The meter shows these names, so the
// strength never depends on colour alone.
enum PasswordStrength {
    STRENGTH_VERY_WEAK,
    STRENGTH_WEAK,
    STRENGTH_FAIR,
    STRENGTH_STRONG,
    STRENGTH_VERY_STRONG
};

// The number of characters in UTF-8 text. "é" and "پ" count as one
// character each, although each takes two bytes.
int characterCount(const std::string &text);

// True if the password is on Finora's list of very common passwords,
// ignoring upper and lower case. Attackers always try these first.
bool isCommonPassword(const std::string &password);

// The strength: one point each for 10 or more characters, 14 or more
// characters, 2 or more kinds of characters and 3 or more kinds (kinds:
// small letters, capital letters, digits, other characters). A password
// that is too short, common or one repeated character is always very weak.
PasswordStrength passwordStrength(const std::string &password);

// "Very weak", "Weak", "Fair", "Strong" or "Very strong".
std::string passwordStrengthName(PasswordStrength strength);

// One short tip that would make the password stronger, or "" when it
// is already very strong.
std::string passwordTip(const std::string &password);

// Checks a new password. Returns "" if it may be used, otherwise the
// reason. A password must be at least "Fair" and must not contain the
// person's name or the first part of their email address.
std::string validateNewPassword(const std::string &password, const std::string &email,
                                const std::string &displayName);

#endif // FINORA_PASSWORDPOLICY_H
