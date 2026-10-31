// =====================================================================
//  UiHelpers.h
//  Small functions that several pages share: month names, money text,
//  chips and emptying a layout.
// =====================================================================
#ifndef FINORA_UIHELPERS_H
#define FINORA_UIHELPERS_H

#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <string>

#include "core/Subscription.h"

// Today's month as "YYYY-MM", for example "2026-10".
QString currentMonth();

// monthTitle("2026-10") gives "October 2026"; shortMonthName("2026-10") gives "Oct".
QString monthTitle(const QString &month);
QString shortMonthName(const QString &month);

// "1 transaction" or "3 transactions": the number with the right word.
QString countText(int count, const QString &singular, const QString &plural);

// formatMoney() from core/Money.h as a QString: "PKR 1,200.50".
QString moneyText(const std::string &currencyCode, long long amountMinor);

// A small rounded label such as "Over budget" coloured by `tone`
// (success, warning, danger, info or neutral; see finora.qss).
QLabel *createChip(const QString &text, const QString &tone, QWidget *parent);

// Removes and deletes every widget and spacing inside `layout`, so that
// a list (for example the budget alerts) can be built again.
void clearLayout(QLayout *layout);

// ---- Subscriptions (Phase 4C) ----

// The time now in UTC, as the database stores it: "2026-10-31T09:30:00Z".
std::string currentUtcTime();

// A stored UTC time as a local date for the screen: "30 Nov 2026".
// Returns "" for an empty or unreadable time.
QString dateText(const std::string &utcTime);

// Whole days from now until a stored UTC time, a part of a day counted
// as a whole day; 0 if the time has passed.
int daysUntil(const std::string &utcTime);

// A plan's name without "Finora ": "Pro".
QString shortPlanName(PlanId plan);

// The plan under the person's name in the sidebar, for example
// "Finora Free", "Pro trial" or "Plus · payment due", and the tone
// that colours it (info, warning or neutral; see finora.qss).
QString planChipText(const Subscription &subscription);
QString planChipTone(const Subscription &subscription);

// Explains a feature of a larger plan and offers the plans. "Not now" is
// the default button, so pressing Enter never leaves the page by
// accident. Returns true if the person chose "See plans".
bool askToSeePlans(QWidget *parent, const QString &title, const QString &explanation);

// Shows a small lock on a button whose feature the current plan does not
// include, and removes it again when the plan includes it. The button
// still works: clicking it explains the plans.
void showLockIfNeeded(QPushButton *button, bool locked, const QString &planNeeded);

#endif // FINORA_UIHELPERS_H
