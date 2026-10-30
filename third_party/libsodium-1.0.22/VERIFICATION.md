# libsodium 1.0.22: source and verification record

Finora uses libsodium for Argon2id password hashing and for secure random numbers (recovery codes). This folder contains the 64-bit part of the official, prebuilt MinGW package, **unchanged**. This file records where it came from and how it was checked, so anyone can repeat the checks.

**Do not upgrade or replace these files without the project owner's approval.** A new version must be downloaded and verified the same way, and this record updated.

## Package

| | |
|---|---|
| Library | libsodium 1.0.22, released 9 April 2026 |
| Package | `libsodium-1.0.22-mingw.tar.gz` (official prebuilt MinGW build, 1,337,453 bytes) |
| Official sources | https://download.libsodium.org/libsodium/releases/libsodium-1.0.22-mingw.tar.gz and https://github.com/jedisct1/libsodium/releases/download/1.0.22-RELEASE/libsodium-1.0.22-mingw.tar.gz |
| Downloaded | 30 October 2026, from both sources |
| SHA-256 of the package | `1d99e0afaf27bce664249232e9dc628ae6bb7b49f0ab53ad6db372b028a35d9d` (identical from both sources) |
| Licence | ISC, see `LICENSE` (taken from the official repository at tag `1.0.22-RELEASE`, because the binary package contains no licence file) |

## Signature check

libsodium releases are signed with [minisign](https://jedisct1.github.io/minisign/) (Ed25519).

- **Public key**, published in libsodium's official documentation (`installation/README.md` in github.com/jedisct1/libsodium-doc), not taken from the package: `RWQf6LRCGA9i53mlYecO4IzT51TGPpvWucNSCh1CBM0QTaLn73Y7GFO3`
- **Signature file:** `libsodium-1.0.22-mingw.tar.gz.minisig`, kept in this folder.
- **Trusted comment:** `timestamp:1775772654 file:libsodium-1.0.22-mingw.tar.gz hashed` (signed on 2026-04-09 22:10:54 UTC).

**Result on 30 October 2026:**

| Check | Result |
|---|---|
| The key id in the signature matches the official key | Yes |
| The file signature (Ed25519 over the BLAKE2b-512 hash of the package) | Valid |
| The global signature over the trusted comment (timestamp and file name) | Valid |
| Negative control: a copy of the package with one byte changed | Correctly rejected |

The check was done with a short Node.js script that uses only Node's built-in cryptography (OpenSSL's Ed25519 and BLAKE2b-512), so no code from the package was trusted while checking it. The official tool gives the same answer:

```text
minisign -Vm libsodium-1.0.22-mingw.tar.gz -P RWQf6LRCGA9i53mlYecO4IzT51TGPpvWucNSCh1CBM0QTaLn73Y7GFO3
```

## Files in this folder

Copied unchanged from `libsodium-win64/` inside the verified package:

| File | SHA-256 |
|---|---|
| `bin/libsodium-26.dll` (757,512 bytes) | `452e93339e157810262f8cb46328f21ab5a026bcff2645658f4d12d62316fb67` |
| `lib/libsodium.dll.a` (import library) | `f71857863487bd09b8014dbfa38e758b81be3631e51330d1ee8761f1204b88bf` |
| `include/` (77 header files) | `65ddd10789b047fc4800507aa54c57939e705f7d257ea1dcb1478d8d04a7eeed` (SHA-256 of all headers joined in sorted path order) |

Added to this folder: `LICENSE` (ISC), the signature file `libsodium-1.0.22-mingw.tar.gz.minisig`, and this record.

Not taken: the 32-bit build, the static library `libsodium.a`, `libsodium.la`, the pkg-config file and the `.def` file. Finora does not use them.

## Compatibility check (30 October 2026, this project's toolchain)

The check was done in a separate scratch project before anything was added to Finora.

- **Format:** 64-bit (`pei-x86-64`); it exports all functions Finora needs, including `sodium_init`, `crypto_pwhash_str`, `crypto_pwhash_str_verify`, `crypto_pwhash_str_needs_rehash`, `randombytes_uniform` and `sodium_memzero`.
- **C runtime:** the DLL, Qt 6.12 and GCC 13.1 all use `msvcrt.dll`, so there is no C runtime mismatch.
- **Other dependencies:** `KERNEL32.dll` and `ADVAPI32.dll` (Windows), plus one function, `__emutls_get_address`, from `libgcc_s_seh-1.dll`. GCC 13.1's `libgcc_s_seh-1.dll`, which `windeployqt` already copies next to `Finora.exe`, exports it.
- **Argon2id test:** compiled with CMake, Ninja, GCC 13.1, Qt 6.12 and C++17. 21 of 21 checks passed, including hash and verify, wrong and empty passwords, a tampered hash, unique salts, Unicode and long passwords, `needs_rehash`, and the MODERATE settings.
- **Clean PATH:** the test ran with only Windows folders on the PATH, from a folder holding just the `windeployqt` files and `libsodium-26.dll`.
- **Independent cross-check:** OpenSSL 3.5's separate Argon2id implementation reproduced libsodium's raw output and both stored password strings byte for byte.

The DLL is not Authenticode-signed. Its authenticity rests on the minisign signature above.
