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
    VERSION 0.5.0
    DESCRIPTION "Desktop personal finance and budget management system"
    LANGUAGES CXX)

# ---- C++ standard: plain C++17, no compiler-specific extensions ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Qt 6: Widgets gives us windows, buttons and layouts, Sql gives ----
# ---- database access (used only by finora_database), and Svg draws  ----
# ---- the icons in the colours of the current theme.                 ----
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Sql Svg Gui)

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
    src/core/Money.h          src/core/Money.cpp
    src/core/TransactionFilter.h  src/core/TransactionFilter.cpp
    src/core/Totals.h         src/core/Totals.cpp
    src/core/Budget.h         src/core/Budget.cpp
    src/core/PasswordPolicy.h src/core/PasswordPolicy.cpp
    src/core/AccountRules.h   src/core/AccountRules.cpp
    src/core/Session.h        src/core/Session.cpp
    src/core/Plans.h          src/core/Plans.cpp
    src/core/Subscription.h   src/core/Subscription.cpp
    src/core/CsvExport.h      src/core/CsvExport.cpp
    src/core/Goal.h           src/core/Goal.cpp
    src/core/ExchangeRates.h  src/core/ExchangeRates.cpp
    src/core/Reports.h        src/core/Reports.cpp
)
target_include_directories(finora_core PUBLIC src)
target_compile_options(finora_core PRIVATE ${FINORA_WARNINGS})

# =====================================================================
#  2. finora_database - every SQL statement in Finora lives here,
#     together with the lock that lets only one copy of Finora use
#     the database at a time.
#     Qt SQL is linked PRIVATE: code that uses this library does not
#     see any SQL classes, only DatabaseManager, BudgetStore, UserStore,
#     SubscriptionStore, GoalStore and InstanceLock.
# =====================================================================
add_library(finora_database STATIC
    src/database/DatabaseManager.h
    src/database/DatabaseManager.cpp
    src/database/InstanceLock.h
    src/database/InstanceLock.cpp
    src/database/BudgetStore.h
    src/database/BudgetStore.cpp
    src/database/UserStore.h
    src/database/UserStore.cpp
    src/database/SubscriptionStore.h
    src/database/SubscriptionStore.cpp
    src/database/GoalStore.h
    src/database/GoalStore.cpp
)
target_include_directories(finora_database PUBLIC src)
target_link_libraries(finora_database
    PUBLIC  finora_core Qt6::Core
    PRIVATE Qt6::Sql)
target_compile_options(finora_database PRIVATE ${FINORA_WARNINGS})

