#include "BTCommandHandler.h"
#include "WiFiManagerCommands.h"

#if BT_CONSOLE_AUTH_ENABLED
#define BT_CONSOLE_AUTH_PASSWORD_VALUE BT_CONSOLE_PASSWORD
#else
#define BT_CONSOLE_AUTH_PASSWORD_VALUE ""
#endif

// Static member initialization
BluetoothSerial* BTCommandHandler::serialBT = nullptr;
bool BTCommandHandler::initialized = false;
bool BTCommandHandler::btConnected = false;
bool BTCommandHandler::autoStopHold = false;
bool BTCommandHandler::userOverrideActive = false;
bool BTCommandHandler::userOverrideEnabled = false;
String BTCommandHandler::commandBuffer = "";
unsigned long BTCommandHandler::lastHeartbeat = 0;
unsigned long BTCommandHandler::noClientTimeoutMs = BT_NO_CLIENT_TIMEOUT_MS;
const char* BTCommandHandler::TAG = "BT";
std::vector<ICommandHandler*> BTCommandHandler::commandHandlers;
ICommandHandler* BTCommandHandler::wifiCommandHandler = nullptr;
BTConsoleAuth BTCommandHandler::consoleAuth;

void BTCommandHandler::begin() {
    if (initialized) {
        return;
    }

    if (userOverrideActive && !userOverrideEnabled) {
        return;
    }

    if (serialBT == nullptr) {
        serialBT = new BluetoothSerial();
    }

    LOG_INFO(TAG, "Starting Bluetooth with name: " BT_DEVICE_NAME);
    serialBT->enableSSP();
    serialBT->onConfirmRequest([](uint32_t numericValue) {
        LOG_WARN(TAG, "BT SSP confirmation auto-accepted: " + String(numericValue));
        if (serialBT != nullptr) {
            serialBT->confirmReply(true);
        }
    });
    serialBT->onAuthComplete([](boolean success) {
        LOG_WARN(TAG, success ? "BT authentication succeeded" : "BT authentication failed");
    });

    // BluetoothSerial role matters:
    // - Slave/server mode (isMaster=false): ESP32 advertises an SPP service.
    //   A phone/PC initiates the connection and uses ESP32 as a serial console.
    //   This is what we want for a maintenance/debug terminal.
    // - Master/client mode (isMaster=true): ESP32 actively connects to another
    //   Bluetooth SPP server. That is useful for peripherals, but awkward for
    //   a phone terminal because the phone expects to connect to the ESP32.
    if (!serialBT->begin(BT_DEVICE_NAME, false)) {
        LOG_ERROR(TAG, "Failed to start Bluetooth");
        delete serialBT;
        serialBT = nullptr;
        return;
    }

    if (!serialBT->setPin(BT_PASSWORD)) {
        LOG_WARN(TAG, "Failed to set Bluetooth PIN");
    }

    // Set BT as logger output
    Logger::setBTStream(serialBT);
    consoleAuth.configure(BT_CONSOLE_AUTH_ENABLED != 0,
                          BT_CONSOLE_AUTH_PASSWORD_VALUE,
                          BT_CONSOLE_AUTH_TIMEOUT_MS,
                          BT_CONSOLE_AUTH_MAX_ATTEMPTS);
    updateBTOutputGate();
    initialized = true;
    btConnected = false;
    commandBuffer = "";

    LOG_INFO(TAG, "Bluetooth started. Pin: " BT_PASSWORD);
    sendResponse("WiFiManager BT Interface Ready");

    // Register WiFi command handler
    if (wifiCommandHandler == nullptr) {
        wifiCommandHandler = new WiFiManagerCommands();
        registerCommandHandler(wifiCommandHandler);
    }

    printHelp();
}

