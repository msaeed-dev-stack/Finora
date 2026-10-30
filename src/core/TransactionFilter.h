// =====================================================================
//  TransactionFilter.h
//  Searching and filtering a list of transactions.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_TRANSACTIONFILTER_H
#define FINORA_TRANSACTIONFILTER_H

#include <string>
#include <vector>

#include "Transaction.h"

// What the user chose in the filter bar. An empty field means "any".
struct TransactionFilter {
    std::string searchText;     // part of the description or category
    std::string type;           // TYPE_INCOME, TYPE_EXPENSE, or "" for both
    std::string currencyCode;   // "PKR", "USD", "EUR", or "" for all
    std::string category;       // one exact category, or "" for all
    std::string fromDate;       // "YYYY-MM-DD": earliest date shown, or ""
    std::string toDate;         // "YYYY-MM-DD": latest date shown, or ""
};

// Returns a copy of text with the letters A to Z changed to a to z.
// Other characters, including Urdu letters, are kept as they are.
std::string toLowerText(const std::string &text);

// True if `part` appears anywhere in `text`, ignoring upper and lower case.
bool containsIgnoringCase(const std::string &text, const std::string &part);

// True if one transaction passes every rule of the filter.
bool matchesFilter(const Transaction &transaction, const TransactionFilter &filter);

// Returns the transactions that pass the filter, in their original order.
std::vector<Transaction> filterTransactions(const std::vector<Transaction> &transactions,
                                            const TransactionFilter &filter);

// Returns every category used by the transactions, once each, in A to Z order.
std::vector<std::string> distinctCategories(const std::vector<Transaction> &transactions);

#endif // FINORA_TRANSACTIONFILTER_H
