// =====================================================================
//  TransactionDialog.cpp
// =====================================================================
#include "TransactionDialog.h"

#include <QDate>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "core/Money.h"

TransactionDialog::TransactionDialog(QWidget *parent)
    : QDialog(parent)
{
    createForm("Add Transaction");
    expenseButton->setChecked(true);
    fillCategories(TYPE_EXPENSE);
}

TransactionDialog::TransactionDialog(const Transaction &existing, QWidget *parent)
    : QDialog(parent)
{
    createForm("Edit Transaction");
    fillForm(existing);
}

Transaction TransactionDialog::transaction() const
{
    return savedTransaction;
}

// Builds every widget of the form and connects the signals.
void TransactionDialog::createForm(const QString &title)
{
    setWindowTitle(title);
    setMinimumWidth(520);

    QLabel *heading = new QLabel(title, this);
    heading->setObjectName("dialogTitle");

    // Type: two radio buttons. Only one of them can be checked.
    incomeButton = new QRadioButton("Income", this);
    incomeButton->setObjectName("incomeButton");
    expenseButton = new QRadioButton("Expense", this);
    expenseButton->setObjectName("expenseButton");
    QHBoxLayout *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(incomeButton);
    typeLayout->addWidget(expenseButton);
    typeLayout->addStretch();

    // Category: a list of suggestions, but the user may also type a new one.
    categoryBox = new QComboBox(this);
    categoryBox->setObjectName("categoryBox");
    categoryBox->setEditable(true);
    categoryBox->lineEdit()->setMaxLength(40);

    amountEdit = new QLineEdit(this);
    amountEdit->setObjectName("amountEdit");
    amountEdit->setPlaceholderText("0.00");
    amountEdit->setMaxLength(20);

    // Currency: one entry per supported currency. The code ("PKR") is kept
    // as hidden data, and the visible text also shows the name.
    currencyBox = new QComboBox(this);
    currencyBox->setObjectName("currencyBox");
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        QString code = QString::fromUtf8(SUPPORTED_CURRENCIES[i].code);
        QString name = QString::fromUtf8(SUPPORTED_CURRENCIES[i].name);
        currencyBox->addItem(code + " - " + name, code);
    }

    // Date: a calendar pop-up limited to the years isValidIsoDate accepts.
    dateEdit = new QDateEdit(QDate::currentDate(), this);
    dateEdit->setObjectName("dateEdit");
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("dd MMM yyyy");
    dateEdit->setMinimumDate(QDate(1900, 1, 1));
    dateEdit->setMaximumDate(QDate(2100, 12, 31));

    descriptionEdit = new QLineEdit(this);
    descriptionEdit->setObjectName("descriptionEdit");
    descriptionEdit->setPlaceholderText("Optional note, for example: Groceries from Ali's shop");
    descriptionEdit->setMaxLength(200);

    amountPreview = new QLabel(this);
    amountPreview->setObjectName("amountPreview");

    errorLabel = new QLabel(this);
    errorLabel->setObjectName("formError");
    errorLabel->setWordWrap(true);
    errorLabel->hide();

    // The form: labels in one column, fields next to them.
    QGridLayout *grid = new QGridLayout();
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(6);
    grid->addWidget(new QLabel("Type", this), 0, 0);
    grid->addWidget(new QLabel("Category", this), 0, 1);
    grid->addLayout(typeLayout, 1, 0);
    grid->addWidget(categoryBox, 1, 1);
    grid->addWidget(new QLabel("Amount", this), 2, 0);
    grid->addWidget(new QLabel("Currency", this), 2, 1);
    grid->addWidget(amountEdit, 3, 0);
    grid->addWidget(currencyBox, 3, 1);
    grid->addWidget(amountPreview, 4, 0, 1, 2);
    grid->addWidget(new QLabel("Date", this), 5, 0);
    grid->addWidget(dateEdit, 6, 0);
    grid->addWidget(new QLabel("Description", this), 7, 0, 1, 2);
    grid->addWidget(descriptionEdit, 8, 0, 1, 2);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    QPushButton *cancelButton = new QPushButton("Cancel", this);
    cancelButton->setObjectName("secondaryButton");
    saveButton = new QPushButton("Save Transaction", this);
    saveButton->setObjectName("primaryButton");
    saveButton->setDefault(true);   // pressing Enter clicks Save

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(saveButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);
    layout->addWidget(heading);
    layout->addLayout(grid);
    layout->addWidget(errorLabel);
    layout->addLayout(buttons);

    // Signals and slots: Qt calls these functions when the user acts.
    connect(incomeButton, &QRadioButton::toggled, this, &TransactionDialog::onTypeChanged);
    connect(amountEdit, &QLineEdit::textChanged, this, &TransactionDialog::onAmountChanged);
    connect(currencyBox, &QComboBox::currentIndexChanged, this, &TransactionDialog::onAmountChanged);
    connect(saveButton, &QPushButton::clicked, this, &TransactionDialog::onSaveClicked);
    connect(cancelButton, &QPushButton::clicked, this, &TransactionDialog::reject);
}