void BTCommandHandler::update() {
    if (!initialized || serialBT == nullptr) {
        return;
    }

    handleConnectionState();
    if (btConnected) {
        handleAuthTimeout();
    }
    handleAutoStop();

    if (!initialized || serialBT == nullptr) {
        return;
    }

    // Read incoming data
    while (serialBT->available()) {
        char c = serialBT->read();

        if (c == '\r' || c == '\n') {
            if (commandBuffer.length() > 0) {
                if (consoleAuth.allowsConsole()) {
                    parseCommand(commandBuffer);
                } else {
                    handleAuthInput(commandBuffer);
                }
                commandBuffer = "";
            }
        } else if (c >= 32 && c < 127) { // Printable characters
            commandBuffer += c;
            if (consoleAuth.allowsConsole()) {
                serialBT->print(c); // Echo
            }
        }
    }
}

bool BTCommandHandler::isConnected() {
    return initialized && serialBT != nullptr && serialBT->connected();
}

bool BTCommandHandler::isRunning() {
    return initialized && serialBT != nullptr;
}

void BTCommandHandler::setAutoStopHold(bool hold) {
    autoStopHold = hold;

    if (userOverrideActive) {
        if (userOverrideEnabled) {
            begin();
        } else {
            stopBluetooth("Bluetooth forced off by user override");
        }
        return;
    }

    if (autoStopHold) {
        if (!initialized) {
            begin();
        }
        return;
    }

    handleAutoStop();
}

bool BTCommandHandler::setUserOverride(bool enabled) {
    userOverrideActive = true;
    userOverrideEnabled = enabled;

    if (enabled) {
        begin();
        return isRunning();
    }

    stopBluetooth("Bluetooth forced off by user override");
    return !isRunning();
}

bool BTCommandHandler::isUserOverrideActive() {
    return userOverrideActive;
}

bool BTCommandHandler::isUserOverrideEnabled() {
    return userOverrideActive && userOverrideEnabled;
}

BluetoothSerial* BTCommandHandler::getSerialStream() {
    if (!initialized || serialBT == nullptr || !consoleAuth.allowsConsole()) {
        return nullptr;
    }

    return serialBT;
}

void BTCommandHandler::registerCommandHandler(ICommandHandler* handler) {
    if (handler != nullptr) {
        commandHandlers.push_back(handler);
        LOG_DEBUG(TAG, "Command handler registered");
    }
}

void BTCommandHandler::parseCommand(const String &fullCommand) {
    String cmd = fullCommand;
    String args = "";

    // Split command and arguments
    int spacePos = cmd.indexOf(' ');
    if (spacePos > 0) {
        args = cmd.substring(spacePos + 1);
        cmd = cmd.substring(0, spacePos);
    }

    // Convert to lowercase
    cmd.toLowerCase();
    args.trim();

    logConsoleInput(fullCommand);

    executeCommand(cmd, args);
}

void BTCommandHandler::executeCommand(const String &cmd, const String &args) {
    // Check built-in commands first
    if (cmd == "help") {
        cmd_help();
        return;
    } else if (cmd == "reboot") {
        cmd_reboot();
        return;
    }

    // Try registered command handlers
    for (ICommandHandler* handler : commandHandlers) {
        if (handler->handleCommand(cmd, args)) {
            return; // Command was handled
        }
    }

    // Command not found
    sendError("Unknown command: " + cmd + ". Type 'help' for available commands.");
}

void BTCommandHandler::cmd_help() {
    printHelp();
}

void BTCommandHandler::cmd_reboot() {
    sendResponse("Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();
}

void BTCommandHandler::printHelp() {
    String help = R"(
=== WiFiManager BT Command Help ===

)";
    sendResponse(help, false);

    if (wifiCommandHandler != nullptr) {
        sendResponse(wifiCommandHandler->getHelpText(), false);
    }

    String systemHelp = R"(
System:
  help                       - Show this help
  reboot                     - Reboot device

)";
    sendResponse(systemHelp, false);

    // Append help from project/external handlers
    for (ICommandHandler* handler : commandHandlers) {
        if (handler == wifiCommandHandler) {
            continue;
        }
        sendResponse(handler->getHelpText(), false);
    }

    sendResponse("=====================================");
}

