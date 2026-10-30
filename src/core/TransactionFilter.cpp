// =====================================================================
//  TransactionFilter.cpp
// =====================================================================
#include "TransactionFilter.h"

#include <algorithm>   // std::sort

std::string toLowerText(const std::string &text)
{
    std::string result = text;
    int length = static_cast<int>(result.length());

    for (int i = 0; i < length; i++) {
        // 'A' to 'Z' and 'a' to 'z' are two runs of consecutive codes,
        // so adding ('a' - 'A') turns an upper-case letter into lower case.
        if (result[i] >= 'A' && result[i] <= 'Z') {
            result[i] = static_cast<char>(result[i] + ('a' - 'A'));
        }
    }
    return result;
}

bool containsIgnoringCase(const std::string &text, const std::string &part)
{
    // find() returns std::string::npos when the part is not found.
    return toLowerText(text).find(toLowerText(part)) != std::string::npos;
}

bool matchesFilter(const Transaction &transaction, const TransactionFilter &filter)
{
    if (!filter.type.empty() && transaction.type != filter.type) {
        return false;
    }
    if (!filter.currencyCode.empty() && transaction.currencyCode != filter.currencyCode) {
        return false;
    }
    if (!filter.category.empty() && transaction.category != filter.category) {
        return false;
    }

    // Dates written as YYYY-MM-DD compare correctly as text:
    // "2026-09-30" < "2026-10-01" is true.
    if (!filter.fromDate.empty() && transaction.date < filter.fromDate) {
        return false;
    }
    if (!filter.toDate.empty() && transaction.date > filter.toDate) {
        return false;
    }

    if (!filter.searchText.empty()) {
        bool inDescription = containsIgnoringCase(transaction.description, filter.searchText);
        bool inCategory = containsIgnoringCase(transaction.category, filter.searchText);
        if (!inDescription && !inCategory) {
            return false;
        }
    }
    return true;
}

std::vector<Transaction> filterTransactions(const std::vector<Transaction> &transactions,
                                            const TransactionFilter &filter)
{
    std::vector<Transaction> result;
    int count = static_cast<int>(transactions.size());

    for (int i = 0; i < count; i++) {
        if (matchesFilter(transactions[i], filter)) {
            result.push_back(transactions[i]);
        }
    }
    return result;
}

std::vector<std::string> distinctCategories(const std::vector<Transaction> &transactions)
{
    std::vector<std::string> categories;
    int count = static_cast<int>(transactions.size());

    for (int i = 0; i < count; i++) {
        // Add the category only if it is not in the list yet.
        bool alreadyListed = false;
        int listed = static_cast<int>(categories.size());
        for (int j = 0; j < listed; j++) {
            if (categories[j] == transactions[i].category) {
                alreadyListed = true;
                break;
            }
        }
        if (!alreadyListed) {
            categories.push_back(transactions[i].category);
        }
    }

    std::sort(categories.begin(), categories.end());   // A to Z
    return categories;
}
