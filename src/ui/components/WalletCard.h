// =====================================================================
//  WalletCard.h
//  One coloured card per currency on the dashboard: the balance and
//  this month's income and spending.
// =====================================================================
#ifndef FINORA_WALLETCARD_H
#define FINORA_WALLETCARD_H

#include <QFrame>
#include <QLabel>

#include "core/Totals.h"
#include "core/Transaction.h"

class WalletCard : public QFrame
{
    Q_OBJECT

public:
    WalletCard(const CurrencyInfo &currency, QWidget *parent = nullptr);

    // Shows the totals of this card's currency. monthName is for
    // example "October", used in the captions.
    void showSummary(const CurrencySummary &summary, const QString &monthName);

    QString currencyCode() const;
    QString balanceText() const;
    QString incomeText() const;
    QString expenseText() const;

private:
    QString code;
    QLabel *balanceLabel = nullptr;
    QLabel *incomeLabel = nullptr;
    QLabel *expenseLabel = nullptr;
    QLabel *incomeCaption = nullptr;
    QLabel *expenseCaption = nullptr;
    QLabel *countLabel = nullptr;
};

#endif // FINORA_WALLETCARD_H
