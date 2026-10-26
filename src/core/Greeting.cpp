// =====================================================================
//  Greeting.cpp
// =====================================================================
#include "Greeting.h"

std::string greetingForHour(int hour)
{
    // Input validation: a clock hour can only be 0 to 23.
    if (hour < 0 || hour > 23) {
        return "Hello";
    }

    if (hour >= 5 && hour < 12) {
        return "Good Morning";
    } else if (hour >= 12 && hour < 17) {
        return "Good Afternoon";
    } else {
        return "Good Evening";   // 17:00 to 23:59 and 00:00 to 04:59
    }
}
