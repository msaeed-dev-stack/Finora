# Third-party notices

Finora uses the following software and fonts made by other people. Each is used under its own licence, listed here.

## Fonts (built into Finora.exe)

| Font | Used for | Version and source | Licence |
|---|---|---|---|
| Inter | Body text, numbers, tables and amounts | 4.1, the static TrueType files from the official release `Inter-4.1.zip` at github.com/rsms/inter | SIL Open Font License 1.1. Copyright (c) 2016 The Inter Project Authors |
| Manrope | Headings | The static TrueType files at commit `6f81ebe` of github.com/googlefonts/manrope, the same version Google Fonts uses | SIL Open Font License 1.1. Copyright 2018 The Manrope Project Authors |

The full licence texts are in `resources/fonts/Inter-OFL.txt` and `resources/fonts/Manrope-OFL.txt`. They are built into Finora.exe next to the fonts, and **Settings > About Finora > View font licences** shows them.

Files included: `Inter-Regular.ttf`, `Inter-Medium.ttf`, `Inter-SemiBold.ttf`, `Inter-Bold.ttf`, `Manrope-SemiBold.ttf`, `Manrope-Bold.ttf`, `Manrope-ExtraBold.ttf`. The files are unchanged. The OFL allows bundling the fonts with a program, including a commercial one, as long as the fonts are not sold on their own and the licence goes with them.

SHA-256 of the font files as downloaded on 30 October 2026:

```text
40d692fce188e4471e2b3cba937be967878f631ad3ebbbdcd587687c7ebe0c82  Inter-Regular.ttf
97ad806f526e41546d46365bb3a393145f75b7b1568913db74549ad8b8dba872  Inter-Medium.ttf
78a843fade9d4612a5567302fb595b56976eb5fcebf4fea5a5912d638bafcde3  Inter-SemiBold.ttf
288316099b1e0a47a4716d159098005eef7c0066921f34e3200393dbdb01947f  Inter-Bold.ttf
e80a2c37e47ec09bff5a9960257bf9001e87d5fb7bf35719ef4958763a0c70ac  Manrope-SemiBold.ttf
2da33eb378d59e6314ed7afdfa837cdcb60e41ac8b1f5d3c4909471b95fcf7d9  Manrope-Bold.ttf
2522cd61b754100a42a53024148a134c63efc5d114447474a7f13519966cfad5  Manrope-ExtraBold.ttf
```

## Libraries

| Library | Used for | Licence |
|---|---|---|
| libsodium 1.0.22 (`libsodium-26.dll`) | Argon2id password hashing, secure random recovery codes, wiping secrets from memory | ISC License, Copyright (c) 2013-2026 Frank Denis. Full text in `third_party/libsodium-1.0.22/LICENSE`. The official prebuilt MinGW DLL is used unchanged; its minisign signature and checksums are recorded in `third_party/libsodium-1.0.22/VERIFICATION.md` |
| Qt 6.12 (Core, Gui, Widgets, Sql, Svg) | Windows, widgets, the SQLite connection, drawing the icons | GNU LGPL version 3. Finora links to the Qt DLLs dynamically; `windeployqt` copies them next to Finora.exe unchanged |
| SQLite (inside Qt's `qsqlite` driver) | The database file | Public domain |
| MinGW-w64 runtime (copied by `windeployqt`) | C++ runtime DLLs | Its own permissive licences, including the GCC Runtime Library Exception |

The ISC License of libsodium:

```text
Copyright (c) 2013-2026
Frank Denis <j at pureftpd dot org>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
```

No other outside code is included. The icons and the Finora logo were drawn for this project.
