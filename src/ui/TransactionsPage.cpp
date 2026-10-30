// =====================================================================
//  TransactionsPage.cpp
// =====================================================================
#include "TransactionsPage.h"

#include <QBrush>
#include <QColor>
#include <QDate>
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

#include "PageHeader.h"
#include "TransactionDialog.h"
#include "core/Money.h"

// ---- Table columns -------------------------------------------------
const int COLUMN_DATE = 0;
const int COLUMN_TYPE = 1;
const int COLUMN_CATEGORY = 2;
const int COLUMN_DESCRIPTION = 3;
const int COLUMN_AMOUNT = 4;
const int COLUMN_COUNT = 5;

// ---- Choices in the period filter ----------------------------------
const int PERIOD_ALL = 0;
const int PERIOD_THIS_MONTH = 1;
const int PERIOD_LAST_MONTH = 2;
const int PERIOD_THIS_YEAR = 3;

// The initializer list makes the member `database` refer to the
// DatabaseManager that main() created. A reference member must be set
// here, because a reference cannot be changed after it is created.
TransactionsPage::TransactionsPage(DatabaseManager &sharedDatabase, QWidget *parent)
    : QWidget(parent), database(sharedDatabase)
{
    setObjectName("transactionsPage");

    messageLabel = new QLabel(this);
    messageLabel->setObjectName("successBanner");
    messageLabel->hide();

    // A single-shot timer hides the banner 4 seconds after the last message.
    messageTimer = new QTimer(this);
    messageTimer->setSingleShot(true);
    connect(messageTimer, &QTimer::timeout, messageLabel, &QLabel::hide);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 28, 32, 24);
    layout->setSpacing(16);
    layout->addWidget(createHeader());
    layout->addWidget(messageLabel);
    layout->addWidget(createFilterBar());
    layout->addWidget(createTableCard(), 1);   // the table takes the remaining height

    reloadTransactions();
}

// Title on the left; Edit, Delete and Add Transaction on the right.
QWidget *TransactionsPage::createHeader()
{
    QWidget *header = new QWidget(this);

    editButton = new QPushButton("Edit", header);
    editButton->setObjectName("secondaryButton");
    editButton->setEnabled(false);   // enabled when a row is selected

    deleteButton = new QPushButton("Delete", header);
    deleteButton->setObjectName("dangerButton");
    deleteButton->setEnabled(false);

    QPushButton *addButton = new QPushButton("+  Add Transaction", header);
    addButton->setObjectName("primaryButton");
    addButton->setAccessibleName("Add Transaction");

    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(new PageHeader("Transactions", "Manage your income and expenses", header));
    layout->addStretch();
    layout->addWidget(editButton);
    layout->addWidget(deleteButton);
    layout->addWidget(addButton);

    connect(addButton, &QPushButton::clicked, this, &TransactionsPage::onAddClicked);
    connect(editButton, &QPushButton::clicked, this, &TransactionsPage::onEditClicked);
    connect(deleteButton, &QPushButton::clicked, this, &TransactionsPage::onDeleteClicked);
    return header;
}