# =====================================================================
#  libsodium 1.0.22 - Argon2id password hashing and secure random numbers.
#  The official, signature-verified MinGW build is kept UNCHANGED in
#  third_party/libsodium-1.0.22 (see VERIFICATION.md there). Finora links
#  the DLL through its import library. Do not replace it without approval.
# =====================================================================
set(FINORA_SODIUM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsodium-1.0.22")
add_library(finora_sodium SHARED IMPORTED)
set_target_properties(finora_sodium PROPERTIES
    IMPORTED_LOCATION "${FINORA_SODIUM_DIR}/bin/libsodium-26.dll"
    IMPORTED_IMPLIB "${FINORA_SODIUM_DIR}/lib/libsodium.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "${FINORA_SODIUM_DIR}/include")

# windeployqt copies only Qt's own files, so every program that uses
# libsodium gets libsodium-26.dll copied next to it after each build.
function(finora_copy_sodium_dll target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FINORA_SODIUM_DIR}/bin/libsodium-26.dll" "$<TARGET_FILE_DIR:${target}>"
        COMMENT "Copying libsodium-26.dll next to ${target}")
endfunction()

# =====================================================================
#  2b. finora_auth - accounts: password hashing, sign-up, sign-in,
#      sessions and recovery codes. Only this library sees libsodium.
# =====================================================================
add_library(finora_auth STATIC
    src/auth/PasswordHasher.h   src/auth/PasswordHasher.cpp
    src/auth/AuthService.h      src/auth/AuthService.cpp
)
target_include_directories(finora_auth PUBLIC src)
target_link_libraries(finora_auth
    PUBLIC  finora_database finora_core
    PRIVATE finora_sodium)
target_compile_options(finora_auth PRIVATE ${FINORA_WARNINGS})

# =====================================================================
#  2c. finora_billing - subscriptions (Phase 4C): the payment gateway
#      interface, the SIMULATED gateway (no real money, no secret keys,
#      no card details) and the subscription lifecycle.
# =====================================================================
add_library(finora_billing STATIC
    src/billing/PaymentGateway.h
    src/billing/SimulatedPaymentGateway.h  src/billing/SimulatedPaymentGateway.cpp
    src/billing/SubscriptionService.h      src/billing/SubscriptionService.cpp
)
target_include_directories(finora_billing PUBLIC src)
target_link_libraries(finora_billing PUBLIC finora_database finora_core Qt6::Core)
target_compile_options(finora_billing PRIVATE ${FINORA_WARNINGS})

# =====================================================================
#  2d. finora_reports - files made from a person's records (Phase 4C):
#      the CSV export (Finora Plus) and the monthly PDF summary (Finora
#      Pro). QPdfWriter and QTextDocument are part of Qt GUI, which
#      Finora already uses, so no new library is needed.
# =====================================================================
add_library(finora_reports STATIC
    src/reports/ReportExporter.h  src/reports/ReportExporter.cpp
)
target_include_directories(finora_reports PUBLIC src)
target_link_libraries(finora_reports PUBLIC finora_database finora_core Qt6::Gui)
target_compile_options(finora_reports PRIVATE ${FINORA_WARNINGS})

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
    src/ui/TransactionsPage.h   src/ui/TransactionsPage.cpp
    src/ui/TransactionDialog.h  src/ui/TransactionDialog.cpp
    src/ui/Page.h
    src/ui/UiHelpers.h        src/ui/UiHelpers.cpp
    src/ui/DashboardPage.h    src/ui/DashboardPage.cpp
    src/ui/BudgetsPage.h      src/ui/BudgetsPage.cpp
    src/ui/BudgetDialog.h     src/ui/BudgetDialog.cpp
    src/ui/SettingsPage.h     src/ui/SettingsPage.cpp
    src/ui/components/Toast.h           src/ui/components/Toast.cpp
    src/ui/components/MonthSwitcher.h   src/ui/components/MonthSwitcher.cpp
    src/ui/components/ProgressRing.h    src/ui/components/ProgressRing.cpp
    src/ui/components/WalletCard.h      src/ui/components/WalletCard.cpp
    src/ui/components/MoneyFlowChart.h  src/ui/components/MoneyFlowChart.cpp
    src/ui/components/PasswordField.h   src/ui/components/PasswordField.cpp
    src/ui/components/RecoveryCodesPanel.h  src/ui/components/RecoveryCodesPanel.cpp
    src/ui/AuthWindow.h       src/ui/AuthWindow.cpp
    src/ui/AccountDialogs.h   src/ui/AccountDialogs.cpp
    src/ui/AppController.h    src/ui/AppController.cpp
    src/ui/SubscriptionPage.h src/ui/SubscriptionPage.cpp
    src/ui/CheckoutDialog.h   src/ui/CheckoutDialog.cpp
    src/ui/AuditHistoryDialog.h  src/ui/AuditHistoryDialog.cpp
    src/ui/GoalsPage.h        src/ui/GoalsPage.cpp
    src/ui/GoalDialogs.h      src/ui/GoalDialogs.cpp
    src/ui/CurrencyPage.h     src/ui/CurrencyPage.cpp
    src/ui/DatabaseStartup.h  src/ui/DatabaseStartup.cpp
    src/ui/ReportsPage.h      src/ui/ReportsPage.cpp
)

# Icons, fonts (with their licences) and the style sheet. They are built
# into the .exe file, so the program never depends on loose files.
# The icons draw with "currentColor"; Theme.cpp colours them at run time.
set(FINORA_RESOURCE_FILES
    resources/icons/logo.svg
    resources/icons/dashboard.svg      resources/icons/transactions.svg
    resources/icons/budgets.svg        resources/icons/goals.svg
    resources/icons/reports.svg        resources/icons/currency.svg
    resources/icons/settings.svg
    resources/icons/chevron_left.svg   resources/icons/chevron_right.svg
    resources/icons/chevron_down_light.svg  resources/icons/chevron_down_dark.svg
    resources/icons/check_circle.svg   resources/icons/check_white.svg
    resources/icons/moon.svg           resources/icons/sun.svg
    resources/icons/eye.svg            resources/icons/eye_off.svg
    resources/icons/log_out.svg        resources/icons/subscription.svg
    resources/icons/lock.svg
    resources/fonts/Inter-Regular.ttf  resources/fonts/Inter-Medium.ttf
    resources/fonts/Inter-SemiBold.ttf resources/fonts/Inter-Bold.ttf
    resources/fonts/Inter-OFL.txt
    resources/fonts/Manrope-SemiBold.ttf  resources/fonts/Manrope-Bold.ttf
    resources/fonts/Manrope-ExtraBold.ttf resources/fonts/Manrope-OFL.txt
    resources/styles/finora.qss
)

