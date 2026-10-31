// =====================================================================
//  DatabaseStartup.h
//  Opening Finora's database when the program starts. A file from an
//  older Finora is upgraded only after the person agrees. (Moved here
//  from main.cpp in Milestone 5, so the tests can run the same steps.)
// =====================================================================
#ifndef FINORA_DATABASESTARTUP_H
#define FINORA_DATABASESTARTUP_H

#include <QString>

#include "database/DatabaseManager.h"

// What happened when the database was opened at start-up.
enum StartupResult {
    STARTUP_OPENED,     // the database is open and up to date
    STARTUP_DECLINED,   // the person chose "Quit without changes": nothing was upgraded
    STARTUP_FAILED      // it could not be opened; database.lastError() says why
};

// What an upgrade from `storedVersion` adds, in words for the question.
QString upgradeChangesText(int storedVersion);

// Asks before a database from an older Finora is upgraded. Returns true
// if the person chose "Upgrade and continue".
bool askToUpgrade(const QString &databasePath, int storedVersion);

// Opens `databasePath` with `database`, asking first if it must be upgraded:
//   1. The layout is read without changing the file. If it is older,
//      the person is asked before anything else happens.
//   2. The file is opened. Without the person's answer, open() never
//      upgrades a file that holds records.
//   3. If step 1 could not read the layout, open() refuses in step 2. This
//      happens after an interrupted upgrade (a crash or a power cut):
//      SQLite only restores such a file when it is opened for writing, as
//      in step 2. Now the layout can be read, so the person is asked, and
//      the file is opened again only after "Upgrade and continue".
StartupResult openDatabaseAtStartup(DatabaseManager &database, const QString &databasePath);

#endif // FINORA_DATABASESTARTUP_H
