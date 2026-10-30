// =====================================================================
//  PasswordHasher.cpp
// =====================================================================
#include "PasswordHasher.h"

#include <sodium.h>

#include "core/AccountRules.h"   // RECOVERY_CODE_ALPHABET, RECOVERY_CODE_LENGTH

bool PasswordHasher::initializeLibrary()
{
    // sodium_init returns 0 the first time, 1 if it was already started,
    // and -1 if libsodium cannot work on this computer.
    return sodium_init() >= 0;
}

std::string PasswordHasher::libraryVersion()
{
    return sodium_version_string();
}

PasswordHasher::PasswordHasher(HashStrength strength)
{
    if (strength == HASH_MODERATE) {
        passCount = crypto_pwhash_OPSLIMIT_MODERATE;     // 3
        memoryLimit = crypto_pwhash_MEMLIMIT_MODERATE;   // 268,435,456 bytes = 256 MiB
    } else {
        passCount = crypto_pwhash_OPSLIMIT_INTERACTIVE;     // 2
        memoryLimit = crypto_pwhash_MEMLIMIT_INTERACTIVE;   // 67,108,864 bytes = 64 MiB
    }
}

PasswordHasher::PasswordHasher(unsigned long long passes, std::size_t memoryBytes)
    : passCount(passes), memoryLimit(memoryBytes)
{
}

bool PasswordHasher::hash(const std::string &secret, std::string &storedHash) const
{
    storedHash.clear();

    // crypto_pwhash_str picks a random 16-byte salt, runs Argon2id and
    // writes everything needed to check the password later into `buffer`.
    char buffer[crypto_pwhash_STRBYTES];
    int result = crypto_pwhash_str(buffer, secret.data(), secret.size(), passCount, memoryLimit);
    if (result != 0) {
        return false;   // the memory could not be allocated
    }
    storedHash = buffer;
    return true;
}

bool PasswordHasher::verify(const std::string &storedHash, const std::string &secret) const
{
    return crypto_pwhash_str_verify(storedHash.c_str(), secret.data(), secret.size()) == 0;
}

bool PasswordHasher::needsRehash(const std::string &storedHash) const
{
    // 1 means "other settings", -1 means "not a valid hash string".
    return crypto_pwhash_str_needs_rehash(storedHash.c_str(), passCount, memoryLimit) != 0;
}

unsigned long long PasswordHasher::passes() const
{
    return passCount;
}

std::size_t PasswordHasher::memoryBytes() const
{
    return memoryLimit;
}

std::string PasswordHasher::newRecoveryCode()
{
    // randombytes_uniform(32) gives a random number from 0 to 31, each
    // equally likely, so every character of the code is unpredictable.
    std::string code;
    int alphabetSize = static_cast<int>(RECOVERY_CODE_ALPHABET.length());
    for (int i = 0; i < RECOVERY_CODE_LENGTH; i++) {
        code += RECOVERY_CODE_ALPHABET[randombytes_uniform(static_cast<uint32_t>(alphabetSize))];
    }
    return code;
}

void PasswordHasher::wipe(std::string &secret)
{
    if (!secret.empty()) {
        // sodium_memzero cannot be skipped by the compiler's optimiser,
        // unlike a plain loop that writes zeros into memory never read again.
        sodium_memzero(&secret[0], secret.size());
    }
    secret.clear();
}
