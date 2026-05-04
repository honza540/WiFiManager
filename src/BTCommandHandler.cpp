#include "BTCommandHandler.h"

// Static member initialization
BluetoothSerial* BTCommandHandler::serialBT = nullptr;
bool BTCommandHandler::btConnected = false;
String BTCommandHandler::commandBuffer = "";
unsigned long BTCommandHandler::lastHeartbeat = 0;
const char* BTCommandHandler::TAG = "BT";

void BTCommandHandler::begin() {
    serialBT = new BluetoothSerial();

    LOG_INFO(TAG, "Starting Bluetooth with name: " BT_DEVICE_NAME);

    if (!serialBT->begin(BT_DEVICE_NAME, true)) { // Master mode
        LOG_ERROR(TAG, "Failed to start Bluetooth");
        return;
    }

    // Set BT as logger output
    Logger::setBTStream(serialBT);

    LOG_INFO(TAG, "Bluetooth started. Pin: " BT_PASSWORD);
    sendResponse("PoolFilter BT Interface Ready");
    printHelp();
}

void BTCommandHandler::update() {
    if (serialBT == nullptr) {
        return;
    }

    // Check connection status
    if (serialBT->connected()) {
        btConnected = true;
        lastHeartbeat = millis();
    } else {
        if (btConnected) {
            LOG_WARN(TAG, "BT disconnected");
            btConnected = false;
        }
    }

    // Read incoming data
    while (serialBT->available()) {
        char c = serialBT->read();

        if (c == '\r' || c == '\n') {
            if (commandBuffer.length() > 0) {
                parseCommand(commandBuffer);
                commandBuffer = "";
            }
        } else if (c >= 32 && c < 127) { // Printable characters
            commandBuffer += c;
            serialBT->print(c); // Echo
        }
    }
}

bool BTCommandHandler::isConnected() {
    return serialBT != nullptr && serialBT->connected();
}

BluetoothSerial* BTCommandHandler::getSerialStream() {
    return serialBT;
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

    LOG_DEBUG(TAG, "Command: " + cmd + " | Args: " + args);

    executeCommand(cmd, args);
}

void BTCommandHandler::executeCommand(const String &cmd, const String &args) {
    if (cmd == "help") {
        cmd_help();
    } else if (cmd == "status") {
        cmd_status();
    } else if (cmd == "netstatus") {
        cmd_netstatus();
    } else if (cmd == "netmonitor") {
        cmd_netmonitor();
    } else if (cmd == "wifi-set") {
        cmd_wifiset(args);
    } else if (cmd == "wifi-clear") {
        cmd_wificlear(args);
    } else if (cmd == "wifi-list") {
        cmd_wifilist();
    } else if (cmd == "netconnect") {
        cmd_netconnect(args);
    } else if (cmd == "live") {
        cmd_live();
    } else if (cmd == "relaystatus") {
        cmd_relaystatus();
    } else if (cmd == "relayset") {
        cmd_relayset(args);
    } else if (cmd == "start") {
        cmd_start();
    } else if (cmd == "stop") {
        cmd_stop();
    } else if (cmd == "relayreset") {
        cmd_relayreset();
    } else if (cmd == "reboot") {
        cmd_reboot();
    } else {
        sendError("Unknown command: " + cmd + ". Type 'help' for available commands.");
    }
}

// Command implementations
void BTCommandHandler::cmd_help() {
    printHelp();
}

void BTCommandHandler::cmd_status() {
    sendResponse("\n=== System Status ===");
    sendResponse("Firmware: " FIRMWARE_VERSION);
    sendResponse("WiFi Manager: " WIFIMANAGER_VERSION);
    sendResponse(formatWiFiStatus());
    sendResponse("Relay Status: Not implemented yet");
    sendResponse("=====================");
}

void BTCommandHandler::cmd_netstatus() {
    sendResponse("\n=== Network Status ===");
    sendResponse("State: " + WiFiManager::getStateString());
    sendResponse("SSID: " + WiFiManager::getSSID());
    sendResponse("IP: " + WiFiManager::getIP());
    sendResponse("Signal: " + WiFiManager::getSignalStrength());

    if (WiFiManager::getState() == WM_CONNECTED) {
        int rssi = WiFi.RSSI();
        sendResponse("RSSI: " + String(rssi) + " dBm");
        sendResponse("Channel: " + String(WiFi.channel()));
    }
    sendResponse("======================");
}

void BTCommandHandler::cmd_netmonitor() {
    sendResponse("\nScanning WiFi networks...");
    WiFiManager::scanNetworks(false);
    delay(1000);

    int count = WiFiManager::getScanResultCount();
    if (count <= 0) {
        sendResponse("No networks found or scan failed");
        return;
    }

    sendResponse("Found " + String(count) + " networks:");
    for (int i = 0; i < count; i++) {
        String result = WiFiManager::getScanResult(i);
        if (result.length() > 0) {
            int pipe1 = result.indexOf('|');
            int pipe2 = result.lastIndexOf('|');
            String ssid = result.substring(0, pipe1);
            String rssi = result.substring(pipe1 + 1, pipe2);
            String enc = result.substring(pipe2 + 1);
            sendResponse("[" + String(i) + "] " + ssid + " | RSSI: " + rssi + " | " + enc);
        }
    }
}

