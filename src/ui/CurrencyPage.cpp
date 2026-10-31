// =====================================================================
//  CurrencyPage.cpp
// =====================================================================
#include "CurrencyPage.h"

#include <QDate>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QVBoxLayout>

#include "PageHeader.h"
#include "UiHelpers.h"
#include "components/Toast.h"
#include "core/Money.h"
#include "core/Totals.h"

CurrencyPage::CurrencyPage(DatabaseManager &sharedDatabase, QWidget *parent)
    : Page(parent), database(sharedDatabase)
{
    setObjectName("currencyPage");

    QWidget *content = new QWidget(this);
    content->setObjectName("pageContent");

    // The converter and the balances side by side, under the rates.
    QHBoxLayout *lowerRow = new QHBoxLayout();
    lowerRow->setSpacing(16);
    lowerRow->addWidget(createConverterCard(), 1);
    lowerRow->addWidget(createBalancesCard(), 1);

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(20);
    contentLayout->addWidget(createHeader());
    contentLayout->addWidget(createRatesCard());
    contentLayout->addLayout(lowerRow);
    contentLayout->addStretch();

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setObjectName("pageScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    refresh();
}

QWidget *CurrencyPage::createHeader()
{
    return new PageHeader("Currency", "Your exchange rates for PKR, USD and EUR", this);
}

// PKR, USD and EUR, with the code as the hidden data of each item.
QComboBox *CurrencyPage::createCurrencyBox(const QString &objectName, QWidget *parent)
{
    QComboBox *box = new QComboBox(parent);
    box->setObjectName(objectName);
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        QString code = QString::fromUtf8(SUPPORTED_CURRENCIES[i].code);
        box->addItem(code + "  " + QString::fromUtf8(SUPPORTED_CURRENCIES[i].name), code);
    }
    return box;
}

QFrame *CurrencyPage::createRatesCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    QLabel *title = new QLabel("Your exchange rates", card);
    title->setObjectName("cardTitle");
    QLabel *explanation = new QLabel(
        "Finora never downloads exchange rates. Enter today's rates from your bank or another source you "
        "trust. They are used only for the estimates on this page: your records always stay in their own "
        "currency.", card);
    explanation->setObjectName("mutedText");
    explanation->setWordWrap(true);

    usdRateEdit = new QLineEdit(card);
    usdRateEdit->setObjectName("usdRateEdit");
    usdRateEdit->setPlaceholderText("For example 278.50");
    usdRateEdit->setAccessibleName("Pakistani Rupees for 1 US Dollar");
    eurRateEdit = new QLineEdit(card);
    eurRateEdit->setObjectName("eurRateEdit");
    eurRateEdit->setPlaceholderText("For example 301.20");
    eurRateEdit->setAccessibleName("Pakistani Rupees for 1 Euro");

    QGridLayout *fields = new QGridLayout();
    fields->setHorizontalSpacing(10);
    fields->setVerticalSpacing(8);
    fields->addWidget(new QLabel("1 USD =", card), 0, 0);
    fields->addWidget(usdRateEdit, 0, 1);
    fields->addWidget(new QLabel("PKR", card), 0, 2);
    fields->addWidget(new QLabel("1 EUR =", card), 1, 0);
    fields->addWidget(eurRateEdit, 1, 1);
    fields->addWidget(new QLabel("PKR", card), 1, 2);
    fields->setColumnStretch(3, 1);

    ratesStatus = new QLabel(card);
    ratesStatus->setObjectName("mutedText");
    ratesError = new QLabel(card);
    ratesError->setObjectName("formError");
    ratesError->setWordWrap(true);
    ratesError->hide();

    QPushButton *saveButton = new QPushButton("Save rates", card);
    saveButton->setObjectName("primaryButton");
    saveButton->setCursor(Qt::PointingHandCursor);
    connect(saveButton, &QPushButton::clicked, this, &CurrencyPage::onSaveRatesClicked);
    // Enter in a rate field saves too.
    connect(usdRateEdit, &QLineEdit::returnPressed, this, &CurrencyPage::onSaveRatesClicked);
    connect(eurRateEdit, &QLineEdit::returnPressed, this, &CurrencyPage::onSaveRatesClicked);

    QHBoxLayout *bottomRow = new QHBoxLayout();
    bottomRow->addWidget(ratesStatus);
    bottomRow->addStretch();
    bottomRow->addWidget(saveButton);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);
    layout->addWidget(title);
    layout->addWidget(explanation);
    layout->addLayout(fields);
    layout->addWidget(ratesError);
    layout->addLayout(bottomRow);
    return card;
}

