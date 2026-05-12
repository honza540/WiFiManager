#ifndef BT_COMMAND_HANDLER_H
#define BT_COMMAND_HANDLER_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <vector>
#include "config.h"
#include "Logger.h"
#include "ICommandHandler.h"

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

    // Get BT serial stream for logger and external use
    static BluetoothSerial* getSerialStream();

    // Register external command handler (e.g., WiFiManagerCommands or project commands)
    static void registerCommandHandler(ICommandHandler* handler);

    // Send response via BT (used by command handlers)
    static void sendResponse(const String &message, bool newline = true);
    
    // Send error via BT (used by command handlers)
    static void sendError(const String &message);

private:
    static BluetoothSerial* serialBT;
    static bool initialized;
    static bool btConnected;
    static String commandBuffer;
    static unsigned long lastHeartbeat;
    static const char* TAG;
    static std::vector<ICommandHandler*> commandHandlers;

    // Command parsing and execution
    static void parseCommand(const String &command);
    static void executeCommand(const String &cmd, const String &args);

    // Built-in command handlers
    static void cmd_help();
    static void cmd_reboot();

    // Helper methods
    static void printHelp();
};

#endif // BT_COMMAND_HANDLER_H
