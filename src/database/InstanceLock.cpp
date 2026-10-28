// =====================================================================
//  InstanceLock.cpp
// =====================================================================
#include "InstanceLock.h"

#include <QDir>
#include <QFileInfo>

InstanceLock::InstanceLock(const QString &lockPath)
    : path(lockPath), lockFile(lockPath)
{
    // By default Qt treats a lock file older than 30 seconds as abandoned
    // and lets another program take it. Finora stays open for hours, so
    // that rule is switched off. A lock is then taken over only when the
    // program that wrote it is no longer running, for example after a crash.
    lockFile.setStaleLockTime(0);
}

QString InstanceLock::lockPathFor(const QString &databasePath)
{
    if (databasePath.isEmpty()) {
        return QString();
    }
    return QFileInfo(databasePath).absolutePath() + "/finora.lock";
}

bool InstanceLock::tryLock()
{
    errorMessage.clear();
    otherCopy = false;

    if (path.isEmpty()) {
        errorMessage = "No lock file was given.";
        return false;
    }
    if (lockFile.isLocked()) {
        return true;   // this object already holds the lock
    }

    // On the very first start the data folder does not exist yet.
    QString folder = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(folder)) {
        errorMessage = "Could not create the folder " + QDir::toNativeSeparators(folder) + ".";
        return false;
    }

    // A timeout of 0 means: try once and do not wait.
    if (lockFile.tryLock(0)) {
        return true;
    }

    switch (lockFile.error()) {
    case QLockFile::LockFailedError:
        otherCopy = true;
        errorMessage = "Another copy of Finora is already running.";
        break;
    case QLockFile::PermissionError:
        errorMessage = "Finora has no permission to create the lock file "
                       + QDir::toNativeSeparators(path) + ".";
        break;
    default:
        errorMessage = "Could not create the lock file " + QDir::toNativeSeparators(path) + ".";
        break;
    }
    return false;
}

bool InstanceLock::heldByAnotherCopy() const
{
    return otherCopy;
}

bool InstanceLock::isLocked() const
{
    return lockFile.isLocked();
}

QString InstanceLock::lockPath() const
{
    return path;
}

QString InstanceLock::lastError() const
{
    return errorMessage;
}