QFrame *CurrencyPage::createConverterCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    QLabel *title = new QLabel("Convert an amount", card);
    title->setObjectName("cardTitle");

    amountEdit = new QLineEdit(card);
    amountEdit->setObjectName("convertAmountEdit");
    amountEdit->setPlaceholderText("For example 1,250.00");
    amountEdit->setAccessibleName("Amount to convert");
    fromBox = createCurrencyBox("convertFromBox", card);
    fromBox->setAccessibleName("From currency");
    toBox = createCurrencyBox("convertToBox", card);
    toBox->setAccessibleName("To currency");
    fromBox->setCurrentIndex(fromBox->findData("USD"));
    toBox->setCurrentIndex(toBox->findData("PKR"));

    resultLabel = new QLabel(card);
    resultLabel->setObjectName("conversionResult");
    resultLabel->setWordWrap(true);
    resultNote = new QLabel(card);
    resultNote->setObjectName("mutedText");
    resultNote->setWordWrap(true);

    // Like the forms: the amount has a row of its own, so long amounts such
    // as 1,25,00,000.00 fit; the two currencies share the row below.
    QGridLayout *fields = new QGridLayout();
    fields->setHorizontalSpacing(12);
    fields->setVerticalSpacing(6);
    fields->addWidget(new QLabel("Amount", card), 0, 0, 1, 2);
    fields->addWidget(amountEdit, 1, 0, 1, 2);
    fields->addWidget(new QLabel("From", card), 2, 0);
    fields->addWidget(new QLabel("To", card), 2, 1);
    fields->addWidget(fromBox, 3, 0);
    fields->addWidget(toBox, 3, 1);
    fields->setColumnStretch(0, 1);   // From and To get the same width
    fields->setColumnStretch(1, 1);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);
    layout->addWidget(title);
    layout->addLayout(fields);
    layout->addWidget(resultLabel);
    layout->addWidget(resultNote);
    layout->addStretch();

    // The result changes while the person types or picks a currency.
    connect(amountEdit, &QLineEdit::textChanged, this, &CurrencyPage::updateConversion);
    connect(fromBox, &QComboBox::currentIndexChanged, this, &CurrencyPage::updateConversion);
    connect(toBox, &QComboBox::currentIndexChanged, this, &CurrencyPage::updateConversion);
    return card;
}

QFrame *CurrencyPage::createBalancesCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    QLabel *title = new QLabel("All your balances in one currency", card);
    title->setObjectName("cardTitle");
    QLabel *subtitle = new QLabel("Income minus expenses of all time, for each currency. Money set aside in "
                                  "savings goals is still part of these balances.", card);
    subtitle->setObjectName("cardSubtitle");
    subtitle->setWordWrap(true);

    totalCurrencyBox = createCurrencyBox("totalCurrencyBox", card);
    totalCurrencyBox->setAccessibleName("Show the total in");
    connect(totalCurrencyBox, &QComboBox::currentIndexChanged, this, &CurrencyPage::updateBalances);

    QHBoxLayout *choiceRow = new QHBoxLayout();
    choiceRow->addWidget(new QLabel("Show in", card));
    choiceRow->addWidget(totalCurrencyBox);
    choiceRow->addStretch();

    balanceArea = new QWidget(card);
    balanceGrid = new QGridLayout(balanceArea);
    balanceGrid->setContentsMargins(0, 0, 0, 0);
    balanceGrid->setVerticalSpacing(6);

    totalLabel = new QLabel(card);
    totalLabel->setObjectName("conversionResult");
    totalLabel->setWordWrap(true);
    totalNote = new QLabel("An estimate with your own rates. Nothing is converted or changed in your records.",
                           card);
    totalNote->setObjectName("mutedText");
    totalNote->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addLayout(choiceRow);
    layout->addWidget(balanceArea);
    layout->addWidget(totalLabel);
    layout->addWidget(totalNote);
    layout->addStretch();
    return card;
}

