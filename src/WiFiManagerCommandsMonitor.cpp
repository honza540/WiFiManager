#include "WiFiManagerCommands.h"

// ============================================================================
// MONITORING COMMANDS
// ============================================================================

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
    WiFiManager::scanNetworks(true);

    unsigned long startTime = millis();
    while (WiFiManager::getScanResultCount() == -2 && millis() - startTime < WIFI_SCAN_TIMEOUT) {
        delay(100);
    }

    int count = WiFiManager::getScanResultCount();
    if (count <= 0) {
        sendResponse("No networks found or scan failed");
        WiFi.scanDelete();
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
    WiFi.scanDelete();
}
void WiFiManagerCommands::cmd_live() {
    sendResponse("Live logger streaming started. Type 'exit' to stop.");
    // Logger is already streaming
}