void BTCommandHandler::sendResponse(const String &message, bool newline) {
    if (serialBT != nullptr && serialBT->connected() && consoleAuth.allowsConsole()) {
        logConsoleOutput(message, false);
        if (newline) {
            serialBT->println(message);
        } else {
            serialBT->print(message);
        }
    }
}

void BTCommandHandler::sendError(const String &message) {
    if (serialBT != nullptr && serialBT->connected() && consoleAuth.allowsConsole()) {
        logConsoleOutput(message, true);
        serialBT->println("[ERROR] " + message);
    }
}

void BTCommandHandler::handleConnectionState() {
    bool connected = serialBT->connected();

    if (connected) {
        if (!btConnected) {
            btConnected = true;
            commandBuffer = "";
            consoleAuth.beginSession(millis());
            updateBTOutputGate();
            LOG_WARN(TAG, "BT serial client connected");

            if (consoleAuth.isEnabled()) {
                sendAuthPrompt();
            }
        }

        lastHeartbeat = millis();
        return;
    }

    if (btConnected) {
        LOG_WARN(TAG, "BT disconnected");
    }

    btConnected = false;
    commandBuffer = "";
    consoleAuth.reset();
    updateBTOutputGate();
}

void BTCommandHandler::handleAutoStop() {
    if (!initialized || serialBT == nullptr || autoStopHold || noClientTimeoutMs == 0) {
        return;
    }

    if (userOverrideActive && userOverrideEnabled) {
        return;
    }

    if (btConnected || serialBT->connected()) {
        return;
    }

    if (millis() < noClientTimeoutMs) {
        return;
    }

    stopBluetooth("No BT client connected before timeout; stopping Bluetooth");
}

void BTCommandHandler::stopBluetooth(const String& reason) {
    if (!initialized || serialBT == nullptr) {
        return;
    }

    LOG_WARN(TAG, reason);
    serialBT->end();
    Logger::setBTStream(nullptr);
    initialized = false;
    btConnected = false;
    commandBuffer = "";
    lastHeartbeat = 0;
    consoleAuth.reset();
}

void BTCommandHandler::handleAuthInput(const String& input) {
    BTConsoleAuth::Result result = consoleAuth.submitPassword(input, millis());
    updateBTOutputGate();

    switch (result) {
        case BTConsoleAuth::RESULT_SUCCESS:
            Logger::log(Logger::WARN, TAG, "BT console authenticated", false);
            sendResponse("Authenticated");
            sendReadyAndHelp();
            break;
        case BTConsoleAuth::RESULT_FAILED:
            sendAuthMessage("[ERROR] Authentication failed");
            sendAuthPrompt();
            break;
        case BTConsoleAuth::RESULT_LOCKED:
            sendAuthMessage("[ERROR] Authentication locked. Disconnect and reconnect to retry.");
            break;
        case BTConsoleAuth::RESULT_TIMEOUT:
            sendAuthMessage("[ERROR] Authentication timed out. Disconnect and reconnect to retry.");
            break;
        default:
            break;
    }
}

void BTCommandHandler::handleAuthTimeout() {
    BTConsoleAuth::Result result = consoleAuth.checkTimeout(millis());
    if (result == BTConsoleAuth::RESULT_TIMEOUT) {
        updateBTOutputGate();
        sendAuthMessage("[ERROR] Authentication timed out. Disconnect and reconnect to retry.");
    }
}

void BTCommandHandler::sendAuthPrompt() {
    sendAuthMessage("BT console password: ", false);
}

void BTCommandHandler::sendAuthMessage(const String& message, bool newline) {
    if (serialBT == nullptr || !serialBT->connected()) {
        return;
    }

    if (newline) {
        serialBT->println(message);
    } else {
        serialBT->print(message);
    }
}

void BTCommandHandler::sendReadyAndHelp() {
    sendResponse("WiFiManager BT Interface Ready");
    printHelp();
}

void BTCommandHandler::updateBTOutputGate() {
    Logger::setBTOutputAllowed(consoleAuth.allowsConsole());
}

