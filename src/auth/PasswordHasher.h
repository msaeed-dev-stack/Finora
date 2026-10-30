// =====================================================================
//  PasswordHasher.h
//  One-way password hashing with Argon2id, secure random recovery
//  codes, and wiping secrets from memory. This is the only part of
//  Finora that talks to libsodium (third_party/libsodium-1.0.22).
// =====================================================================
#ifndef FINORA_PASSWORDHASHER_H
#define FINORA_PASSWORDHASHER_H

#include <cstddef>
#include <string>

// How much work Argon2id does for one hash. More memory and more passes
// make every guess of an attacker slower, but signing in slower too.
// Times measured on the development PC on 30 October 2026.
enum HashStrength {
    HASH_INTERACTIVE,   // 2 passes over 64 MiB:  about 0.1 s. Used for recovery codes
    HASH_MODERATE       // 3 passes over 256 MiB: about 0.6 s. Used for account passwords
};

class PasswordHasher
{
public:
    // Starts libsodium. Must be called once, before any other function.
    // Returns false if the library cannot start (then Finora must not run).
    static bool initializeLibrary();

    // For example "1.0.22".
    static std::string libraryVersion();

    // Constructor overloading: one of the two standard strengths, or
    // exact numbers (the tests use this to force a memory failure).
    PasswordHasher(HashStrength strength);
    PasswordHasher(unsigned long long passes, std::size_t memoryBytes);

    // Turns a secret into a string that is safe to store, such as
    //   $argon2id$v=19$m=262144,t=3,p=1$<random salt>$<hash>
    // The salt is new and random every time, so two people with the same
    // password get different strings. The string cannot be turned back
    // into the password. Returns false, with storedHash empty, if the
    // computer could not provide the memory.
    bool hash(const std::string &secret, std::string &storedHash) const;

    // True if `secret` gives the same hash as `storedHash`. libsodium
    // compares in constant time and reads the salt and settings from the
    // stored string itself.
    bool verify(const std::string &storedHash, const std::string &secret) const;

    // True if `storedHash` was made with other settings than this hasher's,
    // so it should be replaced after the next successful sign-in.
    bool needsRehash(const std::string &storedHash) const;

    unsigned long long passes() const;
    std::size_t memoryBytes() const;

    // A new random recovery code: 12 characters from RECOVERY_CODE_ALPHABET
    // (core/AccountRules.h), chosen by the operating system's secure
    // random number generator.
    static std::string newRecoveryCode();

    // Overwrites every byte of `secret` with zeros, then empties it, so a
    // password does not stay readable in memory longer than needed.
    static void wipe(std::string &secret);

private:
    unsigned long long passCount;
    std::size_t memoryLimit;
};

#endif // FINORA_PASSWORDHASHER_H
