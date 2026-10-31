// =====================================================================
//  AuditHistoryDialog.h
//  The account's audit history (Finora Business, Phase 4C): sign-ins,
//  password changes, plan changes, payments and exports, newest first.
//  Only the signed-in person's own lines are shown.
// =====================================================================
#ifndef FINORA_AUDITHISTORYDIALOG_H
#define FINORA_AUDITHISTORYDIALOG_H

#include <QDialog>
#include <QTableWidget>

#include "database/DatabaseManager.h"

class AuditHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    // Loads the history at once. DatabaseManager::loadAuditHistory checks
    // the plan again, so the dialog shows nothing on a smaller plan.
    AuditHistoryDialog(DatabaseManager &sharedDatabase, QWidget *parent = nullptr);

    // How many lines are shown, for the tests.
    int shownEventCount() const;

private:
    QTableWidget *table = nullptr;
    int eventCount = 0;
};

// "plan_activated" as "Plan activated", for the screen.
QString readableEventName(const std::string &eventType);

#endif // FINORA_AUDITHISTORYDIALOG_H