// Search box and four drop-down filters in a white card.
QFrame *TransactionsPage::createFilterBar()
{
    QFrame *bar = new QFrame(this);
    bar->setObjectName("card");

    searchEdit = new QLineEdit(bar);
    searchEdit->setObjectName("searchEdit");
    searchEdit->setPlaceholderText("Search description or category...");
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setAccessibleName("Search transactions");

    // Each filter item keeps its value as hidden data; "" means "all".
    typeFilter = new QComboBox(bar);
    typeFilter->setObjectName("typeFilter");
    typeFilter->addItem("All types", "");
    typeFilter->addItem("Income", QString::fromStdString(TYPE_INCOME));
    typeFilter->addItem("Expense", QString::fromStdString(TYPE_EXPENSE));

    currencyFilter = new QComboBox(bar);
    currencyFilter->setObjectName("currencyFilter");
    currencyFilter->addItem("All currencies", "");
    for (int i = 0; i < SUPPORTED_CURRENCY_COUNT; i++) {
        QString code = QString::fromUtf8(SUPPORTED_CURRENCIES[i].code);
        currencyFilter->addItem(code, code);
    }

    categoryFilter = new QComboBox(bar);
    categoryFilter->setObjectName("categoryFilter");
    categoryFilter->addItem("All categories", "");

    // The order of these items must match the PERIOD_ constants above.
    periodFilter = new QComboBox(bar);
    periodFilter->setObjectName("periodFilter");
    periodFilter->addItem("All dates");
    periodFilter->addItem("This month");
    periodFilter->addItem("Last month");
    periodFilter->addItem("This year");

    QPushButton *clearButton = new QPushButton("Clear filters", bar);
    clearButton->setObjectName("secondaryButton");
    clearButton->setAccessibleName("Clear filters");

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);
    layout->addWidget(searchEdit, 1);
    layout->addWidget(typeFilter);
    layout->addWidget(currencyFilter);
    layout->addWidget(categoryFilter);
    layout->addWidget(periodFilter);
    layout->addWidget(clearButton);

    // Any change in the filter bar shows the list again at once.
    connect(searchEdit, &QLineEdit::textChanged, this, &TransactionsPage::applyFilters);
    connect(typeFilter, &QComboBox::currentIndexChanged, this, &TransactionsPage::applyFilters);
    connect(currencyFilter, &QComboBox::currentIndexChanged, this, &TransactionsPage::applyFilters);
    connect(categoryFilter, &QComboBox::currentIndexChanged, this, &TransactionsPage::applyFilters);
    connect(periodFilter, &QComboBox::currentIndexChanged, this, &TransactionsPage::applyFilters);
    connect(clearButton, &QPushButton::clicked, this, &TransactionsPage::clearFilters);
    return bar;
}

// The table, the "nothing to show" message and the row count.
QFrame *TransactionsPage::createTableCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    table = new QTableWidget(0, COLUMN_COUNT, card);
    table->setObjectName("transactionTable");
    table->setHorizontalHeaderLabels({"Date", "Type", "Category", "Description", "Amount"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);   // edit through the form only
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(COLUMN_DESCRIPTION, QHeaderView::Stretch);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->horizontalHeaderItem(COLUMN_AMOUNT)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table->setColumnWidth(COLUMN_DATE, 120);
    table->setColumnWidth(COLUMN_TYPE, 100);
    table->setColumnWidth(COLUMN_CATEGORY, 160);
    table->setColumnWidth(COLUMN_AMOUNT, 180);
    table->setAccessibleName("Transactions");

    emptyLabel = new QLabel(card);
    emptyLabel->setObjectName("emptyText");
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);

    countLabel = new QLabel(card);
    countLabel->setObjectName("mutedText");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 8, 8, 12);
    layout->setSpacing(8);
    layout->addWidget(table, 1);
    layout->addWidget(emptyLabel, 1);
    layout->addWidget(countLabel);

    connect(table, &QTableWidget::itemSelectionChanged, this, &TransactionsPage::onSelectionChanged);
    // Double-clicking a row opens it for editing. Qt allows connecting a
    // signal that sends (row, column) to a slot that takes no parameters.
    connect(table, &QTableWidget::cellDoubleClicked, this, &TransactionsPage::onEditClicked);
    return card;
}

// ---- Actions -------------------------------------------------------

bool TransactionsPage::saveNewTransaction(Transaction &transaction)
{
    if (!database.addTransaction(transaction)) {
        showDatabaseError("save the transaction");
        return false;
    }
    reloadTransactions();
    showSuccess("Transaction added.");
    return true;
}

bool TransactionsPage::saveEditedTransaction(const Transaction &transaction)
{
    if (!database.updateTransaction(transaction)) {
        showDatabaseError("update the transaction");
        return false;
    }
    reloadTransactions();
    showSuccess("Transaction updated.");
    return true;
}

bool TransactionsPage::deleteTransactionById(int id)
{
    if (!database.deleteTransaction(id)) {
        showDatabaseError("delete the transaction");
        return false;
    }
    reloadTransactions();
    showSuccess("Transaction deleted.");
    return true;
}

void TransactionsPage::reloadTransactions()
{
    if (!database.loadTransactions(allTransactions)) {
        showDatabaseError("load the transactions");
    }
    refreshCategoryFilter();
    applyFilters();
}

// ---- Slots ---------------------------------------------------------

