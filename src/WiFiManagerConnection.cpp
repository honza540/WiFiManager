#include "WiFiManager.h"
#include "BTCommandHandler.h"

// ============================================================================
// CONNECTION STATE MACHINE
// ============================================================================

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
