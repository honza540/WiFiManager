#ifndef ARDUINO_HOST_BLUETOOTH_SERIAL_H
#define ARDUINO_HOST_BLUETOOTH_SERIAL_H

#include "ArduinoHostIO.h"

#include <cstddef>
#include <cstdint>
#include <functional>

// ============================================================================
// BLUETOOTH SERIAL STUB
// ============================================================================

class BluetoothSerial : public Stream {
public:
    BluetoothSerial() : connectedFlag(false) {}
    void enableSSP() {}
    void onConfirmRequest(std::function<void(uint32_t)> callback) {
        confirmCallback = callback;
    }
    void confirmReply(bool accepted) {
        confirmAccepted = accepted;
    }
    void onAuthComplete(std::function<void(boolean)> callback) {
        authCallback = callback;
    }
    bool begin(const char*, bool) {
        connectedFlag = false;
        return true;
    }
    bool setPin(const char*) {
        return true;
    }
    bool connected() const {
        return connectedFlag;
    }
    size_t available() const {
        return 0;
    }
    char read() {
        return 0;
    }
    void print(const String& msg) override {
        (void)msg;
    }
    void print(char c) {
        (void)c;
    }
    void println(const String& msg) {
        (void)msg;
    }

private:
    bool connectedFlag;
    bool confirmAccepted = false;
    std::function<void(uint32_t)> confirmCallback;
    std::function<void(boolean)> authCallback;
};

#endif // ARDUINO_HOST_BLUETOOTH_SERIAL_H
