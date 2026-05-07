#include "WiFiManagerCommands.h"
#include "BTCommandHandler.h"
#include "Logger.h"

const char* WiFiManagerCommands::TAG = "WiFiCmd";

WiFiManagerCommands::WiFiManagerCommands() {
    LOG_DEBUG(TAG, "WiFiManagerCommands initialized");
}

bool WiFiManagerCommands::handleCommand(const String& cmd, const String& args) {
    if (cmd == "wifi-set") {
        cmd_wifiset(args);
        return true;
    } else if (cmd == "wifi-clear") {
        cmd_wificlear(args);
        return true;
    } else if (cmd == "wifi-list") {
        cmd_wifilist();
        return true;
    } else if (cmd == "netconnect") {
        cmd_netconnect(args);
        return true;
    } else if (cmd == "status") {
        cmd_status();
        return true;
    } else if (cmd == "netstatus") {
        cmd_netstatus();
        return true;
    } else if (cmd == "netmonitor") {
        cmd_netmonitor();
        return true;
    } else if (cmd == "live") {
        cmd_live();
        return true;
    }
    return false;
}

String WiFiManagerCommands::getHelpText() {
    return R"(
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
)";
}

void WiFiManagerCommands::cmd_wifiset(const String &args) {
    // Format: wifi-set <index> <ssid> <password>
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

void WiFiManagerCommands::cmd_wificlear(const String &args) {
    uint8_t index = args.toInt();
    WiFiStorageManager::clearCredential(index);
    sendResponse("WiFi credential at index " + String(index) + " cleared");
}

void WiFiManagerCommands::cmd_wifilist() {
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

void WiFiManagerCommands::cmd_netconnect(const String &args) {
    uint8_t index = args.toInt();
    
    if (index >= WiFiStorageManager::getCredentialCount()) {
        sendError("Invalid network index");
        return;
    }

    sendResponse("Attempting to connect to network index " + String(index) + "...");
    // Note: Implement actual reconnection logic if needed
    sendResponse("Reconnection logic not yet implemented");
}

void WiFiManagerCommands::cmd_status() {
    sendResponse("\n=== System Status ===");
    sendResponse("Firmware: " FIRMWARE_VERSION);
    sendResponse("WiFi Manager: " WIFIMANAGER_VERSION);
    sendResponse("WiFi: " + WiFiManager::getStatusString());
    sendResponse("=====================");
}

void WiFiManagerCommands::cmd_netstatus() {
    sendResponse("\n=== Network Status ===");
    sendResponse("State: " + WiFiManager::getStateString());
    sendResponse("SSID: " + WiFiManager::getSSID());
    sendResponse("IP: " + WiFiManager::getIP());
    if (WiFiManager::getState() == WM_AP_MODE) {
        sendResponse("MAC: " + WiFi.softAPmacAddress());
    } else {
        sendResponse("MAC: " + WiFi.macAddress());
    }
    sendResponse("Signal: " + WiFiManager::getSignalStrength());

    if (WiFiManager::getState() == WM_CONNECTED) {
        int rssi = WiFi.RSSI();
        sendResponse("RSSI: " + String(rssi) + " dBm");
        sendResponse("Channel: " + String(WiFi.channel()));
    }
    sendResponse("======================");
}

void WiFiManagerCommands::cmd_netmonitor() {
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

void WiFiManagerCommands::cmd_live() {
    sendResponse("Live logger streaming started. Type 'exit' to stop.");
    // Logger is already streaming
}

void WiFiManagerCommands::sendResponse(const String &message, bool newline) {
    BluetoothSerial* btSerial = BTCommandHandler::getSerialStream();
    if (btSerial != nullptr && btSerial->connected()) {
        if (newline) {
            btSerial->println(message);
        } else {
            btSerial->print(message);
        }
    }
}

void WiFiManagerCommands::sendError(const String &message) {
    BluetoothSerial* btSerial = BTCommandHandler::getSerialStream();
    if (btSerial != nullptr && btSerial->connected()) {
        btSerial->println("[ERROR] " + message);
    }
}