// Puts the values of an existing transaction into the form.
void TransactionDialog::fillForm(const Transaction &existing)
{
    transactionId = existing.id;

    if (existing.type == TYPE_INCOME) {
        incomeButton->setChecked(true);
    } else {
        expenseButton->setChecked(true);
    }
    fillCategories(existing.type);
    categoryBox->setCurrentText(QString::fromStdString(existing.category));

    amountEdit->setText(QString::fromStdString(formatAmount(existing.amountMinor)));
    int currencyIndex = currencyBox->findData(QString::fromStdString(existing.currencyCode));
    if (currencyIndex >= 0) {
        currencyBox->setCurrentIndex(currencyIndex);
    }
    dateEdit->setDate(QDate::fromString(QString::fromStdString(existing.date), "yyyy-MM-dd"));
    descriptionEdit->setText(QString::fromStdString(existing.description));
}

// Shows the suggested categories for income or for expenses.
void TransactionDialog::fillCategories(const std::string &type)
{
    categoryBox->clear();
    if (type == TYPE_INCOME) {
        for (int i = 0; i < INCOME_CATEGORY_COUNT; i++) {
            categoryBox->addItem(QString::fromStdString(INCOME_CATEGORIES[i]));
        }
    } else {
        for (int i = 0; i < EXPENSE_CATEGORY_COUNT; i++) {
            categoryBox->addItem(QString::fromStdString(EXPENSE_CATEGORIES[i]));
        }
    }
}

void TransactionDialog::onTypeChanged()
{
    if (incomeButton->isChecked()) {
        fillCategories(TYPE_INCOME);
    } else {
        fillCategories(TYPE_EXPENSE);
    }
}

// While the user types, show how the amount will be saved, for example
// "Will be saved as PKR 1,200.50". Nothing is shown while it is not valid.
void TransactionDialog::onAmountChanged()
{
    long long amountMinor = 0;
    std::string problem = parseAmount(amountEdit->text().toStdString(), amountMinor);

    if (problem.empty()) {
        std::string code = currencyBox->currentData().toString().toStdString();
        amountPreview->setText("Will be saved as " + QString::fromStdString(formatMoney(code, amountMinor)));
    } else {
        amountPreview->clear();
    }
}

void TransactionDialog::onSaveClicked()
{
    Transaction entered;
    QString problem;

    if (!readForm(entered, problem)) {
        errorLabel->setText(problem);
        errorLabel->show();
        return;   // the dialog stays open so the user can correct it
    }

    savedTransaction = entered;
    accept();   // closes the dialog; exec() returns QDialog::Accepted
}

// Reads every field into `entered`. Returns false and a message in
// `problem` if the amount cannot be read or a rule is broken.
bool TransactionDialog::readForm(Transaction &entered, QString &problem) const
{
    entered.id = transactionId;
    if (incomeButton->isChecked()) {
        entered.type = TYPE_INCOME;
    } else {
        entered.type = TYPE_EXPENSE;
    }

    std::string amountProblem = parseAmount(amountEdit->text().toStdString(), entered.amountMinor);
    if (!amountProblem.empty()) {
        problem = QString::fromStdString(amountProblem);
        return false;
    }

    entered.currencyCode = currencyBox->currentData().toString().toStdString();
    entered.category = categoryBox->currentText().trimmed().toStdString();
    entered.description = descriptionEdit->text().trimmed().toStdString();
    entered.date = dateEdit->date().toString("yyyy-MM-dd").toStdString();

    // The same rules that DatabaseManager checks before saving.
    std::string ruleProblem = validateTransaction(entered);
    if (!ruleProblem.empty()) {
        problem = QString::fromStdString(ruleProblem);
        return false;
    }
    return true;
}
