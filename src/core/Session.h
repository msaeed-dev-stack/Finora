// =====================================================================
//  Session.h
//  Who is signed in right now.
//  Plain C++ (no Qt).
// =====================================================================
#ifndef FINORA_SESSION_H
#define FINORA_SESSION_H

#include <string>

class AuthService;   // declared in src/auth/AuthService.h

// Every part of Finora may READ the Session, but only AuthService can
// sign someone in or out: the functions that change it are private, and
// AuthService is the only friend class. So no window, text box or page
// can make Finora believe it is another user. DatabaseManager asks the
// Session whose records it may show; the user id never comes from the
// user interface.
class Session
{
public:
    bool isSignedIn() const;
    int userId() const;             // 0 when nobody is signed in
    int sessionId() const;          // the row in the login_sessions table, 0 when signed out
    std::string email() const;
    std::string displayName() const;
    std::string firstName() const;  // for the greeting: "Good afternoon, Muhammad"

private:
    friend class AuthService;

    void start(int user, int session, const std::string &email, const std::string &displayName);
    void end();
    void updateProfile(const std::string &email, const std::string &displayName);

    int currentUserId = 0;
    int currentSessionId = 0;
    std::string currentEmail;
    std::string currentDisplayName;
};

#endif // FINORA_SESSION_H
