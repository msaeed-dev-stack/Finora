// =====================================================================
//  Session.cpp
// =====================================================================
#include "Session.h"

#include "AccountRules.h"   // firstNameOf

bool Session::isSignedIn() const
{
    return currentUserId > 0;
}

int Session::userId() const
{
    return currentUserId;
}

int Session::sessionId() const
{
    return currentSessionId;
}

std::string Session::email() const
{
    return currentEmail;
}

std::string Session::displayName() const
{
    return currentDisplayName;
}

std::string Session::firstName() const
{
    return firstNameOf(currentDisplayName);
}

void Session::start(int user, int session, const std::string &email, const std::string &displayName)
{
    currentUserId = user;
    currentSessionId = session;
    currentEmail = email;
    currentDisplayName = displayName;
}

void Session::end()
{
    currentUserId = 0;
    currentSessionId = 0;
    currentEmail.clear();
    currentDisplayName.clear();
}

void Session::updateProfile(const std::string &email, const std::string &displayName)
{
    currentEmail = email;
    currentDisplayName = displayName;
}
