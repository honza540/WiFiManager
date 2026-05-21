#ifndef BT_COMMAND_HANDLER_H
#define BT_COMMAND_HANDLER_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <vector>
#include "config.h"
#include "Logger.h"
#include "ICommandHandler.h"
#include "BTConsoleAuth.h"

/**
 * Bluetooth Command Handler
 * Central coordinator for BT commands
 * 
 * Supports registration of command handlers from different modules
 * Built-in commands:
 *  - help   : Show available commands
 *  - reboot : Reboot the device
 * 
 * Additional commands can be registered via registerCommandHandler()
 */

class BTCommandHandler {
public:
    // Initialize Bluetooth
    static void begin();

    // Main update loop - must be called in main loop
    static void update();

    // Check if BT is connected
    static bool isConnected();

    // Check if the Bluetooth service is currently running
    static bool isRunning();

    // Keep BT available while another module needs it, such as AP setup mode.
    static void setAutoStopHold(bool hold);

    // Get BT serial stream for external use (nullptr until console auth passes)
    static BluetoothSerial* getSerialStream();

    // Register external command handler (e.g., WiFiManagerCommands or project commands)
    static void registerCommandHandler(ICommandHandler* handler);

    // Send response via BT (used by command handlers)
    static void sendResponse(const String &message, bool newline = true);
    
    // Send error via BT (used by command handlers)
    static void sendError(const String &message);

#ifndef ARDUINO_ARCH_ESP32
    static void resetForTest();
    static BluetoothSerial* getRawSerialStreamForTest();
    static void configureAuthForTest(bool enabled, const String& password,
                                     unsigned long timeoutMs = BT_CONSOLE_AUTH_TIMEOUT_MS,
                                     int maxAttempts = BT_CONSOLE_AUTH_MAX_ATTEMPTS);
    static void configureAutoStopForTest(unsigned long timeoutMs);
#endif

private:
    static BluetoothSerial* serialBT;
    static bool initialized;
    static bool btConnected;
    static bool autoStopHold;
    static String commandBuffer;
    static unsigned long lastHeartbeat;
    static unsigned long noClientTimeoutMs;
    static const char* TAG;
    static std::vector<ICommandHandler*> commandHandlers;
    static ICommandHandler* wifiCommandHandler;
    static BTConsoleAuth consoleAuth;

    // Command parsing and execution
    static void parseCommand(const String &command);
    static void executeCommand(const String &cmd, const String &args);

    // Built-in command handlers
    static void cmd_help();
    static void cmd_reboot();

    // Helper methods
    static void printHelp();
    static void handleConnectionState();
    static void handleAutoStop();
    static void stopBluetooth(const String& reason);
    static void handleAuthInput(const String& input);
    static void handleAuthTimeout();
    static void sendAuthPrompt();
    static void sendAuthMessage(const String& message, bool newline = true);
    static void sendReadyAndHelp();
    static void updateBTOutputGate();
    static void logConsoleInput(const String &fullCommand);
    static void logConsoleOutput(const String &message, bool error);
    static void logConsoleText(Logger::Level level, const String &prefix, const String &message);
    static String sanitizeCommandForLog(const String &fullCommand);
};

#endif // BT_COMMAND_HANDLER_H
