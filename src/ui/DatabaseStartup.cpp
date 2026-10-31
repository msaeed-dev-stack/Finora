// =====================================================================
//  DatabaseStartup.cpp
// =====================================================================
#include "DatabaseStartup.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

// True for a layout from an older Finora: one that must be upgraded.
bool isOlderLayout(int storedVersion)
{
    return storedVersion >= 1 && storedVersion < CURRENT_SCHEMA_VERSION;
}

QString upgradeChangesText(int storedVersion)
{
    if (storedVersion == 1) {
        return "adds budgets, user accounts, subscription plans and savings goals";
    }
    if (storedVersion == 2) {
        return "adds subscription plans, a billing history and savings goals";
    }
    return "adds savings goals";
}

// The person decides; "Quit" closes Finora and leaves the file exactly as it is.
bool askToUpgrade(const QString &databasePath, int storedVersion)
{
    QString backupFolder = QDir::toNativeSeparators(QFileInfo(databasePath).absolutePath() + "/backups");

    QMessageBox box;
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle("Upgrade Finora's data file");
    box.setText("This version of Finora stores your records in a new layout.");
    box.setInformativeText(
        "Your data file uses layout " + QString::number(storedVersion) + ". Finora will upgrade it to layout "
        + QString::number(CURRENT_SCHEMA_VERSION) + ", which " + upgradeChangesText(storedVersion) + ". "
        "Every record and every account is kept.\n\n"
        "Before anything changes, a complete backup copy is saved in:\n" + backupFolder + "\n\n"
        "If any step fails, the upgrade is undone and your file stays exactly as it is.");
    QPushButton *upgradeButton = box.addButton("Upgrade and continue", QMessageBox::AcceptRole);
    box.addButton("Quit without changes", QMessageBox::RejectRole);
    box.setDefaultButton(upgradeButton);
    box.exec();

    return box.clickedButton() == upgradeButton;
}

StartupResult openDatabaseAtStartup(DatabaseManager &database, const QString &databasePath)
{
    // 1. A file from an older Finora: ask before anything else happens.
    int storedVersion = DatabaseManager::storedSchemaVersion(databasePath);
    bool agreed = false;
    if (isOlderLayout(storedVersion)) {
        if (!askToUpgrade(databasePath, storedVersion)) {
            return STARTUP_DECLINED;   // nothing was changed
        }
        agreed = true;
    }

    // 2. Open. Without the person's answer, open() refuses to upgrade.
    database.setUpgradeAllowed(agreed);
    if (database.open(databasePath)) {
        return STARTUP_OPENED;
    }
    if (!database.upgradeNeeded()) {
        return STARTUP_FAILED;
    }

    // 3. The layout could not be read in step 1. Opening the file has let
    //    SQLite restore it to its last saved state, so it can be read now.
    storedVersion = DatabaseManager::storedSchemaVersion(databasePath);
    if (!isOlderLayout(storedVersion)) {
        return STARTUP_FAILED;   // database.lastError() explains the refusal
    }
    if (!askToUpgrade(databasePath, storedVersion)) {
        return STARTUP_DECLINED;
    }
    database.setUpgradeAllowed(true);
    if (database.open(databasePath)) {
        return STARTUP_OPENED;
    }
    return STARTUP_FAILED;
}
