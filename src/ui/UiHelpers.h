// =====================================================================
//  UiHelpers.h
//  Small functions that several pages share: month names, money text,
//  chips and emptying a layout.
// =====================================================================
#ifndef FINORA_UIHELPERS_H
#define FINORA_UIHELPERS_H

#include <QLabel>
#include <QLayout>
#include <QString>

#include <string>

// Today's month as "YYYY-MM", for example "2026-10".
QString currentMonth();

// monthTitle("2026-10") gives "October 2026"; shortMonthName("2026-10") gives "Oct".
QString monthTitle(const QString &month);
QString shortMonthName(const QString &month);

// formatMoney() from core/Money.h as a QString: "PKR 1,200.50".
QString moneyText(const std::string &currencyCode, long long amountMinor);

// A small rounded label such as "Over budget" coloured by `tone`
// (success, warning, danger, info or neutral; see finora.qss).
QLabel *createChip(const QString &text, const QString &tone, QWidget *parent);

// Removes and deletes every widget and spacing inside `layout`, so that
// a list (for example the budget alerts) can be built again.
void clearLayout(QLayout *layout);

#endif // FINORA_UIHELPERS_H
