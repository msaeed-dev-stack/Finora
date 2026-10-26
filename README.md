# Finora

**Your Money. Your Control.**

Finora is a desktop personal finance and budget management application for Windows. It is written in C++17 with Qt 6 Widgets and is being built as a Programming Fundamentals semester project.

> **Current status: Milestone 1 of 7.** The main window, sidebar navigation, visual theme and automated tests work. The finance features are planned and are **not yet implemented**. Each page in the program says which milestone will add its feature.

## Student details

| | |
|---|---|
| Student name | `[Your Name]` |
| Roll number | `[Your Roll No.]` |
| Course | Programming Fundamentals (SW112) |
| Instructor | Mr. Sajjad Ali |
| Department | Department of Software Engineering, Mehran University of Engineering and Technology, Jamshoro |
| Semester and year | 1st semester, 1st year |
| Project type | Individual semester project |

## Problem statement

Many people record their money in a notebook or not at all. They cannot easily see how much they earned and spent this month, whether they are staying inside a budget, or how close they are to a savings target. When some income arrives in another currency, such as US dollars from freelance work, adding the numbers together gives totals that are confusing or simply wrong.

Finora is an offline desktop program that records income and expenses, compares spending with monthly budgets, tracks savings goals, and keeps every currency separate unless the user asks for a clearly labelled conversion.

## Objectives

1. Record income and expense transactions with amount, currency, category, date and description.
2. Show monthly summaries, budget usage and savings progress on a dashboard.
3. Warn the user at 80% of a budget and alert them at 100%.
4. Support PKR, USD and EUR with exchange rates that the user enters.
5. Save everything in a local SQLite database so data survives closing the program.
6. Produce reports and export them to CSV.
7. Use clear, modular C++ that demonstrates the Programming Fundamentals syllabus.

## Features and status

| Feature | Status |
|---|---|
| Main window with sidebar navigation for 7 pages | Done in Milestone 1 |
| Light theme with hand-drawn icons | Done in Milestone 1 |
| Time-of-day greeting on the dashboard | Done in Milestone 1 |
| Automated core and interface tests | Done in Milestone 1 |
| SQLite database that keeps data after closing | Planned: Milestone 2 |
| Add, edit, delete, search and filter transactions | Planned: Milestone 3 |
| Dashboard summaries, monthly category budgets and alerts | Planned: Milestone 4 |
| Savings goals; PKR, USD and EUR with manual exchange rates | Planned: Milestone 5 |
| Reports and CSV export | Planned: Milestone 6 |
| Final testing, documentation and Windows package | Planned: Milestone 7 |

## Requirements to build

- Windows 10 or 11, 64-bit
- Qt 6.12.0 for MinGW 64-bit, installed at `C:\Qt\6.12.0\mingw_64`
- MinGW 13.1.0 64-bit, installed with Qt at `C:\Qt\Tools\mingw1310_64`
- CMake 3.30 and Ninja, installed with Qt in `C:\Qt\Tools`

If Qt is installed in a different folder, change the paths in `CMakePresets.json` and `build.bat`.

An older GCC 6.3 in `C:\MinGW\bin` cannot build Qt 6 programs. The build files always choose the Qt compiler instead.

## Build and run

**Easiest way.** Double-click `build.bat`, or run it from a terminal in the project folder. It configures, builds and runs the tests:

```bat
build.bat
```

Then start the program by double-clicking `build\debug\Finora.exe`, or from the terminal:

```bat
build\debug\Finora.exe
```

After every build, `windeployqt` copies the Qt DLLs, plugins and MinGW runtime next to `Finora.exe`, so it runs without Qt on the PATH.

**Step by step in PowerShell:**

```powershell
$env:PATH = "C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;$env:PATH"
cmake --preset debug           # 1. configure: find Qt and the compiler
cmake --build --preset debug   # 2. compile and link
ctest --preset debug           # 3. run the automated tests
.\build\debug\Finora.exe       # 4. start Finora
```

For an optimised build, use `release` instead of `debug`, or run `build.bat release`.

## Automated tests

| Test program | What it checks | Last result, 27 Oct 2026 |
|---|---|---|
| `finora_tests` | `greetingForHour()` at every boundary hour and for invalid hours | 11 of 11 checks pass |
| `finora_ui_tests` | The real main window: 7 pages, 7 buttons, starts on the dashboard, every button opens its own page and is the only one selected | 22 of 22 checks pass |

Run a test program directly to see every check:

```powershell
.\build\debug\finora_tests.exe
```

## Project structure

```text
Finora/
├── CMakeLists.txt             Build script: programs, library and tests
├── CMakePresets.json          Qt and compiler paths for debug and release builds
├── build.bat                  One-click build and test script
├── src/
│   ├── main.cpp               Program entry point
│   ├── core/                  Plain C++ logic without Qt, tested directly
│   │   └── Greeting.h/.cpp        Time-of-day greeting
│   └── ui/                    Qt Widgets user interface
│       ├── MainWindow.h/.cpp      Window: sidebar and page area
│       ├── Sidebar.h/.cpp         Logo and navigation buttons
│       ├── PageHeader.h/.cpp      Title and subtitle used on every page
│       ├── PlaceholderPage.h/.cpp "Coming in Milestone N" page
│       └── Theme.h/.cpp           Style, font and style sheet
├── resources/
│   ├── icons/                 Hand-drawn SVG icons, grey and white versions
│   └── styles/light.qss       Colours, spacing and rounded corners
├── tests/
│   ├── finora_tests.cpp       Core logic tests
│   └── finora_ui_tests.cpp    Interface tests
└── docs/                      Design, requirements and viva preparation
```

## Documentation

- [Design: architecture, flowcharts, pseudocode, money rules](docs/DESIGN.md)
- [Requirements checklist](docs/REQUIREMENTS_CHECKLIST.md)
- [C++ concepts map](docs/CPP_CONCEPTS_MAP.md)
- [Viva preparation](docs/VIVA_PREPARATION.md)

## Data storage and backup

Planned for Milestone 2. Finora will keep all data in one SQLite database file in the user's AppData folder. A backup will be a copy of that file, made while Finora is closed. The exact location and steps will be added here when the database is built.

## Known limitations at Milestone 1

- No finance features yet. All seven pages are placeholders that name their milestone.
- Light theme only.
- The build paths assume Qt is installed in `C:\Qt`.
- The greeting is chosen when the window opens and does not change while the window stays open.
