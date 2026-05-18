#include "WiFiManager.h"
#include "BTCommandHandler.h"
#include "WiFiManagerWebTemplates.h"
#include <WebServer.h>

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
// PUBLIC METHODS - Veřejné metody
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

void WiFiManager::requestReconnect() {
    requestReconnect(0);
}

void WiFiManager::requestReconnect(uint8_t startIndex) {
    if (state == WM_AP_MODE) {
        stopAPMode();
    }

    credentialApplyActive = false;
    credentialApplyRestoringPrevious = false;
    requestedStartIndex = startIndex < WIFI_MAX_CREDENTIALS ? startIndex : 0;
    reconnectFailureCount = 0;
    nextReconnectAttemptTime = millis();
    startConnectionSequence();
}

/**
 * Pokus o připojení k uloženým WiFi sítím
 * 
 * Algoritmus:
 * 1. Načíst všechny uložené sítě z NVS
 * 2. Zkusit připojit se k prvnímu síti po dobu WIFI_CONNECT_TIMEOUT ms
 * 3. Pokud se nepovede, zkusit druhou, pak třetí atd.
 * 4. Jakmile se podaří připojit -> zalogovat detaily a vrátit true
 * 5. Pokud se všechny pokusy nezdaří -> vrátit false
 * 
 * @return true = úspěšně připojeno, false = všechny pokusy selhaly
 */
bool WiFiManager::connectToStoredNetwork() {
    // Nastavit stav na "probíhá připojování"
    setState(WM_CONNECTING);

    // Načíst všechny uložené WiFi sítě z NVS
    uint8_t count = 0;
    WiFiCredential* credentials = WiFiStorageManager::loadAllCredentials(count);

    // Pokud nemáme žádné uložené sítě, vrátit failure
    if (count == 0) {
        LOG_WARN(TAG, "No stored WiFi credentials");
    }

    // Zkusit každou síť podle priority
    for (uint8_t i = 0; i < count; i++) {
        // Přeskočit nevalidní zápisy
        if (!credentials[i].valid) continue;

        // Logování pokusu
        LOG_INFO(TAG, "Attempting to connect to: " + credentials[i].ssid);

        // Nastavení WiFi na STA mód (station = client)
        WiFi.mode(WIFI_STA);

        // Spuštění připojování
        WiFi.begin(credentials[i].ssid.c_str(), credentials[i].password.c_str());

        // Čekání na připojení s timeoutem
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            // Čekat 100ms a tisk "." (progress indicator)
            if (millis() - startTime > WIFI_CONNECT_TIMEOUT) {
                // Timeout - tato síť je nedostupná
                LOG_WARN(TAG, "Connection timeout for: " + credentials[i].ssid);
                break;
            }
            delay(100);
            Serial.print(".");
        }

        // Kontrola: Podařilo se připojit?
        if (WiFi.status() == WL_CONNECTED) {
            // ÚSPĚCH!
            Serial.println();  // Nový řádek po tečkách
            currentNetworkIndex = i;  // Pamatovat si kterou síť jsme právě připojili
            connectedOnce = true;
            reconnectFailureCount = 0;
            printNetworkInfo();  // Vytisknout detaily (IP, Gateway, atd.)
            setState(WM_CONNECTED);  // Nastavit stav
            delete[] credentials;  // Dealokovat paměť
            return true;
        }
    }

    // FAILURE - žádná síť nebyla dostupná
    delete[] credentials;

    if (String(WIFI_FALLBACK_SSID).length() > 0) {
        LOG_WARN(TAG, "Stored WiFi unavailable, trying fixed fallback: " WIFI_FALLBACK_SSID);
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_FALLBACK_SSID, WIFI_FALLBACK_PASSWORD);

        unsigned long fallbackStartTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - fallbackStartTime > WIFI_CONNECT_TIMEOUT) {
                LOG_WARN(TAG, "Connection timeout for fallback: " WIFI_FALLBACK_SSID);
                break;
            }
            delay(100);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            currentNetworkIndex = WIFI_MAX_CREDENTIALS;
            connectedOnce = true;
            reconnectFailureCount = 0;
            printNetworkInfo();
            setState(WM_CONNECTED);
            return true;
        }
    } else {
        LOG_WARN(TAG, "No fixed fallback WiFi configured");
    }

    LOG_ERROR(TAG, "Failed to connect to any stored or fallback network");
    setState(WM_DISCONNECTED);
    return false;
}

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
 * Skenování dostupných WiFi sítí
 * Výsledky si pak můžeš přečíst přes getScanResult()
 * 
 * @param async - true = skenování v pozadí (bez čekání)
 *                false = blokující (čeká na výsledek)
 */
