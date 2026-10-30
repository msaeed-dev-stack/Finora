// =====================================================================
//  WalletCard.cpp
// =====================================================================
#include "WalletCard.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "core/Money.h"
#include "ui/UiHelpers.h"

WalletCard::WalletCard(const CurrencyInfo &currency, QWidget *parent)
    : QFrame(parent), code(currency.code)
{
    setObjectName("walletCard");
    // The style sheet picks this card's gradient from the property:
    // QFrame#walletCard[currency="PKR"] is green, USD blue, EUR violet.
    setProperty("currency", code);
    setMinimumHeight(184);

    QLabel *badge = new QLabel(QString::fromUtf8(currency.symbol), this);
    badge->setObjectName("walletBadge");
    badge->setFixedSize(36, 36);
    badge->setAlignment(Qt::AlignCenter);

    QLabel *codeLabel = new QLabel(code, this);
    codeLabel->setObjectName("walletCode");
    QLabel *nameLabel = new QLabel(QString::fromUtf8(currency.name), this);
    nameLabel->setObjectName("walletName");

    countLabel = new QLabel(this);
    countLabel->setObjectName("walletCaption");
    countLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);

    QLabel *balanceCaption = new QLabel("Balance", this);
    balanceCaption->setObjectName("walletCaption");
    balanceLabel = new QLabel(this);
    balanceLabel->setObjectName("walletBalance");

    QFrame *divider = new QFrame(this);
    divider->setObjectName("walletDivider");

    incomeCaption = new QLabel(this);
    incomeCaption->setObjectName("walletCaption");
    incomeLabel = new QLabel(this);
    incomeLabel->setObjectName("walletStat");
    expenseCaption = new QLabel(this);
    expenseCaption->setObjectName("walletCaption");
    expenseLabel = new QLabel(this);
    expenseLabel->setObjectName("walletStat");

    // Top row: badge, code and name, transaction count.
    QVBoxLayout *nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(0);
    nameLayout->addWidget(codeLabel);
    nameLayout->addWidget(nameLabel);

    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(10);
    topRow->addWidget(badge);
    topRow->addLayout(nameLayout);
    topRow->addStretch();
    topRow->addWidget(countLabel);

    // Bottom row: income and spending of the month, side by side.
    QGridLayout *statGrid = new QGridLayout();
    statGrid->setHorizontalSpacing(16);
    statGrid->setVerticalSpacing(2);
    statGrid->addWidget(incomeCaption, 0, 0);
    statGrid->addWidget(incomeLabel, 1, 0);
    statGrid->addWidget(expenseCaption, 0, 1);
    statGrid->addWidget(expenseLabel, 1, 1);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(4);
    layout->addLayout(topRow);
    layout->addSpacing(10);
    layout->addWidget(balanceCaption);
    layout->addWidget(balanceLabel);
    layout->addSpacing(8);
    layout->addWidget(divider);
    layout->addSpacing(6);
    layout->addLayout(statGrid);
}

void WalletCard::showSummary(const CurrencySummary &summary, const QString &monthName)
{
    // Up and down arrows repeat the meaning of the words, so the card
    // never relies on colour alone.
    incomeCaption->setText(QString::fromUtf8("↑ Income in ") + monthName);
    expenseCaption->setText(QString::fromUtf8("↓ Spent in ") + monthName);

    if (summary.overflow) {
        balanceLabel->setText("Too large to show");
        incomeLabel->setText("-");
        expenseLabel->setText("-");
    } else {
        balanceLabel->setText(moneyText(code.toStdString(), balanceOf(summary)));
        incomeLabel->setText(moneyText(code.toStdString(), summary.monthIncomeMinor));
        expenseLabel->setText(moneyText(code.toStdString(), summary.monthExpenseMinor));
    }

    if (summary.transactionCount == 1) {
        countLabel->setText("1 transaction");
    } else {
        countLabel->setText(QString::number(summary.transactionCount) + " transactions");
    }

    // A screen reader reads the whole card as one sentence.
    setAccessibleName(code + " wallet");
    setAccessibleDescription("Balance " + balanceLabel->text() + ". " + incomeCaption->text() + ": "
                             + incomeLabel->text() + ". " + expenseCaption->text() + ": "
                             + expenseLabel->text() + ".");
}

QString WalletCard::currencyCode() const
{
    return code;
}

QString WalletCard::balanceText() const
{
    return balanceLabel->text();
}

QString WalletCard::incomeText() const
{
    return incomeLabel->text();
}

QString WalletCard::expenseText() const
{
    return expenseLabel->text();
}