#ifndef ARDUINO_ARCH_ESP32
void BTCommandHandler::resetForTest() {
    if (wifiCommandHandler != nullptr) {
        delete wifiCommandHandler;
        wifiCommandHandler = nullptr;
    }

    commandHandlers.clear();

    if (serialBT != nullptr) {
        delete serialBT;
        serialBT = nullptr;
    }

    initialized = false;
    btConnected = false;
    autoStopHold = false;
    userOverrideActive = false;
    userOverrideEnabled = false;
    commandBuffer = "";
    lastHeartbeat = 0;
    noClientTimeoutMs = BT_NO_CLIENT_TIMEOUT_MS;
    consoleAuth.configure(BT_CONSOLE_AUTH_ENABLED != 0,
                          BT_CONSOLE_AUTH_PASSWORD_VALUE,
                          BT_CONSOLE_AUTH_TIMEOUT_MS,
                          BT_CONSOLE_AUTH_MAX_ATTEMPTS);
    Logger::setBTStream(nullptr);
    Logger::setBTOutputAllowed(true);
}

void BTCommandHandler::configureAuthForTest(bool enabled, const String& password,
                                            unsigned long timeoutMs, int maxAttempts) {
    consoleAuth.configure(enabled, password, timeoutMs, maxAttempts);
    updateBTOutputGate();
}

void BTCommandHandler::configureAutoStopForTest(unsigned long timeoutMs) {
    noClientTimeoutMs = timeoutMs;
}
#endif

void BTCommandHandler::logConsoleInput(const String &fullCommand) {
    String sanitized = sanitizeCommandForLog(fullCommand);
    if (sanitized.length() > 0) {
        Logger::log(Logger::INFO, TAG, "Console IN: " + sanitized, false);
    }
}

void BTCommandHandler::logConsoleOutput(const String &message, bool error) {
    logConsoleText(error ? Logger::WARN : Logger::INFO,
                   error ? "Console ERR: " : "Console OUT: ",
                   message);
}

void BTCommandHandler::logConsoleText(Logger::Level level, const String &prefix, const String &message) {
    if (message.length() == 0) {
        Logger::log(level, TAG, prefix + "<empty>", false);
        return;
    }

    bool loggedAnyLine = false;
    int start = 0;
    int length = message.length();

    for (int i = 0; i <= length; i++) {
        bool atEnd = i == length;
        bool atLineBreak = !atEnd && (message[i] == '\n' || message[i] == '\r');

        if (!atEnd && !atLineBreak) {
            continue;
        }

        if (i > start) {
            Logger::log(level, TAG, prefix + message.substring(start, i), false);
            loggedAnyLine = true;
        }

        start = i + 1;
        while (start < length && (message[start] == '\n' || message[start] == '\r')) {
            start++;
        }
        i = start - 1;
    }

    if (!loggedAnyLine) {
        Logger::log(level, TAG, prefix + "<blank>", false);
    }
}

String BTCommandHandler::sanitizeCommandForLog(const String &fullCommand) {
    String input = fullCommand;
    input.trim();

    if (input.length() == 0) {
        return "";
    }

    int spacePos = input.indexOf(' ');
    String cmd = spacePos > 0 ? input.substring(0, spacePos) : input;
    String args = spacePos > 0 ? input.substring(spacePos + 1) : "";
    cmd.toLowerCase();
    args.trim();

    if (!(cmd == "wifi-set")) {
        return input;
    }

    int indexEnd = args.indexOf(' ');
    if (indexEnd < 0) {
        return cmd + " " + args;
    }

    String index = args.substring(0, indexEnd);
    String rest = args.substring(indexEnd + 1);
    rest.trim();

    int ssidEnd = rest.indexOf(' ');
    if (ssidEnd < 0) {
        return cmd + " " + index + " " + rest;
    }

    String ssid = rest.substring(0, ssidEnd);
    return cmd + " " + index + " " + ssid + " <masked>";
}
