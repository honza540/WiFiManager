#ifndef ARDUINO_HOST_IO_H
#define ARDUINO_HOST_IO_H

#include "ArduinoHostString.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <thread>

// ============================================================================
// SERIAL AND TIME STUBS
// ============================================================================

class IPAddress {
public:
    IPAddress() : octets{0,0,0,0} {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : octets{a,b,c,d} {}
    String toString() const {
        return String(std::to_string(octets[0]) + "." + std::to_string(octets[1]) + "." + std::to_string(octets[2]) + "." + std::to_string(octets[3]));
    }
private:
    uint8_t octets[4];
};

class Stream {
public:
    virtual void print(const String& msg) = 0;
    virtual void println(const String& msg) { print(msg + "\n"); }
    virtual ~Stream() {}
};

class NativeSerial : public Stream {
public:
    void begin(unsigned long) {}
    void print(const String& msg) override {
        lastMessage += msg.c_str();
    }
    void println(const String& msg) override {
        lastMessage += msg.c_str();
        lastMessage += '\n';
    }
    void println(const IPAddress& ip) {
        println(ip.toString());
    }
    void println() {
        lastMessage += '\n';
    }
    void print(char c) {
        lastMessage += c;
    }
    static String lastMessage;
};

extern NativeSerial Serial;

inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void configTime(long, int, const char*, const char* = nullptr) {}

inline bool getLocalTime(struct tm* info, uint32_t = 5000) {
    if (info) {
        *info = tm();
    }
    return false;
}

unsigned long millis();

#endif // ARDUINO_HOST_IO_H
