#ifndef BT_COMMAND_HANDLER_H
#define BT_COMMAND_HANDLER_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include "config.h"
#include "Logger.h"
#include "WiFiManager.h"
#include "WiFiStorageManager.h"

/**
 * Bluetooth Command Handler
 * Provides BT interface for WiFi and system configuration
 * 
 * Commands:
 * help              - Show available commands
 * status            - Current WiFi and system status
 * netstatus         - Detailed network status (IP, RSSI, etc.)
 * netmonitor        - List available WiFi networks
 * wifi-set <idx> <ssid> <password> - Set WiFi credentials
 * wifi-clear <idx>  - Clear WiFi credentials
 * wifi-list         - List stored WiFi credentials
 * netconnect <idx>  - Connect to specific WiFi
 * live              - Stream logger output to BT
 * relaystatus       - Show relay configuration
 * relayset          - Set relay intervals (future)
 * start             - Start filter (future)
 * stop              - Stop filter (future)
 * relayreset        - Reset relay to defaults (future)
 * reboot            - Reboot the device
 */

class BTCommandHandler {
public:
    // Initialize Bluetooth
    static void begin();

    // Main update loop - must be called in main loop
    static void update();

    // Check if BT is connected
    static bool isConnected();

    // Get BT serial stream for logger
    static BluetoothSerial* getSerialStream();

private:
    static BluetoothSerial* serialBT;
    static bool btConnected;
    static String commandBuffer;
    static unsigned long lastHeartbeat;
    static const char* TAG;

    // Command parsing and execution
    static void parseCommand(const String &command);
    static void executeCommand(const String &cmd, const String &args);

    // Command handlers
    static void cmd_help();
    static void cmd_status();
    static void cmd_netstatus();
    static void cmd_netmonitor();
    static void cmd_wifiset(const String &args);
    static void cmd_wificlear(const String &args);
    static void cmd_wifilist();
    static void cmd_netconnect(const String &args);
    static void cmd_live();
    static void cmd_relaystatus();
    static void cmd_relayset(const String &args);
    static void cmd_start();
    static void cmd_stop();
    static void cmd_relayreset();
    static void cmd_reboot();

    // Helper methods
    static void printHelp();
    static String formatWiFiStatus();
    static void sendResponse(const String &message, bool newline = true);
    static void sendError(const String &message);
    static void sendOK();
};

#endif // BT_COMMAND_HANDLER_H
