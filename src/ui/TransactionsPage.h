// =====================================================================
//  TransactionsPage.h
//  The Transactions page: list, search, filter, add, edit and delete.
// =====================================================================
#ifndef FINORA_TRANSACTIONSPAGE_H
#define FINORA_TRANSACTIONSPAGE_H

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

#include <vector>

#include "core/Transaction.h"
#include "core/TransactionFilter.h"
#include "database/DatabaseManager.h"

// Shows the user's transactions in a table and lets them add, edit,
// delete, search and filter them.
//
// The page does not own the database. main() creates the one
// DatabaseManager, and the page keeps a reference to it (`database`).
// Every change goes through DatabaseManager; afterwards the page reloads
// the list from SQLite, so the table always shows what is really saved.
class TransactionsPage : public QWidget
{
    Q_OBJECT   // required because this class has its own slots

public:
    TransactionsPage(DatabaseManager &sharedDatabase, QWidget *parent = nullptr);

    // The actions behind the buttons. Each returns true on success. On a
    // database error it shows a warning and returns false. They are public
    // so that the automated tests can call them directly.
    bool saveNewTransaction(Transaction &transaction);
    bool saveEditedTransaction(const Transaction &transaction);
    bool deleteTransactionById(int id);

    // Loads every transaction from SQLite again and shows the filtered list.
    void reloadTransactions();

private slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onSelectionChanged();
    void applyFilters();
    void clearFilters();

private:
    QWidget *createHeader();
    QFrame *createFilterBar();
    QFrame *createTableCard();

    TransactionFilter currentFilter() const;
    void showTransactions(const std::vector<Transaction> &transactions);
    void refreshCategoryFilter();
    int selectedTransactionId() const;
    bool findLoadedTransaction(int id, Transaction &result) const;
    void showSuccess(const QString &message);
    void showDatabaseError(const QString &action);

    DatabaseManager &database;                  // created in main(); this page only uses it
    std::vector<Transaction> allTransactions;   // every transaction loaded from SQLite

    QPushButton *editButton = nullptr;
    QPushButton *deleteButton = nullptr;
    QLabel *messageLabel = nullptr;     // green "Transaction added." banner
    QTimer *messageTimer = nullptr;     // hides the banner after a few seconds
    QLineEdit *searchEdit = nullptr;
    QComboBox *typeFilter = nullptr;
    QComboBox *currencyFilter = nullptr;
    QComboBox *categoryFilter = nullptr;
    QComboBox *periodFilter = nullptr;
    QTableWidget *table = nullptr;
    QLabel *emptyLabel = nullptr;       // shown instead of the table when it has no rows
    QLabel *countLabel = nullptr;       // "Showing 3 of 10 transactions"
};

#endif // FINORA_TRANSACTIONSPAGE_H
