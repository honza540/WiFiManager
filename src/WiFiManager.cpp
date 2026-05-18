#include "WiFiManager.h"
#include "BTCommandHandler.h"

// ============================================================================
// STATIC MEMBER INITIALIZATION - Inicializace statických proměnných
// ============================================================================

// Aktuální stav WiFi manageru (výchozí: inicializace)
WiFiManagerState WiFiManager::state = WM_INIT;

// WebServer objekt pro AP mód (nullptr dokud se nepoužívá)
WebServer* WiFiManager::apModeServer = nullptr;

// Čas spuštění AP módu (pro timeout countdown)
unsigned long WiFiManager::apModeStartTime = 0;

// Čas poslední zmáry připojení
unsigned long WiFiManager::lastConnectionAttempt = 0;

unsigned long WiFiManager::connectionAttemptStartTime = 0;
unsigned long WiFiManager::nextReconnectAttemptTime = 0;
uint8_t WiFiManager::requestedStartIndex = 0;
uint8_t WiFiManager::nextNetworkIndex = 0;
uint8_t WiFiManager::reconnectFailureCount = 0;
bool WiFiManager::connectionSequenceActive = false;
bool WiFiManager::connectedOnce = false;
bool WiFiManager::apSaveReconnectPending = false;
unsigned long WiFiManager::apSaveReconnectAt = 0;
bool WiFiManager::credentialApplyActive = false;
bool WiFiManager::credentialApplyRestoringPrevious = false;
uint8_t WiFiManager::credentialApplyRestoreIndex = WIFI_MAX_CREDENTIALS;
WiFiCredential WiFiManager::credentialApplyRestoreCredential = {"", "", false};
String WiFiManager::activeConnectionSSID = "";

// Index aktuálně připojené WiFi sítě (0-2)
uint8_t WiFiManager::currentNetworkIndex = 0;

// Probíhá WiFi skenování
bool WiFiManager::scanInProgress = false;

// Počet sítí z posledního skenování
int WiFiManager::scanResultCount = 0;

// Tag pro logování
const char* WiFiManager::TAG = "WiFiMgr";

// ============================================================================
// PUBLIC CORE METHODS
// ============================================================================

/**
 * Inicializace WiFi Manageru
 * 
 * Volá se v setup(), provádí:
 * 1. Inicializaci NVS paměti (WiFiStorageManager)
 * 2. Výpis uložených WiFi sítí (pro debug)
 * 3. Pokus o připojení k uloženým sítím
 * 4. Pokud selhá -> spuštění AP módu pro konfiguraci
 */
void WiFiManager::begin() {
    LOG_INFO(TAG, "WiFi Manager v" WIFIMANAGER_VERSION " starting...");

    // Bring up the service console before WiFi starts changing radio state.
    BTCommandHandler::begin();

    // Inicializace NVS paměti (ukládání WiFi dat)
    WiFiStorageManager::begin();

    // Výpis všech uložených WiFi sítí (pro debug)
    WiFiStorageManager::printAllCredentials();

    // Start connection in non-blocking mode. update() will advance the state
    // machine so the main application is not stuck in setup().
    requestReconnect();
}
/**
 * Hlavní update loop
 * 
 * MUSÍ se volat v každé loop() iteraci
 * 
 * Zajišťuje:
 * - Obsluhu webových requestů v AP módu
 * - Kontrolu timeoutu AP módu
 * - Monitoring stavu WiFi připojení
 */
void WiFiManager::update() {
    if (state == WM_AP_MODE) {
        if (apModeServer != nullptr) {
            apModeServer->handleClient();
        }

        if (apSaveReconnectPending && static_cast<int32_t>(millis() - apSaveReconnectAt) >= 0) {
            apSaveReconnectPending = false;
            LOG_WARN(TAG, "WiFi credentials saved, leaving AP setup mode");
            stopAPMode();
            startConnectionSequence();
            return;
        }

        if (millis() - apModeStartTime > (AP_MODE_TIMEOUT * 1000UL)) {
            LOG_WARN(TAG, "AP mode timeout, returning to reconnect loop");
            stopAPMode();
            scheduleReconnect();
        }
    }

    if (state == WM_CONNECTING) {
        if (credentialApplyActive) {
            handleCredentialApplyProgress();
        } else {
            handleConnectionProgress();
        }
    }

    if (state == WM_RETRY_WAIT && millis() - nextReconnectAttemptTime < 0x80000000UL) {
        startConnectionSequence();
    }

    if (state == WM_CONNECTED && WiFi.status() != WL_CONNECTED) {
        LOG_WARN(TAG, "WiFi disconnected!");
        setState(WM_DISCONNECTED);
        scheduleReconnect();
    }
}
/**
 * Zjistit aktuální stav WiFi manageru
 * @return Enum WiFiManagerState
 */