void TransactionsPage::onAddClicked()
{
    TransactionDialog dialog(this);

    // exec() shows the form and waits until the user clicks Save or Cancel.
    if (dialog.exec() == QDialog::Accepted) {
        Transaction newTransaction = dialog.transaction();
        saveNewTransaction(newTransaction);
    }
}

void TransactionsPage::onEditClicked()
{
    Transaction existing;
    if (!findLoadedTransaction(selectedTransactionId(), existing)) {
        return;   // no row selected
    }

    TransactionDialog dialog(existing, this);
    if (dialog.exec() == QDialog::Accepted) {
        saveEditedTransaction(dialog.transaction());
    }
}

void TransactionsPage::onDeleteClicked()
{
    Transaction existing;
    if (!findLoadedTransaction(selectedTransactionId(), existing)) {
        return;
    }

    // Ask first. "No" is the default button, so pressing Enter is safe.
    QDate date = QDate::fromString(QString::fromStdString(existing.date), "yyyy-MM-dd");
    QString summary = date.toString("dd MMM yyyy") + "   "
                      + QString::fromStdString(existing.category) + "   "
                      + QString::fromStdString(formatMoney(existing.currencyCode, existing.amountMinor));
    if (!existing.description.empty()) {
        summary += "\n" + QString::fromStdString(existing.description);
    }
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, "Delete transaction",
        "Delete this transaction?\n\n" + summary + "\n\nThis cannot be undone.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer == QMessageBox::Yes) {
        deleteTransactionById(existing.id);
    }
}

void TransactionsPage::onSelectionChanged()
{
    bool rowSelected = selectedTransactionId() > 0;
    editButton->setEnabled(rowSelected);
    deleteButton->setEnabled(rowSelected);
}

void TransactionsPage::applyFilters()
{
    showTransactions(filterTransactions(allTransactions, currentFilter()));
}

void TransactionsPage::clearFilters()
{
    // blockSignals stops each reset from redrawing the table separately.
    searchEdit->blockSignals(true);
    typeFilter->blockSignals(true);
    currencyFilter->blockSignals(true);
    categoryFilter->blockSignals(true);
    periodFilter->blockSignals(true);

    searchEdit->clear();
    typeFilter->setCurrentIndex(0);
    currencyFilter->setCurrentIndex(0);
    categoryFilter->setCurrentIndex(0);
    periodFilter->setCurrentIndex(PERIOD_ALL);

    searchEdit->blockSignals(false);
    typeFilter->blockSignals(false);
    currencyFilter->blockSignals(false);
    categoryFilter->blockSignals(false);
    periodFilter->blockSignals(false);

    applyFilters();
}

// ---- Helpers -------------------------------------------------------

// Turns the filter bar into a TransactionFilter for the core logic.
TransactionFilter TransactionsPage::currentFilter() const
{
    TransactionFilter filter;
    filter.searchText = searchEdit->text().trimmed().toStdString();
    filter.type = typeFilter->currentData().toString().toStdString();
    filter.currencyCode = currencyFilter->currentData().toString().toStdString();
    filter.category = categoryFilter->currentData().toString().toStdString();

    QDate today = QDate::currentDate();
    QDate firstDay;   // stays invalid for "All dates"
    QDate lastDay;

    switch (periodFilter->currentIndex()) {
    case PERIOD_THIS_MONTH:
        firstDay = QDate(today.year(), today.month(), 1);
        lastDay = firstDay.addMonths(1).addDays(-1);
        break;
    case PERIOD_LAST_MONTH:
        firstDay = QDate(today.year(), today.month(), 1).addMonths(-1);
        lastDay = firstDay.addMonths(1).addDays(-1);
        break;
    case PERIOD_THIS_YEAR:
        firstDay = QDate(today.year(), 1, 1);
        lastDay = QDate(today.year(), 12, 31);
        break;
    default:
        break;   // PERIOD_ALL: no date limits
    }

    if (firstDay.isValid()) {
        filter.fromDate = firstDay.toString("yyyy-MM-dd").toStdString();
        filter.toDate = lastDay.toString("yyyy-MM-dd").toStdString();
    }
    return filter;
}

