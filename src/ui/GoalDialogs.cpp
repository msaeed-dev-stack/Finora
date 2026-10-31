// =====================================================================
//  GoalDialogs.cpp
// =====================================================================
#include "GoalDialogs.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

#include "UiHelpers.h"
#include "core/Money.h"
#include "core/Transaction.h"   // SUPPORTED_CURRENCIES

// Cancel and Save buttons at the bottom right of a form.
QHBoxLayout *createFormButtons(QDialog *dialog, const QString &saveText, QPushButton *&saveButton)
{
    QPushButton *cancelButton = new QPushButton("Cancel", dialog);
    cancelButton->setObjectName("secondaryButton");
    saveButton = new QPushButton(saveText, dialog);
    saveButton->setObjectName("primaryButton");
    saveButton->setDefault(true);   // Enter saves
    QObject::connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(saveButton);
    return buttons;
}

// ---- GoalDialog ------------------------------------------------------------

GoalDialog::GoalDialog(QWidget *parent)
    : QDialog(parent)
{
    createForm("New Goal");
}

GoalDialog::GoalDialog(const Goal &goalToEdit, QWidget *parent)
    : QDialog(parent), goalId(goalToEdit.id), saved(goalToEdit.savedMinor)
{
    createForm("Edit Goal");

    nameEdit->setText(QString::fromStdString(goalToEdit.name));
    int currencyIndex = currencyBox->findData(QString::fromStdString(goalToEdit.currencyCode));
    if (currencyIndex >= 0) {
        currencyBox->setCurrentIndex(currencyIndex);
    }
    targetEdit->setText(QString::fromStdString(formatAmount(goalToEdit.targetMinor)));
    if (!goalToEdit.targetDate.empty()) {
        hasDateBox->setChecked(true);
        dateEdit->setDate(QDate::fromString(QString::fromStdString(goalToEdit.targetDate), "yyyy-MM-dd"));
    }

    // The saved money is in the goal's currency, so the currency is fixed
    // once something is saved. GoalStore refuses the change as well.
    if (goalToEdit.savedMinor != 0) {
        currencyBox->setEnabled(false);
        currencyBox->setToolTip("The currency cannot change after money has been put into the goal.");
    }
}

Goal GoalDialog::goal() const
{
    return savedGoal;
}

void GoalDialog::createForm(const QString &title)
{
    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(480);

    QLabel *heading = new QLabel(title, this);
    heading->setObjectName("dialogTitle");
    QLabel *help = new QLabel("Something you are saving for, such as a laptop or an emergency fund. "
                              "With a target date, Finora works out how much to save each month.", this);
    help->setObjectName("mutedText");
    help->setWordWrap(true);

    nameEdit = new QLineEdit(this);
    nameEdit->setObjectName("goalNameEdit");
    nameEdit->setPlaceholderText("For example New laptop");
    nameEdit->setMaxLength(GOAL_NAME_MAX_LENGTH);
    nameEdit->setAccessibleName("Goal name");

    currencyBox = new QComboBox(this);
    currencyBox->setObjectName("goalCurrencyBox");
    currencyBox->setAccessibleName("Currency");
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        QString code = QString::fromUtf8(SUPPORTED_CURRENCIES[i].code);
        currencyBox->addItem(code + "  " + QString::fromUtf8(SUPPORTED_CURRENCIES[i].name), code);
    }

    targetEdit = new QLineEdit(this);
    targetEdit->setObjectName("goalTargetEdit");
    targetEdit->setPlaceholderText("For example 250,000");
    targetEdit->setAccessibleName("Target amount");
    targetPreview = new QLabel(this);
    targetPreview->setObjectName("amountPreview");

    // A target date is optional: the date field works only while the box is ticked.
    hasDateBox = new QCheckBox("Reach it by a date", this);
    hasDateBox->setObjectName("goalHasDateBox");
    dateEdit = new QDateEdit(QDate::currentDate().addMonths(6), this);
    dateEdit->setObjectName("goalDateEdit");
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("dd MMM yyyy");
    dateEdit->setMinimumDate(QDate(1900, 1, 1));
    dateEdit->setMaximumDate(QDate(2100, 12, 31));
    dateEdit->setAccessibleName("Target date");
    dateEdit->setEnabled(false);

    errorLabel = new QLabel(this);
    errorLabel->setObjectName("formError");
    errorLabel->setWordWrap(true);
    errorLabel->hide();

    QGridLayout *grid = new QGridLayout();
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);
    grid->addWidget(new QLabel("Name", this), 0, 0);
    grid->addWidget(new QLabel("Currency", this), 0, 1);
    grid->addWidget(nameEdit, 1, 0);
    grid->addWidget(currencyBox, 1, 1);
    grid->addWidget(new QLabel("Target amount", this), 2, 0, 1, 2);
    grid->addWidget(targetEdit, 3, 0, 1, 2);
    grid->addWidget(targetPreview, 4, 0, 1, 2);
    grid->addWidget(hasDateBox, 5, 0);
    grid->addWidget(dateEdit, 5, 1);

    QPushButton *saveButton = nullptr;
    QHBoxLayout *buttons = createFormButtons(this, "Save Goal", saveButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(12);
    layout->addWidget(heading);
    layout->addWidget(help);
    layout->addLayout(grid);
    layout->addWidget(errorLabel);
    layout->addLayout(buttons);

    connect(targetEdit, &QLineEdit::textChanged, this, &GoalDialog::onTargetChanged);
    connect(currencyBox, &QComboBox::currentIndexChanged, this, &GoalDialog::onTargetChanged);
    connect(hasDateBox, &QCheckBox::toggled, this, &GoalDialog::onHasDateToggled);
    connect(saveButton, &QPushButton::clicked, this, &GoalDialog::onSaveClicked);
    onTargetChanged();
}

