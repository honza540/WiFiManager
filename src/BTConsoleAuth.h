#ifndef BT_CONSOLE_AUTH_H
#define BT_CONSOLE_AUTH_H

#include <Arduino.h>

class BTConsoleAuth {
public:
    enum Result {
        RESULT_NONE,
        RESULT_SUCCESS,
        RESULT_FAILED,
        RESULT_LOCKED,
        RESULT_TIMEOUT
    };

    BTConsoleAuth();

    void configure(bool enabled, const String& password, unsigned long timeoutMs, int maxAttempts);
    void reset();
    void beginSession(unsigned long now);

    bool isEnabled() const;
    bool isAuthenticated() const;
    bool allowsConsole() const;
    bool isPending() const;
    bool isLocked() const;

    Result submitPassword(const String& input, unsigned long now);
    Result checkTimeout(unsigned long now);

private:
    enum State {
        STATE_DISABLED,
        STATE_PENDING,
        STATE_AUTHENTICATED,
        STATE_LOCKED
    };

    bool enabled;
    String password;
    unsigned long timeoutMs;
    int maxAttempts;
    int attempts;
    unsigned long sessionStartedAt;
    State state;

    bool hasTimedOut(unsigned long now) const;
};

#endif // BT_CONSOLE_AUTH_H
