// =====================================================================
//  Greeting.h
//  Chooses the greeting shown at the top of the dashboard.
//  This is plain C++ (no Qt), so the tests can check it directly.
// =====================================================================
#ifndef FINORA_GREETING_H
#define FINORA_GREETING_H

#include <string>

// Returns a greeting that matches the time of day.
//   hour   : the current hour on a 24-hour clock (0 to 23)
//   returns: "Good Morning"   for 05:00 to 11:59
//            "Good Afternoon" for 12:00 to 16:59
//            "Good Evening"   for 17:00 to 04:59
//            "Hello"          if the hour is not between 0 and 23
std::string greetingForHour(int hour);

#endif // FINORA_GREETING_H