qt_add_executable(Finora WIN32
    src/main.cpp
    ${FINORA_UI_SOURCES}
    resources/finora.rc   # the .exe icon: Explorer, shortcuts and pinned taskbar buttons use it
)
qt_add_resources(Finora "finora_resources"
    PREFIX "/"
    BASE resources
    FILES ${FINORA_RESOURCE_FILES}
)
# libsodium's licence text, shown under Settings > View licences.
qt_add_resources(Finora "finora_licences"
    PREFIX "/licences/libsodium"
    BASE third_party/libsodium-1.0.22
    FILES third_party/libsodium-1.0.22/LICENSE
)
target_include_directories(Finora PRIVATE src)
target_link_libraries(Finora PRIVATE finora_core finora_database finora_auth finora_billing finora_reports
                                     Qt6::Widgets Qt6::Svg)
target_compile_options(Finora PRIVATE ${FINORA_WARNINGS})
# The version number above is passed to the C++ code as FINORA_VERSION.
target_compile_definitions(Finora PRIVATE FINORA_VERSION="${PROJECT_VERSION}")
finora_copy_sodium_dll(Finora)

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
qt_add_resources(finora_ui_tests "finora_ui_test_licences"
    PREFIX "/licences/libsodium"
    BASE third_party/libsodium-1.0.22
    FILES third_party/libsodium-1.0.22/LICENSE
)
target_include_directories(finora_ui_tests PRIVATE src)
# Qt SQL lets the start-up test prepare an older data file and interrupt an upgrade.
target_link_libraries(finora_ui_tests PRIVATE finora_core finora_database finora_auth finora_billing
                                              finora_reports Qt6::Widgets Qt6::Svg Qt6::Sql)
target_compile_options(finora_ui_tests PRIVATE ${FINORA_WARNINGS})
target_compile_definitions(finora_ui_tests PRIVATE FINORA_VERSION="${PROJECT_VERSION}")
finora_copy_sodium_dll(finora_ui_tests)
add_test(NAME ui_tests COMMAND finora_ui_tests)
# "offscreen" draws windows in memory, so nothing pops up during tests.
# TIMEOUT stops the test after 120 s if a pop-up window ever waits forever.
set_tests_properties(ui_tests PROPERTIES
    TIMEOUT 120
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=${FINORA_QT_PLUGINS_DIR}")

# 4c. Database tests: real SQLite files inside a temporary folder.
add_executable(finora_db_tests tests/finora_db_tests.cpp)
target_link_libraries(finora_db_tests PRIVATE finora_database finora_core Qt6::Core Qt6::Sql)
target_compile_options(finora_db_tests PRIVATE ${FINORA_WARNINGS})
add_test(NAME database_tests COMMAND finora_db_tests)
set_tests_properties(database_tests PROPERTIES
    ENVIRONMENT "QT_PLUGIN_PATH=${FINORA_QT_PLUGINS_DIR}")

# 4d. Account and security tests: Argon2id, sign-up, sign-in, sessions,
#     recovery codes and one person's data per account. psapi reads the
#     process's memory use, to prove the 256 MiB Argon2id setting.
add_executable(finora_auth_tests tests/finora_auth_tests.cpp)
target_link_libraries(finora_auth_tests PRIVATE finora_auth finora_database finora_core
                                                Qt6::Core Qt6::Sql psapi)
target_compile_options(finora_auth_tests PRIVATE ${FINORA_WARNINGS})
finora_copy_sodium_dll(finora_auth_tests)
add_test(NAME auth_tests COMMAND finora_auth_tests)
set_tests_properties(auth_tests PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "QT_PLUGIN_PATH=${FINORA_QT_PLUGINS_DIR}")

# 4e. Subscription and billing tests (Phase 4C): the upgrade to schema
#     version 3, plans, entitlements, the simulated checkout and the
#     billing history, on real SQLite files in a temporary folder.
add_executable(finora_billing_tests tests/finora_billing_tests.cpp)
target_link_libraries(finora_billing_tests PRIVATE finora_billing finora_reports finora_auth finora_database
                                                   finora_core Qt6::Core Qt6::Gui Qt6::Sql)
target_compile_options(finora_billing_tests PRIVATE ${FINORA_WARNINGS})
finora_copy_sodium_dll(finora_billing_tests)
add_test(NAME billing_tests COMMAND finora_billing_tests)
# The PDF test needs fonts, so the program is a QGuiApplication; "offscreen"
# lets it run without a screen.
set_tests_properties(billing_tests PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=${FINORA_QT_PLUGINS_DIR}")