void GoalDialog::onTargetChanged()
{
    long long targetMinor = 0;
    std::string problem = parseAmount(targetEdit->text().toStdString(), targetMinor);
    if (problem.empty() && targetMinor > 0) {
        std::string code = currencyBox->currentData().toString().toStdString();
        targetPreview->setText("Will be saved as " + QString::fromStdString(formatMoney(code, targetMinor)));
    } else {
        targetPreview->setText("Type the amount you want to reach.");
    }
}

void GoalDialog::onHasDateToggled()
{
    dateEdit->setEnabled(hasDateBox->isChecked());
}

void GoalDialog::onSaveClicked()
{
    Goal entered;
    QString problem;
    if (!readForm(entered, problem)) {
        errorLabel->setText(problem);
        errorLabel->show();
        adjustSize();   // grow the form so the message fits
        return;         // the dialog stays open
    }
    savedGoal = entered;
    accept();
}

bool GoalDialog::readForm(Goal &entered, QString &problem) const
{
    entered.id = goalId;
    entered.savedMinor = saved;
    entered.name = nameEdit->text().trimmed().toStdString();
    entered.currencyCode = currencyBox->currentData().toString().toStdString();
    if (hasDateBox->isChecked()) {
        entered.targetDate = dateEdit->date().toString("yyyy-MM-dd").toStdString();
    }

    std::string amountProblem = parseAmount(targetEdit->text().toStdString(), entered.targetMinor);
    if (!amountProblem.empty()) {
        problem = QString::fromStdString(amountProblem);
        return false;
    }
    std::string goalProblem = validateGoal(entered);
    if (!goalProblem.empty()) {
        problem = QString::fromStdString(goalProblem);
        return false;
    }
    return true;
}

// ---- ContributionDialog ------------------------------------------------------

ContributionDialog::ContributionDialog(const Goal &goalToChange, bool takeOut, QWidget *parent)
    : QDialog(parent), goal(goalToChange), takingOut(takeOut)
{
    QString title = "Add money";
    QString help = "Record money you have set aside for this goal. No transaction is added, so your "
                   "balances and budgets stay the same: the money is still yours.";
    QString saveText = "Add Money";
    if (takingOut) {
        title = "Take money out";
        help = "Record money you took back from this goal, for example to pay for something. No transaction "
               "is added. You can take out at most what is saved.";
        saveText = "Take Out";
    }
    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(460);

    QLabel *heading = new QLabel(title, this);
    heading->setObjectName("dialogTitle");
    QLabel *goalLine = new QLabel(QString::fromStdString(goal.name) + " · "
                                      + moneyText(goal.currencyCode, goal.savedMinor) + " saved of "
                                      + moneyText(goal.currencyCode, goal.targetMinor), this);
    goalLine->setObjectName("bodyText");
    QLabel *helpLabel = new QLabel(help, this);
    helpLabel->setObjectName("mutedText");
    helpLabel->setWordWrap(true);

    amountEdit = new QLineEdit(this);
    amountEdit->setObjectName("contributionAmountEdit");
    amountEdit->setPlaceholderText("For example 5,000");
    amountEdit->setAccessibleName("Amount");
    amountPreview = new QLabel(this);
    amountPreview->setObjectName("amountPreview");

    dateEdit = new QDateEdit(QDate::currentDate(), this);
    dateEdit->setObjectName("contributionDateEdit");
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("dd MMM yyyy");
    dateEdit->setMinimumDate(QDate(1900, 1, 1));
    dateEdit->setMaximumDate(QDate::currentDate());   // money cannot be saved on a future day
    dateEdit->setAccessibleName("Date");

    noteEdit = new QLineEdit(this);
    noteEdit->setObjectName("contributionNoteEdit");
    noteEdit->setPlaceholderText("Optional, for example October salary");
    noteEdit->setMaxLength(GOAL_NOTE_MAX_LENGTH);
    noteEdit->setAccessibleName("Note");

    errorLabel = new QLabel(this);
    errorLabel->setObjectName("formError");
    errorLabel->setWordWrap(true);
    errorLabel->hide();

    QGridLayout *grid = new QGridLayout();
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);
    grid->addWidget(new QLabel("Amount (" + QString::fromStdString(goal.currencyCode) + ")", this), 0, 0);
    grid->addWidget(new QLabel("Date", this), 0, 1);
    grid->addWidget(amountEdit, 1, 0);
    grid->addWidget(dateEdit, 1, 1);
    grid->addWidget(amountPreview, 2, 0, 1, 2);
    grid->addWidget(new QLabel("Note", this), 3, 0, 1, 2);
    grid->addWidget(noteEdit, 4, 0, 1, 2);

    QPushButton *saveButton = nullptr;
    QHBoxLayout *buttons = createFormButtons(this, saveText, saveButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(12);
    layout->addWidget(heading);
    layout->addWidget(goalLine);
    layout->addWidget(helpLabel);
    layout->addLayout(grid);
    layout->addWidget(errorLabel);
    layout->addLayout(buttons);

    connect(amountEdit, &QLineEdit::textChanged, this, &ContributionDialog::onAmountChanged);
    connect(saveButton, &QPushButton::clicked, this, &ContributionDialog::onSaveClicked);
    onAmountChanged();
}