WiFiManagerState WiFiManager::getState() {
    return state;
}
/**
 * Zjistit aktuální stav jako text
 * @return String ("CONNECTED", "AP_MODE", atd.)
 */
String WiFiManager::getStateString() {
    switch (state) {
        case WM_INIT:
            return "INIT";
        case WM_CONNECTING:
            return "CONNECTING";
        case WM_CONNECTED:
            return "CONNECTED";
        case WM_AP_MODE:
            return "AP_MODE";
        case WM_DISCONNECTED:
            return "DISCONNECTED";
        case WM_RETRY_WAIT:
            return "RETRY_WAIT";
        default:
            return "UNKNOWN";
    }
}
/**
 * Kompletní status string pro BT/Web
 * @return "State: CONNECTED | SSID: MyWiFi | IP: 192.168.1.100 | RSSI: -45 dBm"
 */
String WiFiManager::getStatusString() {
    String status = "State: " + getStateString() + " | ";

    // Pokud je připojeno - přidat detaily
    if (state == WM_CONNECTED) {
        status += "SSID: " + String(WiFi.SSID());
        status += " | IP: " + WiFi.localIP().toString();
        status += " | RSSI: " + String(WiFi.RSSI()) + " dBm";
    } 
    // Pokud je AP mód - přidat AP IP
    else if (state == WM_AP_MODE) {
        status += "AP IP: " + WiFi.softAPIP().toString();
    }

    return status;
}
/**
 * Síla signálu - kvalitativní popis
 * RSSI = Received Signal Strength Indicator (v dBm, záporné číslo)
 * Čím víc negativní = horší signál
 * 
 * @return "Excellent" (-50+), "Good" (-60 až -50), "Fair" (-70 až -60),
 *         "Weak" (-80 až -70), "Very Weak" (-80-), "N/A"
 */
String WiFiManager::getSignalStrength() {
    if (state != WM_CONNECTED) {
        return "N/A";
    }

    int rssi = WiFi.RSSI();
    if (rssi > -50) return "Excellent";
    if (rssi > -60) return "Good";
    if (rssi > -70) return "Fair";
    if (rssi > -80) return "Weak";
    return "Very Weak";
}
/**
 * Vrátit aktuální SSID (jméno WiFi sítě)
 * @return SSID nebo "N/A"
 */
String WiFiManager::getSSID() {
    if (state == WM_CONNECTED) {
        return String(WiFi.SSID());
    }
    return "N/A";
}
/**
 * Vrátit aktuální IP adresu
 * @return IP adresa nebo "N/A"
 */
String WiFiManager::getIP() {
    if (state == WM_CONNECTED) {
        return WiFi.localIP().toString();
    }
    if (state == WM_AP_MODE) {
        return WiFi.softAPIP().toString();
    }
    return "N/A";
}
/**
 * Přidat novou WiFi síť do NVS paměti
 * Automaticky se pak zkusí při příštím startu
 * 
 * @param ssid - Jméno sítě
 * @param password - Heslo
 * @param index - Pozice (0-2)
 * @return true = uloženo, false = chyba
 */
bool WiFiManager::addWiFiNetwork(const String &ssid, const String &password, uint8_t index) {
    if (index >= WIFI_MAX_CREDENTIALS) {
        LOG_ERROR(TAG, "Invalid index for WiFi network");
        return false;
    }

    // Uložit přes WiFiStorageManager
    if (WiFiStorageManager::saveCredential(index, ssid, password)) {
        LOG_INFO(TAG, "WiFi network added at index " + String(index));
        return true;
    }
    return false;
}
/**
 * Vrátit ukazatel na WebServer (pro přidání vlastních endpointů)
 * @return Ukazatel na WebServer nebo nullptr
 */
WebServer* WiFiManager::getWebServer() {
    return apModeServer;
}
/**
 * Nastavit nový stav a zalogovat změnu
 * @param newState - Nový stav
 */
void WiFiManager::setState(WiFiManagerState newState) {
    if (state != newState) {
        LOG_DEBUG(TAG, "State change: " + getStateString() + " -> " + 
                 (newState == WM_INIT ? "INIT" : 
                  newState == WM_CONNECTING ? "CONNECTING" :
                  newState == WM_CONNECTED ? "CONNECTED" :
                  newState == WM_AP_MODE ? "AP_MODE" :
                  newState == WM_DISCONNECTED ? "DISCONNECTED" :
                  newState == WM_RETRY_WAIT ? "RETRY_WAIT" : "UNKNOWN"));
        state = newState;
    }
}
