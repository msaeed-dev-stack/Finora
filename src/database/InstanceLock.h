// =====================================================================
//  InstanceLock.h
//  Makes sure only one copy of Finora uses the database at a time.
//
//  SQLite lets only one program write to the file at once. If two
//  copies of Finora saved at the same moment, one save could wait for
//  seconds or be rejected with "database is locked". So before opening
//  the database, main() takes this lock. A second copy finds the lock
//  already taken, tells the user that Finora is already open, and exits.
// =====================================================================
#ifndef FINORA_INSTANCELOCK_H
#define FINORA_INSTANCELOCK_H

#include <QLockFile>
#include <QString>

// Holds one lock file, for example .../Finora/finora.lock.
//
// Lifetime (RAII): tryLock() takes the lock, and the destructor gives
// it back by deleting the lock file, so the lock is released even when
// main() returns early. If Finora crashes, the file stays behind, but
// the next start sees that the program which wrote it is no longer
// running and takes the lock over.
class InstanceLock
{
public:
    explicit InstanceLock(const QString &lockPath);

    // A lock must have exactly one owner, so copying is not allowed.
    InstanceLock(const InstanceLock &) = delete;
    InstanceLock &operator=(const InstanceLock &) = delete;

    // The lock file that belongs to a database: finora.lock in the same
    // folder. Returns an empty string if databasePath is empty.
    static QString lockPathFor(const QString &databasePath);

    // Tries once, without waiting. Returns true if this object now holds
    // the lock. Returns false and sets lastError() otherwise.
    bool tryLock();

    // True if tryLock() failed because another running copy holds the lock.
    // False for other problems, such as a folder that cannot be written.
    bool heldByAnotherCopy() const;

    bool isLocked() const;
    QString lockPath() const;
    QString lastError() const;

private:
    QString path;           // full path of the lock file
    QLockFile lockFile;     // Qt's lock file; its destructor releases the lock
    QString errorMessage;   // explains the most recent failure
    bool otherCopy = false; // true if the last failure was "already running"
};

#endif // FINORA_INSTANCELOCK_H