// Fills the table with one row per transaction.
void TransactionsPage::showTransactions(const std::vector<Transaction> &transactions)
{
    int rowCount = static_cast<int>(transactions.size());
    table->clearContents();
    table->setRowCount(rowCount);

    for (int row = 0; row < rowCount; row++) {
        const Transaction &transaction = transactions[row];
        bool isIncome = (transaction.type == TYPE_INCOME);

        QDate date = QDate::fromString(QString::fromStdString(transaction.date), "yyyy-MM-dd");
        QTableWidgetItem *dateItem = new QTableWidgetItem(date.toString("dd MMM yyyy"));
        dateItem->setData(Qt::UserRole, transaction.id);   // the hidden id of this row

        QString typeText = "Expense";
        if (isIncome) {
            typeText = "Income";
        }
        QTableWidgetItem *typeItem = new QTableWidgetItem(typeText);
        QTableWidgetItem *categoryItem = new QTableWidgetItem(QString::fromStdString(transaction.category));
        QTableWidgetItem *descriptionItem = new QTableWidgetItem(QString::fromStdString(transaction.description));
        QTableWidgetItem *amountItem = new QTableWidgetItem(
            QString::fromStdString(formatMoney(transaction.currencyCode, transaction.amountMinor)));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // Green for income, red for expenses.
        QColor color("#DC2626");
        if (isIncome) {
            color = QColor("#16A34A");
        }
        typeItem->setForeground(QBrush(color));
        amountItem->setForeground(QBrush(color));

        // The table takes ownership of each item and deletes it later.
        table->setItem(row, COLUMN_DATE, dateItem);
        table->setItem(row, COLUMN_TYPE, typeItem);
        table->setItem(row, COLUMN_CATEGORY, categoryItem);
        table->setItem(row, COLUMN_DESCRIPTION, descriptionItem);
        table->setItem(row, COLUMN_AMOUNT, amountItem);
    }

    int total = static_cast<int>(allTransactions.size());
    if (total == 0) {
        emptyLabel->setText("No transactions yet.\nClick \"Add Transaction\" to record your first income or expense.");
    } else if (rowCount == 0) {
        emptyLabel->setText("No transactions match these filters.");
    }
    table->setVisible(rowCount > 0);
    emptyLabel->setVisible(rowCount == 0);
    countLabel->setText("Showing " + QString::number(rowCount) + " of " + QString::number(total)
                        + " transactions");

    table->clearSelection();
    onSelectionChanged();
}

// Rebuilds the category filter from the categories that are really used,
// keeping the current choice if that category still exists.
void TransactionsPage::refreshCategoryFilter()
{
    QString chosen = categoryFilter->currentData().toString();

    categoryFilter->blockSignals(true);
    categoryFilter->clear();
    categoryFilter->addItem("All categories", "");

    std::vector<std::string> categories = distinctCategories(allTransactions);
    int count = static_cast<int>(categories.size());
    for (int i = 0; i < count; i++) {
        QString category = QString::fromStdString(categories[i]);
        categoryFilter->addItem(category, category);
    }

    int index = categoryFilter->findData(chosen);
    if (index < 0) {
        index = 0;
    }
    categoryFilter->setCurrentIndex(index);
    categoryFilter->blockSignals(false);
}

// The id of the selected row, or 0 if no row is selected.
int TransactionsPage::selectedTransactionId() const
{
    QList<QTableWidgetItem *> selected = table->selectedItems();
    if (selected.isEmpty()) {
        return 0;
    }
    int row = selected.first()->row();
    return table->item(row, COLUMN_DATE)->data(Qt::UserRole).toInt();
}

// Finds a loaded transaction by its id with a simple linear search.
bool TransactionsPage::findLoadedTransaction(int id, Transaction &result) const
{
    if (id <= 0) {
        return false;
    }
    int count = static_cast<int>(allTransactions.size());
    for (int i = 0; i < count; i++) {
        if (allTransactions[i].id == id) {
            result = allTransactions[i];
            return true;
        }
    }
    return false;
}

void TransactionsPage::showSuccess(const QString &message)
{
    messageLabel->setText(message);
    messageLabel->show();
    messageTimer->start(4000);   // restarts the 4-second countdown
}

void TransactionsPage::showDatabaseError(const QString &action)
{
    QMessageBox::warning(this, "Finora",
                         "Could not " + action + ".\n\nReason: " + database.lastError());
}
