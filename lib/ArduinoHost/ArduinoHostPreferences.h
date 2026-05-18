#ifndef ARDUINO_HOST_PREFERENCES_H
#define ARDUINO_HOST_PREFERENCES_H

#include "ArduinoHostString.h"

#include <cstdint>
#include <map>
#include <string>

// ============================================================================
// PREFERENCES STUB
// ============================================================================

class Preferences {
public:
    bool begin(const char* ns, bool) {
        namespaceName = ns ? ns : "";
        started = true;
        return true;
    }
    void end() {
        started = false;
    }
    String getString(const char* key, const String& defaultValue) {
        if (!started) {
            return defaultValue;
        }
        auto it = storage.find(key);
        return it != storage.end() ? String(it->second) : defaultValue;
    }
    void putString(const char* key, const String& value) {
        if (!started) {
            return;
        }
        storage[key] = value.c_str();
    }
    uint8_t getUChar(const char* key, uint8_t defaultValue) {
        if (!started) {
            return defaultValue;
        }
        auto it = storage.find(key);
        if (it != storage.end()) {
            try {
                return static_cast<uint8_t>(std::stoi(it->second));
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    void putUChar(const char* key, uint8_t value) {
        if (!started) {
            return;
        }
        storage[key] = std::to_string(value);
    }
    void remove(const char* key) {
        if (!started) {
            return;
        }
        storage.erase(key);
    }
    void clear() {
        if (!started) {
            return;
        }
        storage.clear();
    }

private:
    bool started = false;
    string namespaceName;
    std::map<string, string> storage;
};

#endif // ARDUINO_HOST_PREFERENCES_H
