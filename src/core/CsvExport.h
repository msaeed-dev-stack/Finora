// =====================================================================
//  CsvExport.h
//  Turns transactions into CSV text (comma-separated values), the
//  format that spreadsheet programs such as Excel open. Phase 4C: the
//  CSV export of Finora Plus.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_CSVEXPORT_H
#define FINORA_CSVEXPORT_H

#include <string>
#include <vector>

#include "Transaction.h"

// One CSV field, ready to be written:
//  - a field with a comma, a quote or a line break is put in quotes, and
//    every quote inside is doubled:  Ali "Bhai"  ->  "Ali ""Bhai"""
//  - a field whose first character after any spaces, tabs or line breaks
//    is = + - or @, and a field that starts with a tab, a carriage return
//    or a line feed, gets a ' in front, so a spreadsheet shows it as text
//    and never runs it as a formula ("CSV injection", a known attack on
//    exported files).
std::string csvField(const std::string &value);

// An amount as a plain number for a spreadsheet, without thousands
// separators: 120050 gives "1200.50", 5 gives "0.05".
std::string plainAmount(long long amountMinor);

// The whole file: a header line, then one line per transaction. Every
// line ends with \r\n, as the CSV standard (RFC 4180) asks.
std::string transactionsCsv(const std::vector<Transaction> &transactions);

#endif // FINORA_CSVEXPORT_H
