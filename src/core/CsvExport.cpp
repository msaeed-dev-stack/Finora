// =====================================================================
//  CsvExport.cpp
// =====================================================================
#include "CsvExport.h"

std::string csvField(const std::string &value)
{
    std::string text = value;
    int length = static_cast<int>(text.size());

    // 1. A spreadsheet runs a field as a formula when it starts with
    //    = + - or @, and some spreadsheets first skip spaces, tabs and line
    //    breaks. So Finora looks at the first character that is NOT one of
    //    those. A ' in front then keeps the field plain text. A field that
    //    starts with a tab, a carriage return or a line feed gets the ' too.
    int first = 0;
    while (first < length && (text[first] == ' ' || text[first] == '\t' || text[first] == '\r'
                              || text[first] == '\n')) {
        first++;
    }
    bool dangerous = false;
    if (first < length) {
        char c = text[first];
        dangerous = (c == '=' || c == '+' || c == '-' || c == '@');
    }
    if (length > 0 && (text[0] == '\t' || text[0] == '\r' || text[0] == '\n')) {
        dangerous = true;
    }
    if (dangerous) {
        text = "'" + text;
        length = static_cast<int>(text.size());
    }

    // 2. Commas, quotes and line breaks need quotes around the field.
    bool needsQuotes = false;
    for (int i = 0; i < length; i++) {
        if (text[i] == ',' || text[i] == '"' || text[i] == '\n' || text[i] == '\r') {
            needsQuotes = true;
        }
    }
    if (!needsQuotes) {
        return text;
    }

    // Every quote inside the field is written twice.
    std::string quoted = "\"";
    for (int i = 0; i < length; i++) {
        if (text[i] == '"') {
            quoted += "\"\"";
        } else {
            quoted += text[i];
        }
    }
    quoted += "\"";
    return quoted;
}

std::string plainAmount(long long amountMinor)
{
    bool negative = amountMinor < 0;
    long long value = amountMinor;
    if (negative) {
        value = -value;
    }
    long long whole = value / 100;
    long long cents = value % 100;
    std::string text = std::to_string(whole) + ".";
    if (cents < 10) {
        text += "0";
    }
    text += std::to_string(cents);
    if (negative) {
        text = "-" + text;
    }
    return text;
}

std::string transactionsCsv(const std::vector<Transaction> &transactions)
{
    std::string text = "Date,Type,Category,Description,Currency,Amount\r\n";
    int count = static_cast<int>(transactions.size());
    for (int i = 0; i < count; i++) {
        const Transaction &t = transactions[i];
        text += csvField(t.date) + "," + csvField(t.type) + "," + csvField(t.category) + ","
                + csvField(t.description) + "," + csvField(t.currencyCode) + "," + plainAmount(t.amountMinor)
                + "\r\n";
    }
    return text;
}
