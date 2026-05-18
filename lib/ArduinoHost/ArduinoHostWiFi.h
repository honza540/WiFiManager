#ifndef ARDUINO_HOST_WIFI_H
#define ARDUINO_HOST_WIFI_H

#include "ArduinoHostIO.h"

// ============================================================================
// WIFI STUB
// ============================================================================

const int WIFI_STA = 1;
const int WIFI_AP = 2;
const int WL_CONNECTED = 3;
const int WIFI_SCAN_FAILED = -1;
const int WIFI_SCAN_RUNNING = -2;
const int WIFI_AUTH_OPEN = 0;
const int WIFI_AUTH_WPA_PSK = 4;

class WiFiClass {
public:
    WiFiClass() : connected(true), scanResultCount(0) {}

    void mode(int newMode) { currentMode = newMode; }
    void begin(const char* ssid, const char* password) {
        lastBeginSSID = ssid ? ssid : "";
        lastBeginPassword = password ? password : "";
        currentSSID = lastBeginSSID;
        connected = connectOnBegin;
    }
    int status() const { return connected ? WL_CONNECTED : 0; }
    int RSSI() const { return -50; }
    int RSSI(int) const { return -50; }
    int channel() const { return 1; }
    bool softAP(const char* ssid, const char* password, int = 1, int = 0, int = 4, bool = false) {
        lastSoftAPSSID = ssid ? ssid : "";
        lastSoftAPPassword = password ? password : "";
        return softAPResult;
    }
    IPAddress softAPIP() const { return IPAddress(192,168,4,1); }
    void softAPdisconnect(bool) {}
    void disconnect(bool) { connected = false; }
    void scanNetworks(bool) { scanResultCount = 0; }
    int scanComplete() { return scanResultCount; }
    void scanDelete() {}
    String SSID() const { return currentSSID.length() > 0 ? currentSSID : String("MockSSID"); }
    String SSID(int) const { return "MockSSID"; }
    IPAddress localIP() const { return IPAddress(192,168,1,100); }
    String macAddress() const { return "AA:BB:CC:DD:EE:FF"; }
    String softAPmacAddress() const { return "FF:EE:DD:CC:BB:AA"; }
    IPAddress subnetMask() const { return IPAddress(255,255,255,0); }
    IPAddress gatewayIP() const { return IPAddress(192,168,1,1); }
    IPAddress dnsIP() const { return IPAddress(8,8,8,8); }
    uint8_t encryptionType(int) const { return WIFI_AUTH_OPEN; }

    void resetMock() {
        connected = true;
        connectOnBegin = true;
        softAPResult = true;
        currentMode = 0;
        scanResultCount = 0;
        currentSSID = "";
        lastBeginSSID = "";
        lastBeginPassword = "";
        lastSoftAPSSID = "";
        lastSoftAPPassword = "";
    }
    void setConnectOnBegin(bool value) {
        connectOnBegin = value;
    }
    void setConnected(bool value) {
        connected = value;
    }
    void setSoftAPResult(bool value) {
        softAPResult = value;
    }
    int getMode() const {
        return currentMode;
    }
    String getLastBeginSSID() const {
        return lastBeginSSID;
    }
    String getLastBeginPassword() const {
        return lastBeginPassword;
    }
    String getLastSoftAPSSID() const {
        return lastSoftAPSSID;
    }
    String getLastSoftAPPassword() const {
        return lastSoftAPPassword;
    }

private:
    bool connected;
    bool connectOnBegin = true;
    bool softAPResult = true;
    int currentMode = 0;
    int scanResultCount;
    String currentSSID = "";
    String lastBeginSSID = "";
    String lastBeginPassword = "";
    String lastSoftAPSSID = "";
    String lastSoftAPPassword = "";
};

extern WiFiClass WiFi;

#endif // ARDUINO_HOST_WIFI_H
