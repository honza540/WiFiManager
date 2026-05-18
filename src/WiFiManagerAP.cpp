#include "WiFiManager.h"
#include "WiFiManagerWebTemplates.h"

// ============================================================================
// AP MODE WEB SERVER
// ============================================================================

/**
 * Spuštění AP módu (přístupový bod)
 * 
 * Vytváří WiFi síť pro konfiguraci zařízení
 * Uživatel se připojí přes mobil/počítač a konfiguruje WiFi přes web
 * 
 * Postup:
 * 1. Generovat jméno (WIFI_AP_SSID_PREFIX + chip ID)
 * 2. Spustit AP s vlastnim WiFi heslem (WIFI_AP_PASSWORD)
 * 3. Vytvořit WebServer na portu 80 s HTML konfigurátorem
 * 4. Čekat na konfiguraci s timeoutem (AP_MODE_TIMEOUT)
 */
void WiFiManager::startAPMode() {
    // Nastavit stav
    setState(WM_AP_MODE);
    apModeStartTime = millis();

    // Generate a unique AP name. The prefix is configurable, but the chip ID
    // keeps several devices in the same room distinguishable.
    String apSSID = String(WIFI_AP_SSID_PREFIX) + String((uint32_t)ESP.getEfuseMac(), HEX);
    String apPassword = WIFI_AP_PASSWORD;
    bool usePassword = apPassword.length() >= 8;

    if (apPassword.length() > 0 && !usePassword) {
        LOG_WARN(TAG, "WIFI_AP_PASSWORD is shorter than 8 chars; starting open setup AP");
    }

    // Nastavit WiFi do AP módu
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(
        apSSID.c_str(),
        usePassword ? apPassword.c_str() : nullptr,
        WIFI_AP_CHANNEL,
        0,
        WIFI_AP_MAX_CLIENTS
    );

    if (!apStarted) {
        LOG_ERROR(TAG, "Failed to start AP mode");
        setState(WM_DISCONNECTED);
        scheduleReconnect();
        return;
    }

    // Logování informací
    LOG_INFO(TAG, "AP Mode started");
    LOG_INFO(TAG, "SSID: " + apSSID);
    LOG_INFO(TAG, "AP MAC: " + WiFi.softAPmacAddress());
    LOG_INFO(TAG, usePassword ? "Security: WPA/WPA2" : "Security: open AP");
    LOG_INFO(TAG, "IP: " + WiFi.softAPIP().toString());
    LOG_INFO(TAG, "Timeout: " + String(AP_MODE_TIMEOUT) + " seconds");

    // Inicializace WebServeru
    if (apModeServer != nullptr) {
        delete apModeServer;  // Smazat starý server pokud existuje
    }
    apModeServer = new WebServer(WEB_SERVER_PORT);

    // Registrace endpointů (URL cesty které webserver obsluhuje)
    apModeServer->on("/", HTTP_GET, []() { handleAPModeRoot(); });
    apModeServer->on("/config", HTTP_GET, []() { handleAPModeConfig(); });
    apModeServer->on("/status", HTTP_GET, []() { handleAPModeStatus(); });
    apModeServer->on("/scan", HTTP_GET, []() { handleAPModeScan(); });
    apModeServer->on("/save", HTTP_POST, []() { handleAPModeSave(); });

    // Spustit WebServer
    apModeServer->begin();
}
/**
 * Vypnutí AP módu
 */
void WiFiManager::stopAPMode() {
    if (apModeServer != nullptr) {
        apModeServer->stop();
        delete apModeServer;
        apModeServer = nullptr;
    }
    WiFi.softAPdisconnect(true);
    setState(WM_DISCONNECTED);
    LOG_INFO(TAG, "AP Mode stopped");
}
/**
 * GET / - Hlavní stránka s konfigurační formulář
 * Uživatel zde zadá SSID, heslo a pozici
 */
void WiFiManager::handleAPModeRoot() {
    apModeServer->send(200, "text/html", WiFiManagerWebTemplates::setupPage());
}
void WiFiManager::handleAPModeConfig() {
    // Vyhrazeno pro budoucí použití
    apModeServer->send(200, "text/plain", "Config endpoint");
}
void WiFiManager::handleAPModeStatus() {
    String status = "WiFi Manager Status\n";
    status += "State: " + getStateString() + "\n";
    status += "Credentials stored: " + String(WiFiStorageManager::getCredentialCount()) + "\n";
    apModeServer->send(200, "text/plain", status);
}
void WiFiManager::handleAPModeScan() {
    if (!scanInProgress) {
        scanNetworks(true);
    }

    int count = getScanResultCount();
    if (count == -2) {
        apModeServer->send(202, "text/plain", "Scan running, refresh /scan in a few seconds");
        return;
    }

    String result = "Available Networks:\n";
    if (count < 0) {
        result += "Scan failed";
    } else if (count == 0) {
        result += "No networks found";
    } else {
        for (int i = 0; i < count; i++) {
            result += String(i + 1) + ". " + getScanResult(i) + "\n";
        }
    }

    WiFi.scanDelete();
    WiFi.mode(WIFI_AP);
    apModeServer->send(200, "text/plain", result);
}
/**
 * POST /save - Uložení nové WiFi sítě
 * Čeká na SSID, password a index
 */
void WiFiManager::handleAPModeSave() {
    // Kontrola že máme potřebné parametry
    if (!apModeServer->hasArg("ssid") || !apModeServer->hasArg("pass")) {
        apModeServer->send(400, "text/plain", "Missing SSID or password");
        return;
    }

    // Parsování parametrů z HTTP requestu
    String ssid = apModeServer->arg("ssid");
    String pass = apModeServer->arg("pass");
    uint8_t idx = apModeServer->hasArg("idx") ? apModeServer->arg("idx").toInt() : 0;

    // Uložení do NVS
    if (addWiFiNetwork(ssid, pass, idx)) {
        apModeServer->send(200, "text/html", WiFiManagerWebTemplates::savedPage());
        apSaveReconnectPending = true;
        apSaveReconnectAt = millis() + 1000;
    } else {
        // Chyba
        apModeServer->send(400, "text/plain", "Failed to save credentials");
    }
}