void BTCommandHandler::cmd_wifiset(const String &args) {
    // Format: wifi-set <index> <ssid> <password>
    // Example: wifi-set 0 MyNetwork MyPassword123

    int space1 = args.indexOf(' ');
    if (space1 < 0) {
        sendError("Usage: wifi-set <index> <ssid> <password>");
        return;
    }

    uint8_t index = args.substring(0, space1).toInt();

    int space2 = args.indexOf(' ', space1 + 1);
    if (space2 < 0) {
        sendError("Usage: wifi-set <index> <ssid> <password>");
        return;
    }

    String ssid = args.substring(space1 + 1, space2);
    String password = args.substring(space2 + 1);

    if (WiFiManager::addWiFiNetwork(ssid, password, index)) {
        sendResponse("WiFi credentials saved at index " + String(index));
        sendResponse("SSID: " + ssid);
        sendResponse("Consider rebooting to apply changes.");
    } else {
        sendError("Failed to save WiFi credentials");
    }
}

void BTCommandHandler::cmd_wificlear(const String &args) {
    uint8_t index = args.toInt();
    WiFiStorageManager::clearCredential(index);
    sendResponse("WiFi credential at index " + String(index) + " cleared");
}

void BTCommandHandler::cmd_wifilist() {
    sendResponse("\n=== Stored WiFi Networks ===");
    uint8_t count = WiFiStorageManager::getCredentialCount();

    if (count == 0) {
        sendResponse("No WiFi networks stored");
        return;
    }

    for (uint8_t i = 0; i < count; i++) {
        WiFiCredential cred = WiFiStorageManager::loadCredential(i);
        if (cred.valid) {
            sendResponse("[" + String(i) + "] " + cred.ssid);
        }
    }
    sendResponse("=============================");
}

void BTCommandHandler::cmd_netconnect(const String &args) {
    uint8_t index = args.toInt();
    
    if (index >= WiFiStorageManager::getCredentialCount()) {
        sendError("Invalid network index");
        return;
    }

    sendResponse("Attempting to connect to network index " + String(index) + "...");
    // Note: Implement actual reconnection logic if needed
    sendResponse("Reconnection logic not yet implemented");
}

void BTCommandHandler::cmd_live() {
    sendResponse("Live logger streaming started. Type 'exit' to stop.");
    // Logger is already streaming, just inform user
}

void BTCommandHandler::cmd_relaystatus() {
    sendResponse("Relay status not yet implemented");
}

void BTCommandHandler::cmd_relayset(const String &args) {
    sendResponse("Relay configuration not yet implemented");
}

void BTCommandHandler::cmd_start() {
    sendResponse("Filter start not yet implemented");
}

void BTCommandHandler::cmd_stop() {
    sendResponse("Filter stop not yet implemented");
}

void BTCommandHandler::cmd_relayreset() {
    sendResponse("Relay reset not yet implemented");
}

void BTCommandHandler::cmd_reboot() {
    sendResponse("Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();
}

void BTCommandHandler::printHelp() {
    String help = R"(
=== PoolFilter BT Command Help ===

WiFi Configuration:
  wifi-set <idx> <ssid> <pass>  - Save WiFi credentials
  wifi-clear <idx>              - Clear WiFi at index
  wifi-list                     - List stored networks
  netconnect <idx>              - Connect to stored network

Monitoring:
  status                        - System status
  netstatus                     - Network details
  netmonitor                   - Scan available networks
  live                         - Stream logger output

Relay Control (future):
  relaystatus                  - Show relay config
  relayset                     - Configure relay intervals
  start                        - Start filtering
  stop                         - Stop filtering
  relayreset                   - Reset to defaults

System:
  reboot                       - Reboot device
  help                         - Show this help

==================================
)";
    sendResponse(help);
}

String BTCommandHandler::formatWiFiStatus() {
    return "WiFi: " + WiFiManager::getStatusString();
}

void BTCommandHandler::sendResponse(const String &message, bool newline) {
    if (serialBT != nullptr && serialBT->connected()) {
        if (newline) {
            serialBT->println(message);
        } else {
            serialBT->print(message);
        }
    }
}

void BTCommandHandler::sendError(const String &message) {
    if (serialBT != nullptr && serialBT->connected()) {
        serialBT->println("[ERROR] " + message);
    }
}

void BTCommandHandler::sendOK() {
    if (serialBT != nullptr && serialBT->connected()) {
        serialBT->println("OK");
    }
}