GoalContribution ContributionDialog::contribution() const
{
    return savedContribution;
}

// Shows what the saved amount will become, before anything is saved.
void ContributionDialog::onAmountChanged()
{
    long long amountMinor = 0;
    std::string problem = parseAmount(amountEdit->text().toStdString(), amountMinor);
    if (!problem.empty() || amountMinor <= 0) {
        amountPreview->setText("Type the amount.");
        return;
    }
    if (takingOut) {
        amountMinor = -amountMinor;
    }
    GoalContribution check;
    check.amountMinor = amountMinor;
    check.date = "2000-01-01";   // only the amount matters for this preview
    std::string amountProblem = validateContribution(check, goal.savedMinor);
    if (!amountProblem.empty()) {
        amountPreview->setText(QString::fromStdString(amountProblem));
        return;
    }
    amountPreview->setText("Saved afterwards: " + moneyText(goal.currencyCode, goal.savedMinor + amountMinor));
}

void ContributionDialog::onSaveClicked()
{
    GoalContribution entered;
    QString problem;
    if (!readForm(entered, problem)) {
        errorLabel->setText(problem);
        errorLabel->show();
        adjustSize();
        return;
    }
    savedContribution = entered;
    accept();
}

bool ContributionDialog::readForm(GoalContribution &entered, QString &problem) const
{
    entered.goalId = goal.id;
    entered.date = dateEdit->date().toString("yyyy-MM-dd").toStdString();
    entered.note = noteEdit->text().trimmed().toStdString();

    long long amountMinor = 0;
    std::string amountProblem = parseAmount(amountEdit->text().toStdString(), amountMinor);
    if (!amountProblem.empty()) {
        problem = QString::fromStdString(amountProblem);
        return false;
    }
    // The form always takes a positive amount; taking out stores it negative.
    entered.amountMinor = amountMinor;
    if (takingOut) {
        entered.amountMinor = -amountMinor;
    }
    std::string contributionProblem = validateContribution(entered, goal.savedMinor);
    if (!contributionProblem.empty()) {
        problem = QString::fromStdString(contributionProblem);
        return false;
    }
    return true;
}

// ---- GoalHistoryDialog -------------------------------------------------------

GoalHistoryDialog::GoalHistoryDialog(GoalStore &goalStore, const Goal &goal, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Goal history");
    setMinimumSize(560, 420);

    QLabel *heading = new QLabel(QString::fromStdString(goal.name), this);
    heading->setObjectName("dialogTitle");
    QLabel *subtitle = new QLabel("Every amount put in or taken out, newest first. A mistake is corrected "
                                  "with a new entry, so the history always stays complete.", this);
    subtitle->setObjectName("mutedText");
    subtitle->setWordWrap(true);

    table = new QTableWidget(this);
    table->setObjectName("historyTable");
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"Date", "Amount", "Note"});
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setFrameShape(QFrame::NoFrame);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    QLabel *problem = new QLabel(this);
    problem->setObjectName("formError");
    problem->setWordWrap(true);
    problem->hide();

    std::vector<GoalContribution> contributions;
    if (goalStore.loadContributions(goal.id, contributions)) {
        rowCount = static_cast<int>(contributions.size());
        table->setRowCount(rowCount);
        for (int i = 0; i < rowCount; i++) {
            QDate date = QDate::fromString(QString::fromStdString(contributions[i].date), "yyyy-MM-dd");
            QTableWidgetItem *amount = new QTableWidgetItem(
                QString::fromStdString(formatSignedMoney(goal.currencyCode, contributions[i].amountMinor)));
            amount->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(i, 0, new QTableWidgetItem(date.toString("d MMM yyyy")));
            table->setItem(i, 1, amount);
            table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(contributions[i].note)));
        }
    } else {
        table->hide();
        problem->setText(goalStore.lastError());
        problem->show();
    }

    QPushButton *closeButton = new QPushButton("Close", this);
    closeButton->setObjectName("primaryButton");
    closeButton->setDefault(true);
    connect(closeButton, &QPushButton::clicked, this, &GoalHistoryDialog::accept);
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(closeButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(12);
    layout->addWidget(heading);
    layout->addWidget(subtitle);
    layout->addWidget(table, 1);
    layout->addWidget(problem);
    layout->addLayout(buttons);
}

int GoalHistoryDialog::shownCount() const
{
    return rowCount;
}
