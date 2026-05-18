#include "WiFiManager.h"

// ============================================================================
// WIFI SCANNING
// ============================================================================

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
