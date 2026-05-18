#include "BTConsoleAuth.h"

BTConsoleAuth::BTConsoleAuth()
    : enabled(false),
      password(""),
      timeoutMs(0),
      maxAttempts(0),
      attempts(0),
      sessionStartedAt(0),
      state(STATE_DISABLED) {
}

void BTConsoleAuth::configure(bool enabledValue, const String& passwordValue,
                              unsigned long timeoutMsValue, int maxAttemptsValue) {
    enabled = enabledValue;
    password = passwordValue;
    timeoutMs = timeoutMsValue;
    maxAttempts = maxAttemptsValue;
    reset();
}

void BTConsoleAuth::reset() {
    attempts = 0;
    sessionStartedAt = 0;
    state = enabled ? STATE_PENDING : STATE_DISABLED;
}

void BTConsoleAuth::beginSession(unsigned long now) {
    attempts = 0;
    sessionStartedAt = now;
    state = enabled ? STATE_PENDING : STATE_DISABLED;
}

bool BTConsoleAuth::isEnabled() const {
    return enabled;
}

bool BTConsoleAuth::isAuthenticated() const {
    return !enabled || state == STATE_AUTHENTICATED;
}

bool BTConsoleAuth::allowsConsole() const {
    return isAuthenticated();
}

bool BTConsoleAuth::isPending() const {
    return enabled && state == STATE_PENDING;
}

bool BTConsoleAuth::isLocked() const {
    return enabled && state == STATE_LOCKED;
}

BTConsoleAuth::Result BTConsoleAuth::submitPassword(const String& input, unsigned long now) {
    if (!enabled) {
        return RESULT_SUCCESS;
    }

    Result timeoutResult = checkTimeout(now);
    if (timeoutResult != RESULT_NONE) {
        return timeoutResult;
    }

    if (state == STATE_AUTHENTICATED) {
        return RESULT_SUCCESS;
    }

    if (state == STATE_LOCKED) {
        return RESULT_LOCKED;
    }

    String candidate = input;
    candidate.trim();

    if (candidate == password) {
        state = STATE_AUTHENTICATED;
        return RESULT_SUCCESS;
    }

    attempts++;
    if (maxAttempts > 0 && attempts >= maxAttempts) {
        state = STATE_LOCKED;
        return RESULT_LOCKED;
    }

    return RESULT_FAILED;
}

BTConsoleAuth::Result BTConsoleAuth::checkTimeout(unsigned long now) {
    if (!isPending() || timeoutMs == 0) {
        return RESULT_NONE;
    }

    if (hasTimedOut(now)) {
        state = STATE_LOCKED;
        return RESULT_TIMEOUT;
    }

    return RESULT_NONE;
}

bool BTConsoleAuth::hasTimedOut(unsigned long now) const {
    return now - sessionStartedAt >= timeoutMs;
}