// ---- Loading and drawing -------------------------------------------------

void CurrencyPage::refresh()
{
    rates = database.exchangeRates();
    if (!database.loadTransactions(transactions)) {
        transactions.clear();
        QMessageBox::warning(this, "Finora", "Could not load the transactions.\n\nReason: " + database.lastError());
    }
    showRates();
    updateConversion();
    updateBalances();
}

// The saved rates in the fields ("" for a rate that is not entered).
void CurrencyPage::showRates()
{
    usdRateEdit->clear();
    eurRateEdit->clear();
    if (rates.usdInPkr > 0) {
        usdRateEdit->setText(QString::fromStdString(formatRate(rates.usdInPkr)));
    }
    if (rates.eurInPkr > 0) {
        eurRateEdit->setText(QString::fromStdString(formatRate(rates.eurInPkr)));
    }
    QString savedOn = database.exchangeRatesSavedOn();
    if (savedOn.isEmpty()) {
        ratesStatus->setText("No rates saved yet.");
    } else {
        ratesStatus->setText("Saved on " + QDate::fromString(savedOn, "yyyy-MM-dd").toString("d MMM yyyy") + ".");
    }
}

// "1 USD = 278.5000 PKR", or "" for PKR and for a missing rate.
QString CurrencyPage::rateLine(const std::string &currencyCode) const
{
    long long rate = rateInPkr(rates, currencyCode);
    if (currencyCode == "PKR" || rate <= 0) {
        return "";
    }
    return "1 " + QString::fromStdString(currencyCode) + " = " + QString::fromStdString(formatRate(rate)) + " PKR";
}

void CurrencyPage::updateConversion()
{
    std::string from = fromBox->currentData().toString().toStdString();
    std::string to = toBox->currentData().toString().toStdString();

    long long amountMinor = 0;
    std::string problem = parseAmount(amountEdit->text().toStdString(), amountMinor);
    if (amountEdit->text().trimmed().isEmpty()) {
        resultLabel->setText("-");
        resultNote->setText("Type an amount to convert it with your rates.");
        return;
    }
    if (!problem.empty()) {
        resultLabel->setText("-");
        resultNote->setText(QString::fromStdString(problem));
        return;
    }

    long long converted = 0;
    if (!convertAmount(amountMinor, from, to, rates, converted)) {
        resultLabel->setText("-");
        // Either a rate is missing, or the result would be too large.
        std::string missing = "";
        if (rateInPkr(rates, from) <= 0) {
            missing = from;
        } else if (rateInPkr(rates, to) <= 0) {
            missing = to;
        }
        if (!missing.empty()) {
            resultNote->setText("Enter and save the " + QString::fromStdString(missing)
                                + " rate above to convert this amount.");
        } else {
            resultNote->setText("The amount is too large to convert.");
        }
        return;
    }

    resultLabel->setText(moneyText(from, amountMinor) + " = " + moneyText(to, converted));
    // Which rates were used, and how the result was rounded.
    QStringList used;
    if (!rateLine(from).isEmpty()) {
        used.append(rateLine(from));
    }
    if (from != to && !rateLine(to).isEmpty()) {
        used.append(rateLine(to));
    }
    QString note = "Rounded to the nearest paisa or cent; an exact half is rounded away from zero.";
    if (!used.isEmpty()) {
        note = "At " + used.join(" and ") + ". " + note;
    }
    resultNote->setText(note);
}

