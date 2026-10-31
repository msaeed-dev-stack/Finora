// =====================================================================
//  CurrencyPage.h
//  The Currency page (Milestone 5): the exchange rates the person
//  enters, a converter, and an estimate of all balances in one currency.
//  Finora never downloads rates, and it never changes a record's
//  currency: every conversion here is only shown, never saved.
// =====================================================================
#ifndef FINORA_CURRENCYPAGE_H
#define FINORA_CURRENCYPAGE_H

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>

#include <string>
#include <vector>

#include "Page.h"
#include "core/ExchangeRates.h"
#include "core/Transaction.h"
#include "database/DatabaseManager.h"

class CurrencyPage : public Page
{
    Q_OBJECT

public:
    CurrencyPage(DatabaseManager &sharedDatabase, QWidget *parent = nullptr);

    // Loads the saved rates and the transactions again (Page).
    void refresh() override;

    // Reads both rate fields and saves them. An empty field means "no
    // rate". Returns false and shows the problem under the fields if a
    // rate is not valid. Public so the tests can call it.
    bool saveRates();

    // What the page shows, for the tests.
    QString conversionText() const;
    QString totalText() const;

private slots:
    void onSaveRatesClicked();
    void updateConversion();
    void updateBalances();

private:
    QWidget *createHeader();
    QFrame *createRatesCard();
    QFrame *createConverterCard();
    QFrame *createBalancesCard();
    QComboBox *createCurrencyBox(const QString &objectName, QWidget *parent);
    bool readRateField(QLineEdit *field, const QString &code, long long &rate, QString &problem);
    QString rateLine(const std::string &currencyCode) const;
    void showRates();

    DatabaseManager &database;   // created in main(); this page only uses it
    ExchangeRates rates;         // the saved rates (0 = not entered)
    std::vector<Transaction> transactions;

    QLineEdit *usdRateEdit = nullptr;
    QLineEdit *eurRateEdit = nullptr;
    QLabel *ratesStatus = nullptr;   // "Saved on 31 Oct 2026" or "No rates saved yet"
    QLabel *ratesError = nullptr;

    QLineEdit *amountEdit = nullptr;
    QComboBox *fromBox = nullptr;
    QComboBox *toBox = nullptr;
    QLabel *resultLabel = nullptr;
    QLabel *resultNote = nullptr;

    QComboBox *totalCurrencyBox = nullptr;
    QWidget *balanceArea = nullptr;
    QGridLayout *balanceGrid = nullptr;
    QLabel *totalLabel = nullptr;
    QLabel *totalNote = nullptr;
};

#endif // FINORA_CURRENCYPAGE_H
