// =====================================================================
//  finora_tests.cpp
//  Automated checks for Finora's core logic (plain C++, no Qt).
//
//  Every check prints PASS or FAIL. The program returns 0 only when all
//  checks pass, so CTest (and build.bat) can report the result.
// =====================================================================
#include <iostream>
#include <string>

#include "core/Greeting.h"

// Counts how many checks ran and how many of them failed.
struct TestCounter {
    int checksRun = 0;
    int checksFailed = 0;
};

// Compares the text a function returned with the text we expected.
void checkText(TestCounter &counter, const std::string &testName,
               const std::string &actual, const std::string &expected)
{
    counter.checksRun++;

    if (actual == expected) {
        std::cout << "PASS  " << testName << "\n";
    } else {
        counter.checksFailed++;
        std::cout << "FAIL  " << testName << "   expected \"" << expected
                  << "\" but got \"" << actual << "\"\n";
    }
}

// One test case: an input hour and the greeting it should produce.
struct GreetingCase {
    int hour;
    std::string expected;
};

// Tests greetingForHour() at the edges of every time range, because
// mistakes usually happen at the boundaries (for example < versus <=).
void testGreetingForHour(TestCounter &counter)
{
    const int CASE_COUNT = 11;
    const GreetingCase cases[CASE_COUNT] = {
        {5,  "Good Morning"},     // first morning hour
        {9,  "Good Morning"},     // normal morning hour
        {11, "Good Morning"},     // last morning hour
        {12, "Good Afternoon"},   // first afternoon hour
        {16, "Good Afternoon"},   // last afternoon hour
        {17, "Good Evening"},     // first evening hour
        {23, "Good Evening"},     // last hour of the day
        {0,  "Good Evening"},     // midnight
        {4,  "Good Evening"},     // last hour before morning
        {-1, "Hello"},            // invalid: below 0
        {24, "Hello"}             // invalid: above 23
    };

    for (int i = 0; i < CASE_COUNT; i++) {
        std::string testName = "greetingForHour(" + std::to_string(cases[i].hour) + ")";
        checkText(counter, testName, greetingForHour(cases[i].hour), cases[i].expected);
    }
}

int main()
{
    TestCounter counter;

    std::cout << "Finora core tests\n";
    std::cout << "-----------------\n";

    testGreetingForHour(counter);

    std::cout << "-----------------\n";
    std::cout << counter.checksRun << " checks, " << counter.checksFailed << " failed\n";

    if (counter.checksFailed > 0) {
        return 1;   // a non-zero exit code tells CTest that tests failed
    }
    return 0;
}