// One line per currency that has transactions, and their total in the
// chosen currency. A currency without a rate is shown but not added.
void CurrencyPage::updateBalances()
{
    clearLayout(balanceGrid);
    std::string to = totalCurrencyBox->currentData().toString().toStdString();
    std::vector<CurrencySummary> summaries = summarizeByCurrency(transactions, currentMonth().toStdString());

    long long total = 0;
    bool complete = true;   // false if a currency could not be added
    int row = 0;
    int count = static_cast<int>(summaries.size());
    for (int i = 0; i < count; i++) {
        const CurrencySummary &summary = summaries[i];
        if (summary.transactionCount == 0) {
            continue;   // nothing recorded in this currency
        }
        QLabel *name = new QLabel(QString::fromStdString(summary.currencyCode) + " balance", balanceArea);
        name->setObjectName("mutedText");
        QLabel *own = new QLabel(balanceArea);
        own->setObjectName("amountText");
        QLabel *converted = new QLabel(balanceArea);
        converted->setObjectName("amountText");

        long long convertedMinor = 0;
        if (summary.overflow) {
            own->setText("Too large to show");
            converted->setText("-");
            complete = false;
        } else {
            long long balance = balanceOf(summary);
            own->setText(moneyText(summary.currencyCode, balance));
            if (!convertAmount(balance, summary.currencyCode, to, rates, convertedMinor)) {
                converted->setText("needs a rate");
                complete = false;
            } else if (!addSignedChecked(total, convertedMinor)) {
                converted->setText("too large");
                complete = false;
            } else {
                converted->setText("≈ " + moneyText(to, convertedMinor));
            }
        }
        balanceGrid->addWidget(name, row, 0);
        balanceGrid->addWidget(own, row, 1, Qt::AlignRight);
        balanceGrid->addWidget(converted, row, 2, Qt::AlignRight);
        row++;
    }

    if (row == 0) {
        totalLabel->setText("-");
        totalNote->setText("No transactions yet, so there are no balances to add up.");
    } else if (!complete) {
        totalLabel->setText("-");
        totalNote->setText("Enter and save the missing rates above to see the total.");
    } else {
        totalLabel->setText("≈ " + moneyText(to, total));
        totalNote->setText("An estimate with your own rates. Nothing is converted or changed in your records.");
    }
}

// ---- Saving the rates ----------------------------------------------------

// Reads one rate field: an empty field means "not entered" (0).
bool CurrencyPage::readRateField(QLineEdit *field, const QString &code, long long &rate, QString &problem)
{
    rate = 0;
    if (field->text().trimmed().isEmpty()) {
        return true;
    }
    std::string message = parseRate(field->text().toStdString(), rate);
    if (!message.empty()) {
        problem = code + " rate: " + QString::fromStdString(message);
        return false;
    }
    return true;
}

bool CurrencyPage::saveRates()
{
    ExchangeRates entered;
    QString problem;
    if (!readRateField(usdRateEdit, "USD", entered.usdInPkr, problem)
        || !readRateField(eurRateEdit, "EUR", entered.eurInPkr, problem)) {
        ratesError->setText(problem);
        ratesError->show();
        return false;
    }
    if (!database.saveExchangeRates(entered)) {
        ratesError->setText("Could not save the rates: " + database.lastError());
        ratesError->show();
        return false;
    }
    ratesError->hide();
    refresh();
    Toast::showMessage(this, "Exchange rates saved.");
    return true;
}

void CurrencyPage::onSaveRatesClicked()
{
    saveRates();
}

QString CurrencyPage::conversionText() const
{
    return resultLabel->text();
}

QString CurrencyPage::totalText() const
{
    return totalLabel->text();
}
