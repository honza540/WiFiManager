#ifndef ARDUINO_HOST_BLUETOOTH_SERIAL_H
#define ARDUINO_HOST_BLUETOOTH_SERIAL_H

#include "ArduinoHostIO.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

// ============================================================================
// BLUETOOTH SERIAL STUB
// ============================================================================

class BluetoothSerial : public Stream {
public:
    BluetoothSerial() : connectedFlag(false), runningFlag(false), rxPos(0) {}
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
        runningFlag = true;
        return true;
    }
    void end() {
        connectedFlag = false;
        runningFlag = false;
    }
    bool setPin(const char*) {
        return true;
    }
    bool connected() const {
        return runningFlag && connectedFlag;
    }
    size_t available() const {
        return rxBuffer.size() - rxPos;
    }
    char read() {
        if (rxPos >= rxBuffer.size()) {
            return 0;
        }
        char c = rxBuffer[rxPos++];
        if (rxPos >= rxBuffer.size()) {
            rxBuffer.clear();
            rxPos = 0;
        }
        return c;
    }
    void print(const String& msg) override {
        txBuffer += msg.c_str();
    }
    void print(char c) {
        txBuffer += c;
    }
    void println(const String& msg) override {
        txBuffer += msg.c_str();
        txBuffer += '\n';
    }

    void setConnected(bool connected) {
        connectedFlag = connected;
    }

    void queueInput(const String& input) {
        rxBuffer += input.c_str();
    }

    void clearOutput() {
        txBuffer.clear();
    }

    String getOutput() const {
        return String(txBuffer);
    }

private:
    bool connectedFlag;
    bool runningFlag;
    bool confirmAccepted = false;
    std::function<void(uint32_t)> confirmCallback;
    std::function<void(boolean)> authCallback;
    std::string rxBuffer;
    size_t rxPos;
    std::string txBuffer;
};

#endif // ARDUINO_HOST_BLUETOOTH_SERIAL_H
