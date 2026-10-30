// =====================================================================
//  PasswordPolicy.cpp
// =====================================================================
#include "PasswordPolicy.h"

#include "Transaction.h"         // isBlank
#include "TransactionFilter.h"   // toLowerText

// Passwords that are long enough but still among the first ones an
// attacker tries. Shorter common passwords are refused by the length rule.
const int COMMON_PASSWORD_COUNT = 24;
const char *COMMON_PASSWORDS[COMMON_PASSWORD_COUNT] = {
    "password123", "password1234", "1234567890", "0123456789", "1111111111",
    "qwertyuiop", "qwerty1234", "qwerty12345", "1q2w3e4r5t", "asdfghjkl1",
    "abcdefghij", "abcd123456", "iloveyou123", "welcome123", "letmein123",
    "football123", "superman123", "princess123", "passw0rd123", "p@ssw0rd123",
    "pakistan123", "pakistan786", "finora1234", "finora12345"
};

int characterCount(const std::string &text)
{
    // In UTF-8, every character starts with a byte that is NOT of the form
    // 10xxxxxx. Counting those bytes counts the characters.
    int count = 0;
    int length = static_cast<int>(text.length());
    for (int i = 0; i < length; i++) {
        unsigned char byte = static_cast<unsigned char>(text[i]);
        if ((byte & 0xC0) != 0x80) {
            count++;
        }
    }
    return count;
}

bool isCommonPassword(const std::string &password)
{
    std::string lower = toLowerText(password);
    for (int i = 0; i < COMMON_PASSWORD_COUNT; i++) {
        if (lower == COMMON_PASSWORDS[i]) {
            return true;
        }
    }
    return false;
}

// How many of the four kinds of characters the password uses.
int characterKinds(const std::string &password)
{
    bool hasSmall = false;
    bool hasCapital = false;
    bool hasDigit = false;
    bool hasOther = false;
    int length = static_cast<int>(password.length());
    for (int i = 0; i < length; i++) {
        char c = password[i];
        if (c >= 'a' && c <= 'z') {
            hasSmall = true;
        } else if (c >= 'A' && c <= 'Z') {
            hasCapital = true;
        } else if (c >= '0' && c <= '9') {
            hasDigit = true;
        } else {
            hasOther = true;   // symbols, spaces and non-English letters
        }
    }
    // A bool becomes 1 (true) or 0 (false) when added up.
    return hasSmall + hasCapital + hasDigit + hasOther;
}

bool isOneRepeatedCharacter(const std::string &password)
{
    int length = static_cast<int>(password.length());
    for (int i = 1; i < length; i++) {
        if (password[i] != password[0]) {
            return false;
        }
    }
    return true;
}

PasswordStrength passwordStrength(const std::string &password)
{
    int count = characterCount(password);
    if (count < MIN_PASSWORD_LENGTH || isCommonPassword(password) || isOneRepeatedCharacter(password)) {
        return STRENGTH_VERY_WEAK;
    }

    int score = 1;   // at least 10 characters
    if (count >= 14) {
        score++;
    }
    int kinds = characterKinds(password);
    if (kinds >= 2) {
        score++;
    }
    if (kinds >= 3) {
        score++;
    }
    // score is 1 to 4. The enum values are numbered 0 to 4 in order, so
    // the number can be turned into the matching enum value.
    return static_cast<PasswordStrength>(score);
}

std::string passwordStrengthName(PasswordStrength strength)
{
    switch (strength) {
    case STRENGTH_VERY_WEAK:
        return "Very weak";
    case STRENGTH_WEAK:
        return "Weak";
    case STRENGTH_FAIR:
        return "Fair";
    case STRENGTH_STRONG:
        return "Strong";
    case STRENGTH_VERY_STRONG:
        return "Very strong";
    }
    return "";
}

std::string passwordTip(const std::string &password)
{
    int count = characterCount(password);
    int kinds = characterKinds(password);
    if (count < MIN_PASSWORD_LENGTH) {
        return "Use at least " + std::to_string(MIN_PASSWORD_LENGTH) + " characters.";
    }
    if (isCommonPassword(password) || isOneRepeatedCharacter(password)) {
        return "This password is too easy to guess.";
    }
    if (kinds < 2) {
        return "Mix letters with numbers or symbols.";
    }
    if (count < 14) {
        return "A longer password, 14 characters or more, is stronger.";
    }
    if (kinds < 3) {
        return "Add a capital letter, a number or a symbol.";
    }
    return "";
}

// True if `password` contains `part`, ignoring case. Parts shorter than
// 4 characters are ignored, so a name like "Ali" does not block "Alibaba".
bool containsPersonalPart(const std::string &password, const std::string &part)
{
    if (characterCount(part) < 4) {
        return false;
    }
    return toLowerText(password).find(toLowerText(part)) != std::string::npos;
}

std::string validateNewPassword(const std::string &password, const std::string &email,
                                const std::string &displayName)
{
    int count = characterCount(password);
    if (count < MIN_PASSWORD_LENGTH) {
        return "Use at least " + std::to_string(MIN_PASSWORD_LENGTH) + " characters.";
    }
    if (count > MAX_PASSWORD_LENGTH) {
        return "Use at most " + std::to_string(MAX_PASSWORD_LENGTH) + " characters.";
    }
    if (isBlank(password)) {
        return "A password cannot be only spaces.";
    }
    if (isCommonPassword(password)) {
        return "This password is too common. Please choose another one.";
    }

    // The part of the email before the @, and every word of the name.
    std::string emailName = email.substr(0, email.find('@'));
    bool personal = containsPersonalPart(password, emailName);
    std::string word;
    int nameLength = static_cast<int>(displayName.length());
    for (int i = 0; i <= nameLength; i++) {
        if (i == nameLength || displayName[i] == ' ') {
            personal = personal || containsPersonalPart(password, word);
            word.clear();
        } else {
            word += displayName[i];
        }
    }
    if (personal) {
        return "Do not use your name or email address in the password.";
    }

    if (passwordStrength(password) < STRENGTH_FAIR) {
        return "This password is too easy to guess. Make it longer, or mix letters, numbers and symbols.";
    }
    return "";
}
