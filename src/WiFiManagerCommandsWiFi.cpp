#include "WiFiManagerCommands.h"

// ============================================================================
// WIFI CONFIGURATION COMMANDS
// ============================================================================

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

    if (WiFiManager::addWiFiNetworkAndConnect(ssid, password, index)) {
        sendResponse("WiFi credentials saved at index " + String(index));
        sendResponse("SSID: " + ssid);
        sendResponse("Connecting to WiFi [" + String(index) + "]: " + ssid);
        sendResponse("Timeout: " + String(WIFI_CONNECT_TIMEOUT / 1000) + "s");
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
    WiFiManager::requestReconnect(index);
    sendResponse("Reconnect requested. Use netstatus to watch progress.");
}
