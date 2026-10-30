// =====================================================================
//  BudgetDialog.cpp
// =====================================================================
#include "BudgetDialog.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "core/Money.h"
#include "core/TransactionFilter.h"   // toLowerText

BudgetDialog::BudgetDialog(const std::vector<Budget> &existingBudgets, QWidget *parent)
    : QDialog(parent), otherBudgets(existingBudgets)
{
    createForm("New Budget");
}

BudgetDialog::BudgetDialog(const Budget &budgetToEdit, const std::vector<Budget> &existingBudgets,
                           QWidget *parent)
    : QDialog(parent), otherBudgets(existingBudgets), budgetId(budgetToEdit.id)
{
    createForm("Edit Budget");

    categoryBox->setCurrentText(QString::fromStdString(budgetToEdit.category));
    int currencyIndex = currencyBox->findData(QString::fromStdString(budgetToEdit.currencyCode));
    if (currencyIndex >= 0) {
        currencyBox->setCurrentIndex(currencyIndex);
    }
    limitEdit->setText(QString::fromStdString(formatAmount(budgetToEdit.limitMinor)));
}

Budget BudgetDialog::budget() const
{
    return savedBudget;
}

void BudgetDialog::createForm(const QString &title)
{
    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(460);

    QLabel *heading = new QLabel(title, this);
    heading->setObjectName("dialogTitle");
    QLabel *help = new QLabel("A budget is a spending limit for one category. It starts again every month, "
                              "and Finora warns you at 80%.", this);
    help->setObjectName("mutedText");
    help->setWordWrap(true);

    // Expense categories only: a budget limits spending. The list is
    // editable, so a category of the user's own can be typed in.
    categoryBox = new QComboBox(this);
    categoryBox->setObjectName("budgetCategoryBox");
    categoryBox->setEditable(true);
    categoryBox->setAccessibleName("Category");
    for (int i = 0; i < EXPENSE_CATEGORY_COUNT; i++) {
        categoryBox->addItem(QString::fromStdString(EXPENSE_CATEGORIES[i]));
    }

    currencyBox = new QComboBox(this);
    currencyBox->setObjectName("budgetCurrencyBox");
    currencyBox->setAccessibleName("Currency");
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        QString code = QString::fromUtf8(SUPPORTED_CURRENCIES[i].code);
        currencyBox->addItem(code + "  " + QString::fromUtf8(SUPPORTED_CURRENCIES[i].name), code);
    }

    limitEdit = new QLineEdit(this);
    limitEdit->setObjectName("budgetLimitEdit");
    limitEdit->setPlaceholderText("For example 20,000");
    limitEdit->setAccessibleName("Monthly limit");

    limitPreview = new QLabel(this);
    limitPreview->setObjectName("amountPreview");

    errorLabel = new QLabel(this);
    errorLabel->setObjectName("formError");
    errorLabel->setWordWrap(true);
    errorLabel->hide();

    QGridLayout *grid = new QGridLayout();
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);
    grid->addWidget(new QLabel("Category", this), 0, 0);
    grid->addWidget(new QLabel("Currency", this), 0, 1);
    grid->addWidget(categoryBox, 1, 0);
    grid->addWidget(currencyBox, 1, 1);
    grid->addWidget(new QLabel("Monthly limit", this), 2, 0, 1, 2);
    grid->addWidget(limitEdit, 3, 0, 1, 2);
    grid->addWidget(limitPreview, 4, 0, 1, 2);

    QPushButton *cancelButton = new QPushButton("Cancel", this);
    cancelButton->setObjectName("secondaryButton");
    saveButton = new QPushButton("Save Budget", this);
    saveButton->setObjectName("primaryButton");
    saveButton->setDefault(true);   // Enter saves

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(saveButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(12);
    layout->addWidget(heading);
    layout->addWidget(help);
    layout->addLayout(grid);
    layout->addWidget(errorLabel);
    layout->addLayout(buttons);

    connect(limitEdit, &QLineEdit::textChanged, this, &BudgetDialog::onLimitChanged);
    connect(currencyBox, &QComboBox::currentIndexChanged, this, &BudgetDialog::onLimitChanged);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveButton, &QPushButton::clicked, this, &BudgetDialog::onSaveClicked);
    onLimitChanged();
}

void BudgetDialog::onLimitChanged()
{
    long long limitMinor = 0;
    std::string problem = parseAmount(limitEdit->text().toStdString(), limitMinor);
    if (problem.empty() && limitMinor > 0) {
        std::string code = currencyBox->currentData().toString().toStdString();
        limitPreview->setText("Will be saved as " + QString::fromStdString(formatMoney(code, limitMinor))
                              + " per month");
    } else {
        limitPreview->setText("Type the most you want to spend in one month.");
    }
}

void BudgetDialog::onSaveClicked()
{
    Budget entered;
    QString problem;
    if (!readForm(entered, problem)) {
        errorLabel->setText(problem);
        errorLabel->show();
        adjustSize();   // grow the form so the message fits
        return;         // the dialog stays open
    }
    savedBudget = entered;
    accept();   // closes the dialog; exec() returns QDialog::Accepted
}

bool BudgetDialog::readForm(Budget &entered, QString &problem) const
{
    entered.id = budgetId;
    entered.category = categoryBox->currentText().trimmed().toStdString();
    entered.currencyCode = currencyBox->currentData().toString().toStdString();

    std::string amountProblem = parseAmount(limitEdit->text().toStdString(), entered.limitMinor);
    if (!amountProblem.empty()) {
        problem = QString::fromStdString(amountProblem);
        return false;
    }

    std::string budgetProblem = validateBudget(entered);
    if (!budgetProblem.empty()) {
        problem = QString::fromStdString(budgetProblem);
        return false;
    }

    // One budget per category and currency (the store checks this again).
    int count = static_cast<int>(otherBudgets.size());
    for (int i = 0; i < count; i++) {
        const Budget &other = otherBudgets[i];
        if (other.id != entered.id && other.currencyCode == entered.currencyCode
            && toLowerText(other.category) == toLowerText(entered.category)) {
            problem = "There is already a " + QString::fromStdString(other.category) + " budget in "
                      + QString::fromStdString(other.currencyCode) + ". Edit that budget instead.";
            return false;
        }
    }
    return true;
}
