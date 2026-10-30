// =====================================================================
//  AccountRules.cpp
// =====================================================================
#include "AccountRules.h"

#include "Money.h"               // trimSpaces
#include "PasswordPolicy.h"      // characterCount
#include "TransactionFilter.h"   // toLowerText

std::string normalizeEmail(const std::string &email)
{
    return toLowerText(trimSpaces(email));
}

// Letters, digits and the few symbols allowed in an email address.
bool isEmailCharacter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
           || c == '.' || c == '_' || c == '%' || c == '+' || c == '-';
}

std::string validateEmail(const std::string &email)
{
    const std::string INVALID = "Please enter a valid email address, for example name@example.com.";
    std::string text = trimSpaces(email);
    if (text.empty()) {
        return "Please enter your email address.";
    }
    if (text.length() > 254) {
        return "This email address is too long.";
    }

    // Exactly one @, with something before it and a domain after it.
    std::string::size_type at = text.find('@');
    if (at == std::string::npos || at == 0 || text.find('@', at + 1) != std::string::npos) {
        return INVALID;
    }
    std::string local = text.substr(0, at);
    std::string domain = text.substr(at + 1);
    if (local.length() > 64 || domain.length() < 3) {
        return INVALID;
    }

    int length = static_cast<int>(text.length());
    for (int i = 0; i < length; i++) {
        if (i != static_cast<int>(at) && !isEmailCharacter(text[i])) {
            return INVALID;   // spaces, quotes and other characters are refused
        }
    }

    // The domain needs a dot, not at either end, and never two dots in a row.
    if (domain.find('.') == std::string::npos || domain.front() == '.' || domain.back() == '.'
        || domain.find("..") != std::string::npos || local.front() == '.' || local.back() == '.') {
        return INVALID;
    }
    return "";
}

std::string validateDisplayName(const std::string &displayName)
{
    std::string name = trimSpaces(displayName);
    if (name.empty()) {
        return "Please enter your name.";
    }
    if (characterCount(name) > 60) {
        return "A name can be at most 60 characters long.";
    }
    return "";
}

std::string firstNameOf(const std::string &displayName)
{
    std::string name = trimSpaces(displayName);
    std::string::size_type space = name.find(' ');
    if (space == std::string::npos) {
        return name;
    }
    return name.substr(0, space);
}

std::string formatRecoveryCode(const std::string &code)
{
    if (code.length() != static_cast<std::string::size_type>(RECOVERY_CODE_LENGTH)) {
        return code;
    }
    return code.substr(0, 4) + "-" + code.substr(4, 4) + "-" + code.substr(8, 4);
}

std::string normalizeRecoveryCode(const std::string &typed)
{
    std::string code;
    int length = static_cast<int>(typed.length());
    for (int i = 0; i < length; i++) {
        char c = typed[i];
        if (c == ' ' || c == '-') {
            continue;   // ignore the separators
        }
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
        if (RECOVERY_CODE_ALPHABET.find(c) == std::string::npos) {
            return "";   // a character that never appears in a code
        }
        code += c;
    }
    if (code.length() != static_cast<std::string::size_type>(RECOVERY_CODE_LENGTH)) {
        return "";
    }
    return code;
}

int signInDelaySeconds(int failedAttempts)
{
    if (failedAttempts < 3) {
        return 0;
    }
    // 30 seconds after the 3rd failure, then twice as long after each one.
    int seconds = 30;
    for (int attempt = 4; attempt <= failedAttempts; attempt++) {
        seconds = seconds * 2;
        if (seconds >= 900) {
            return 900;   // at most 15 minutes
        }
    }
    return seconds;
}
