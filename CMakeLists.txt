# =====================================================================
#  Finora - Desktop Personal Finance & Budget Management System
#  CMake build script (C++17 + Qt 6 Widgets, Windows + MinGW)
#
#  This file tells CMake:
#    1. which C++ standard and Qt modules to use,
#    2. which source files make up each program, and
#    3. how to run the automated tests.
# =====================================================================

cmake_minimum_required(VERSION 3.21)

project(Finora
    VERSION 0.2.0
    DESCRIPTION "Desktop personal finance and budget management system"
    LANGUAGES CXX)

# ---- C++ standard: plain C++17, no compiler-specific extensions ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Qt 6: Widgets gives us windows, buttons and layouts, ----
# ---- and Sql gives database access (used only by finora_database) ----
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Sql)

# Turns on Qt's code generators: moc (for signals and slots) and
# rcc (for icons and style sheets that are built into the program).
qt_standard_project_setup()

# ---- Compiler warnings: ask the compiler to report possible mistakes ----
if(MSVC)
    set(FINORA_WARNINGS /W4)
else()
    set(FINORA_WARNINGS -Wall -Wextra -Wpedantic)
endif()

# =====================================================================
#  1. finora_core - the plain C++ logic (no Qt inside).
#     Both the application and the tests use this library.
# =====================================================================
add_library(finora_core STATIC
    src/core/Greeting.h       src/core/Greeting.cpp
    src/core/Transaction.h    src/core/Transaction.cpp
)
target_include_directories(finora_core PUBLIC src)
target_compile_options(finora_core PRIVATE ${FINORA_WARNINGS})

# =====================================================================
#  2. finora_database - every SQL statement in Finora lives here.
#     Qt SQL is linked PRIVATE: code that uses this library does not
#     see any SQL classes, only DatabaseManager.
# =====================================================================
add_library(finora_database STATIC
    src/database/DatabaseManager.h
    src/database/DatabaseManager.cpp
)
target_include_directories(finora_database PUBLIC src)
target_link_libraries(finora_database
    PUBLIC  finora_core Qt6::Core
    PRIVATE Qt6::Sql)
target_compile_options(finora_database PRIVATE ${FINORA_WARNINGS})

# =====================================================================
#  3. Finora - the desktop application
# =====================================================================

# User-interface source files (the interface tests use them too).
set(FINORA_UI_SOURCES
    src/ui/MainWindow.h       src/ui/MainWindow.cpp
    src/ui/Sidebar.h          src/ui/Sidebar.cpp
    src/ui/PageHeader.h       src/ui/PageHeader.cpp
    src/ui/PlaceholderPage.h  src/ui/PlaceholderPage.cpp
    src/ui/Theme.h            src/ui/Theme.cpp
)

# Icons and the style sheet. They are built into the .exe file,
# so the program never depends on loose image files.
set(FINORA_RESOURCE_FILES
    resources/icons/logo.svg
    resources/icons/dashboard.svg     resources/icons/dashboard_active.svg
    resources/icons/transactions.svg  resources/icons/transactions_active.svg
    resources/icons/budgets.svg       resources/icons/budgets_active.svg
    resources/icons/goals.svg         resources/icons/goals_active.svg
    resources/icons/reports.svg       resources/icons/reports_active.svg
    resources/icons/currency.svg      resources/icons/currency_active.svg
    resources/icons/settings.svg      resources/icons/settings_active.svg
    resources/styles/light.qss
)

qt_add_executable(Finora WIN32
    src/main.cpp
    ${FINORA_UI_SOURCES}
)
qt_add_resources(Finora "finora_resources"
    PREFIX "/"
    BASE resources
    FILES ${FINORA_RESOURCE_FILES}
)
target_include_directories(Finora PRIVATE src)
target_link_libraries(Finora PRIVATE finora_core finora_database Qt6::Widgets)
target_compile_options(Finora PRIVATE ${FINORA_WARNINGS})
# The version number above is passed to the C++ code as FINORA_VERSION.
target_compile_definitions(Finora PRIVATE FINORA_VERSION="${PROJECT_VERSION}")

# After every build, windeployqt copies the Qt DLLs, Qt plugins and the
# MinGW runtime next to Finora.exe, so it also runs by double-clicking it.
# Because Finora uses Qt SQL, windeployqt would copy all six SQL drivers.
# Finora needs only SQLite (qsqlite); the other five need database client
# programs that are not installed, so they are excluded.
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET Finora POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --release --compiler-runtime -svg
                    --exclude-plugins qsqlibase,qsqlmimer,qsqloci,qsqlodbc,qsqlpsql
                    --no-translations --no-opengl-sw
                    --no-system-d3d-compiler --no-system-dxc-compiler
                    --verbose 0
                    "$<TARGET_FILE:Finora>"
            COMMENT "Copying Qt files next to Finora.exe (windeployqt)"
        )
    else()
        message(WARNING "windeployqt not found: run Finora.exe with Qt's bin folder on PATH.")
    endif()
endif()

# =====================================================================
#  4. Automated tests   (run them with:  ctest --preset debug)
# =====================================================================
enable_testing()

# Qt's own plugin folder. windeployqt copies only the plugins Finora.exe
# needs, so the tests also look here (for "offscreen" and for SQLite).
get_filename_component(FINORA_QT_PLUGINS_DIR "${Qt6_DIR}/../../../plugins" ABSOLUTE)

# 4a. Core logic tests: a small console program without Qt.
add_executable(finora_tests tests/finora_tests.cpp)
target_link_libraries(finora_tests PRIVATE finora_core)
target_compile_options(finora_tests PRIVATE ${FINORA_WARNINGS})
add_test(NAME core_tests COMMAND finora_tests)

# 4b. Interface tests: build the real main window in memory and
#     click every sidebar button from code.
qt_add_executable(finora_ui_tests
    tests/finora_ui_tests.cpp
    ${FINORA_UI_SOURCES}
)
qt_add_resources(finora_ui_tests "finora_ui_test_resources"
    PREFIX "/"
    BASE resources
    FILES ${FINORA_RESOURCE_FILES}
)
target_include_directories(finora_ui_tests PRIVATE src)
target_link_libraries(finora_ui_tests PRIVATE finora_core Qt6::Widgets)
target_compile_options(finora_ui_tests PRIVATE ${FINORA_WARNINGS})
target_compile_definitions(finora_ui_tests PRIVATE FINORA_VERSION="${PROJECT_VERSION}")
add_test(NAME ui_tests COMMAND finora_ui_tests)
# "offscreen" draws windows in memory, so nothing pops up during tests.
set_tests_properties(ui_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=${FINORA_QT_PLUGINS_DIR}")

# 4c. Database tests: real SQLite files inside a temporary folder.
add_executable(finora_db_tests tests/finora_db_tests.cpp)
target_link_libraries(finora_db_tests PRIVATE finora_database finora_core Qt6::Core Qt6::Sql)
target_compile_options(finora_db_tests PRIVATE ${FINORA_WARNINGS})
add_test(NAME database_tests COMMAND finora_db_tests)
set_tests_properties(database_tests PROPERTIES
    ENVIRONMENT "QT_PLUGIN_PATH=${FINORA_QT_PLUGINS_DIR}")