void WiFiManager::scanNetworks(bool async) {
    if (scanInProgress) {
        return;  // Zabránit duplikátům
    }

    LOG_INFO(TAG, "Starting WiFi scan...");
    scanInProgress = true;
    scanResultCount = WIFI_SCAN_RUNNING;
    WiFi.scanNetworks(async);
}

/**
 * Zjistit počet sítí z posledního skenování
 * @return Počet sítí, -1 = chyba, -2 = probíhá skenování
 */
int WiFiManager::getScanResultCount() {
    scanResultCount = WiFi.scanComplete();
    if (scanResultCount == WIFI_SCAN_FAILED) {
        scanInProgress = false;
        return -1;  // Chyba
    }
    if (scanResultCount == WIFI_SCAN_RUNNING) {
        return -2;  // Stále probíhá
    }
    scanInProgress = false;
    return scanResultCount;
}

/**
 * Vrátit informace o jedné WiFi síti ze skenování
 * 
 * @param index - Pozice (0-n)
 * @return String formátu: "SSID|RSSI|ENCRYPTION"
 *         Příklad: "MyWiFi|-45|Secured"
 */
String WiFiManager::getScanResult(int index) {
    int count = getScanResultCount();
    if (count <= 0 || index >= count) {
        return "";
    }

    // Načíst informace o síti
    String ssid = WiFi.SSID(index);
    int rssi = WiFi.RSSI(index);
    uint8_t sec = WiFi.encryptionType(index);
    String encryption = (sec == WIFI_AUTH_OPEN) ? "Open" : "Secured";

    // Vrátit ve formátu "SSID|RSSI|Encryption"
    return ssid + "|" + String(rssi) + "|" + encryption;
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

bool WiFiManager::addWiFiNetworkAndConnect(const String &ssid, const String &password, uint8_t index) {
    if (index >= WIFI_MAX_CREDENTIALS) {
        LOG_ERROR(TAG, "Invalid index for WiFi network");
        return false;
    }

    uint8_t restoreIndex = WIFI_MAX_CREDENTIALS;
    WiFiCredential restoreCredential = {"", "", false};

    if (state == WM_CONNECTED && WiFi.status() == WL_CONNECTED &&
        currentNetworkIndex < WIFI_MAX_CREDENTIALS && currentNetworkIndex != index) {
        restoreIndex = currentNetworkIndex;
        restoreCredential = WiFiStorageManager::loadCredential(restoreIndex);
    }

    if (!addWiFiNetwork(ssid, password, index)) {
        return false;
    }

    startCredentialApply(index, restoreIndex, restoreCredential);
    return true;
}

/**
 * Vrátit ukazatel na WebServer (pro přidání vlastních endpointů)
 * @return Ukazatel na WebServer nebo nullptr
 */
WebServer* WiFiManager::getWebServer() {
    return apModeServer;
}

// ============================================================================
// PRIVATE METHODS - Privátní metody
// ============================================================================

void WiFiManager::startConnectionSequence() {
    credentialApplyActive = false;
    credentialApplyRestoringPrevious = false;
    connectionSequenceActive = true;
    nextNetworkIndex = requestedStartIndex;
    requestedStartIndex = 0;
    WiFi.mode(WIFI_STA);

    if (!startNextConnectionCandidate()) {
        handleConnectionFailure();
    }
}

bool WiFiManager::startNextConnectionCandidate() {
    while (nextNetworkIndex < WIFI_MAX_CREDENTIALS) {
        WiFiCredential credential = WiFiStorageManager::loadCredential(nextNetworkIndex);
        uint8_t candidateIndex = nextNetworkIndex;
        nextNetworkIndex++;

        if (!credential.valid) {
            continue;
        }

        LOG_INFO(TAG, "Connecting to: " + credential.ssid);
        WiFi.disconnect(false);
        WiFi.mode(WIFI_STA);
        WiFi.begin(credential.ssid.c_str(), credential.password.c_str());

        currentNetworkIndex = candidateIndex;
        connectionAttemptStartTime = millis();
        lastConnectionAttempt = connectionAttemptStartTime;
        setState(WM_CONNECTING);
        return true;
    }

    if (nextNetworkIndex == WIFI_MAX_CREDENTIALS) {
        nextNetworkIndex++;
        if (String(WIFI_FALLBACK_SSID).length() > 0) {
            LOG_WARN(TAG, "Trying fixed fallback: " WIFI_FALLBACK_SSID);
            WiFi.disconnect(false);
            WiFi.mode(WIFI_STA);
            WiFi.begin(WIFI_FALLBACK_SSID, WIFI_FALLBACK_PASSWORD);

            currentNetworkIndex = WIFI_MAX_CREDENTIALS;
            connectionAttemptStartTime = millis();
            lastConnectionAttempt = connectionAttemptStartTime;
            setState(WM_CONNECTING);
            return true;
        }
    }

    connectionSequenceActive = false;
    return false;
}

void WiFiManager::handleConnectionProgress() {
    if (WiFi.status() == WL_CONNECTED) {
        connectionSequenceActive = false;
        reconnectFailureCount = 0;
        connectedOnce = true;
        printNetworkInfo();
        setState(WM_CONNECTED);
        return;
    }

    if (millis() - connectionAttemptStartTime <= WIFI_CONNECT_TIMEOUT) {
        return;
    }

    LOG_WARN(TAG, "Connection timeout");
    if (!startNextConnectionCandidate()) {
        handleConnectionFailure();
    }
}

void WiFiManager::startCredentialApply(uint8_t targetIndex, uint8_t restoreIndex, const WiFiCredential &restoreCredential) {
    if (state == WM_AP_MODE) {
        stopAPMode();
    }

    WiFiCredential targetCredential = WiFiStorageManager::loadCredential(targetIndex);
    credentialApplyActive = true;
    credentialApplyRestoringPrevious = false;
    credentialApplyRestoreIndex = restoreIndex;
    credentialApplyRestoreCredential = restoreCredential;
    connectionSequenceActive = false;
    reconnectFailureCount = 0;

    if (!startCredentialConnectionCandidate(targetIndex, targetCredential)) {
        enterCredentialApplyAPMode("Saved WiFi credential is not available");
    }
}

bool WiFiManager::startCredentialConnectionCandidate(uint8_t index, const WiFiCredential &credential) {
    if (index >= WIFI_MAX_CREDENTIALS || !credential.valid) {
        return false;
    }

    activeConnectionSSID = credential.ssid;
    WiFi.disconnect(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(credential.ssid.c_str(), credential.password.c_str());

    currentNetworkIndex = index;
    connectionAttemptStartTime = millis();
    lastConnectionAttempt = connectionAttemptStartTime;
    setState(WM_CONNECTING);
    return true;
}

void WiFiManager::handleCredentialApplyProgress() {
    if (WiFi.status() == WL_CONNECTED) {
        String label = credentialApplyRestoringPrevious ?
            "Reconnected to previous WiFi" : "Connected to saved WiFi";

        credentialApplyActive = false;
        credentialApplyRestoringPrevious = false;
        connectionSequenceActive = false;
        connectedOnce = true;
        reconnectFailureCount = 0;
        printNetworkInfo();
        setState(WM_CONNECTED);

        sendBTStatus(label + " [" + String(currentNetworkIndex) + "]: " + String(WiFi.SSID()));
        sendBTStatus("IP: " + WiFi.localIP().toString() + " | RSSI: " + String(WiFi.RSSI()) + " dBm");
        return;
    }

    if (millis() - connectionAttemptStartTime <= WIFI_CONNECT_TIMEOUT) {
        return;
    }

    sendBTStatus("Connection timeout (" + String(WIFI_CONNECT_TIMEOUT / 1000) + "s): [" +
                 String(currentNetworkIndex) + "] " + activeConnectionSSID);
    LOG_WARN(TAG, "Connection timeout for: " + activeConnectionSSID);

    if (!credentialApplyRestoringPrevious && credentialApplyRestoreCredential.valid) {
        credentialApplyRestoringPrevious = true;
        sendBTStatus("Reconnecting to previous WiFi [" + String(credentialApplyRestoreIndex) + "]: " +
                     credentialApplyRestoreCredential.ssid);

        if (!startCredentialConnectionCandidate(credentialApplyRestoreIndex, credentialApplyRestoreCredential)) {
            enterCredentialApplyAPMode("Previous WiFi credential is not available");
        }
        return;
    }

    enterCredentialApplyAPMode(credentialApplyRestoringPrevious ?
        "Previous WiFi reconnect failed" :
        "Saved WiFi failed and no previous different index is available");
}

void WiFiManager::enterCredentialApplyAPMode(const String &reason) {
    credentialApplyActive = false;
    credentialApplyRestoringPrevious = false;
    connectionSequenceActive = false;

    sendBTStatus(reason);
    sendBTStatus("AP mode starting for WiFi setup");
    WiFi.disconnect(false);
    startAPMode();

    if (state == WM_AP_MODE) {
        String apSSID = String(WIFI_AP_SSID_PREFIX) + String((uint32_t)ESP.getEfuseMac(), HEX);
        sendBTStatus("AP mode: " + apSSID + " | IP: " + WiFi.softAPIP().toString() +
                     " | Port: " + String(WEB_SERVER_PORT));
    } else {
        sendBTStatus("AP mode failed to start");
    }
}

void WiFiManager::sendBTStatus(const String &message) {
    BTCommandHandler::sendResponse(message);
}

void WiFiManager::handleConnectionFailure() {
    connectionSequenceActive = false;

    if (!connectedOnce) {
        LOG_WARN(TAG, "Initial connection failed, entering AP setup mode");
        startAPMode();
        return;
    }

    LOG_WARN(TAG, "Reconnect sequence failed");
    scheduleReconnect();
}

void WiFiManager::scheduleReconnect() {
    unsigned long delayMs = getReconnectDelayMs();
    nextReconnectAttemptTime = millis() + delayMs;
    reconnectFailureCount++;
    setState(WM_RETRY_WAIT);
    LOG_INFO(TAG, "Next WiFi reconnect in " + String(delayMs / 1000) + " seconds");
}

unsigned long WiFiManager::getReconnectDelayMs() {
    unsigned long delayMs = WIFI_RECONNECT_BASE_DELAY_MS;

    for (uint8_t i = 0; i < reconnectFailureCount && delayMs < WIFI_RECONNECT_MAX_DELAY_MS; i++) {
        delayMs *= 2;
        if (delayMs > WIFI_RECONNECT_MAX_DELAY_MS) {
            delayMs = WIFI_RECONNECT_MAX_DELAY_MS;
        }
    }

    return delayMs;
}

bool WiFiManager::tryConnectBlocking(const String &ssid, const String &password, uint8_t networkIndex) {
    LOG_INFO(TAG, "Attempting to connect to: " + ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > WIFI_CONNECT_TIMEOUT) {
            LOG_WARN(TAG, "Connection timeout for: " + ssid);
            break;
        }
        delay(100);
        Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    Serial.println();
    currentNetworkIndex = networkIndex;
    connectedOnce = true;
    reconnectFailureCount = 0;
    printNetworkInfo();
    setState(WM_CONNECTED);
    return true;
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

/**
 * Vytisknout detaily připojení
 * Volá se po úspěšném připojení
 */
void WiFiManager::printNetworkInfo() {
    Serial.println("\n=== WiFi Connected ===");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println("=====================\n");
}

// ============================================================================
// AP MODE WEB HANDLERS - Obslužné funkce webového serveru
// ============================================================================

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
