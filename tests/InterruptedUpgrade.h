// =====================================================================
//  InterruptedUpgrade.h  (used only by the test programs)
//  Leaves a database file exactly as a crash or a power cut in the
//  middle of an upgrade would leave it, for the Milestone 5 regression
//  tests of the start-up question.
//
//  How: the test program starts a second copy of ITSELF with
//      --interrupt-upgrade <file>
//  That copy (runInterruptedUpgrade) begins a transaction, makes
//  upgrade-like changes and prints "ready". The test then kills it
//  before COMMIT, so SQLite's journal is left beside the file (a "hot
//  journal"). SQLite uses it to undo the changes the next time the file
//  is opened for writing.
// =====================================================================
#ifndef FINORA_INTERRUPTEDUPGRADE_H
#define FINORA_INTERRUPTEDUPGRADE_H

#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QStringList>

#include <iostream>
#include <string>

// The second copy: changes the file inside a transaction that never ends.
inline int runInterruptedUpgrade(const QString &path)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "interrupted_upgrade");
    db.setDatabaseName(path);
    if (!db.open()) {
        return 4;
    }
    QSqlQuery query(db);
    // A tiny cache makes SQLite write changed pages into the file itself
    // before COMMIT, as it does during a large upgrade.
    bool ok = query.exec("PRAGMA cache_size = 10") && query.exec("BEGIN")
              && query.exec("CREATE TABLE goals (id INTEGER PRIMARY KEY)")
              && query.exec("PRAGMA user_version = 4")
              && query.exec("UPDATE transactions SET description = 'changed by the interrupted upgrade'")
              && query.exec("CREATE TABLE interrupted_filler (x TEXT)");
    for (int i = 0; ok && i < 3000; i++) {
        ok = query.exec("INSERT INTO interrupted_filler VALUES (hex(randomblob(500)))");
    }
    if (!ok) {
        return 5;
    }
    std::cout << "ready" << std::endl;   // the test kills this program now
    std::string line;
    std::getline(std::cin, line);        // waits; the transaction stays open
    return 6;                            // reached only if the test did not kill it
}

// The test's side: starts the second copy and kills it in the middle of
// its transaction, like a crash. Returns true if the journal was left.
inline bool interruptUpgradeOf(const QString &path)
{
    QProcess copy;
    copy.start(QCoreApplication::applicationFilePath(), QStringList{"--interrupt-upgrade", path});
    bool ready = copy.waitForStarted(30000) && copy.waitForReadyRead(120000)
                 && copy.readAllStandardOutput().contains("ready");
    copy.kill();   // no COMMIT and no clean close
    copy.waitForFinished(30000);
    return ready && QFile::exists(path + "-journal");
}

#endif // FINORA_INTERRUPTEDUPGRADE_H
