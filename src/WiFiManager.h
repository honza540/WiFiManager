#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "Logger.h"
#include "WiFiStorageManager.h"

/**
 * WiFi Manager v1.0 - Reusable WiFi Management Module
 * 
 * Hlavní řídící modul pro WiFi na ESP32
 * 
 * Funkcionalita:
 * - Ukládání až 3 WiFi sítí do paměti
 * - Auto-connect k uloženým sítím v pořadí podle priority
 * - Fallback do AP módu (přístupový bod) na konfiguraci
 * - Web server pro nastavení WiFi přes prohlížeč
 * - Monitoring stavu připojení
 * - Skenování dostupných sítí
 * 
 * Postup při startu:
 * 1. Pokusit se připojit k uloženým sítím (podle priority)
 * 2. Pokud selhá, spustit AP mód
 * 3. V AP módu čekat na konfiguraci přes web
 * 4. Po konfiguraci restartovat a zkusit znovu
 * 
 * Příklad:
 *   WiFiManager::begin();     // Inicializace v setup()
 *   WiFiManager::update();    // Zavolat v každé loop()
 */

// Stavy WiFi manageru
enum WiFiManagerState {
    WM_INIT,           // Inicializace
    WM_CONNECTING,     // Pokus o připojení k síti
    WM_CONNECTED,      // Úspěšně připojeno
    WM_AP_MODE,        // Režim přístupového bodu (konfigurační web)
    WM_DISCONNECTED    // Odpojeno - žádné připojení
};

class WiFiManager {
public:
    /**
     * Inicializace WiFi Manageru
     * 
     * Postup:
     * 1. Inicializuje NVS paměť (WiFiStorageManager)
     * 2. Načte uložené WiFi přihlašovací údaje
     * 3. Pokusí se připojit k prvnímu dostupnému síti
     * 4. Pokud selhá, spustí AP mód s webovým konfigurátorem
     * 
     * MUSÍ se zavolat v setup()
     */
    static void begin();

    /**
     * Hlavní update loop
     * 
     * MUSÍ se zavolat v každé loop() iteraci
     * 
     * Zajišťuje:
     * - Obsluhu webového serveru v AP módu
     * - Timeout pro AP mód (max 300 sekund)
     * - Monitoring stavu připojení
     */
    static void update();

    /**
     * Pokus o připojení k uloženým WiFi sítím
     * Zkusí je v pořadí: index 0 -> 1 -> 2
     * Timeout per síť: WIFI_CONNECT_TIMEOUT ms
     * 
     * @return true = připojeno, false = selhalo
     */
    static bool connectToStoredNetwork();

    /**
     * Spuštění AP módu (přístupový bod)
     * 
     * Vytvoří WiFi síť s názvem "PoolFilter-XXXXXX" (XXXXXX = chip ID)
     * Heslo: definováno v config.h (BT_PASSWORD)
     * 
     * Web server běží na:
     * - IP: 192.168.4.1
     * - Port: WEB_SERVER_PORT (80)
     * - Úkony: Nastavení WiFi, skenování sítí, status
     */
    static void startAPMode();

    /**
     * Vypnutí AP módu
     */
    static void stopAPMode();

    /**
     * Zjistit aktuální stav WiFi Manageru
     * @return WiFiManagerState enum
     */
    static WiFiManagerState getState();

    /**
     * Zjistit aktuální stav jako text
     * @return "CONNECTED", "CONNECTING", "AP_MODE", atd.
     */
    static String getStateString();

    /**
     * Kompletní status WiFi jako string
     * @return "State: CONNECTED | SSID: MyWiFi | IP: 192.168.1.100 | RSSI: -45 dBm"
     */
    static String getStatusString();

    /**
     * Síla signálu (kvalitativně)
     * @return "Excellent", "Good", "Fair", "Weak", "Very Weak", nebo "N/A"
     */
    static String getSignalStrength();

    /**
     * Aktuální připojené WiFi SSID
     * @return Jméno sítě nebo "N/A"
     */
    static String getSSID();

    /**
     * Aktuální IP adresa
     * @return IP adresa nebo "N/A"
     */
    static String getIP();

    /**
     * Skenování dostupných WiFi sítí
     * @param async - true = asynchronní (pozadí), false = blokující (čekej na výsledek)
     */
    static void scanNetworks(bool async = false);

    /**
     * Počet nalezených WiFi sítí při posledním skenování
     * @return Počet sítí, -1 = chyba, -2 = probíhá skenování
     */
    static int getScanResultCount();

    /**
     * Informace o jedné WiFi síti ze skenování
     * @param index - Pozice v seznamu (0-n)
     * @return "SSID|RSSI|ENCRYPTED" (např. "MyWiFi|-45|Secured")
     */
    static String getScanResult(int index);

    /**
     * Přidat novou WiFi síť do paměti
     * @param ssid - Jméno sítě
     * @param password - Heslo
     * @param index - Pozice (0-2)
     * @return true = uloženo, false = chyba
     */
    static bool addWiFiNetwork(const String &ssid, const String &password, uint8_t index);

    /**
     * Vrátit ukazatel na WebServer objekt
     * Používáno pro přidání vlastních endpointů
     * @return Ukazatel na WebServer nebo nullptr
     */
    static WebServer* getWebServer();

private:
    // ========== STATICKÉ PROMĚNNÉ ==========
    
    // Aktuální stav WiFi manageru
    static WiFiManagerState state;
    
    // WebServer pro AP mód konfiguraci
    static WebServer* apModeServer;
    
    // Čas kdy se spustil AP mód (pro timeout)
    static unsigned long apModeStartTime;
    
    // Čas poslední zmáry připojení
    static unsigned long lastConnectionAttempt;
    
    // Index aktuálně připojené WiFi sítě
    // 0-2 = NVS, WIFI_MAX_CREDENTIALS = fixed fallback.
    static uint8_t currentNetworkIndex;
    
    // Příznak že probíhá WiFi skenování
    static bool scanInProgress;
    
    // Počet sítí nalezených při posledním skenování
    static int scanResultCount;
    
    // Tag pro logování
    static const char* TAG;

    // ========== PRIVÁTNÍ METODY ==========
    
    // Obsluha AP módu - zpracování webových requestů
    static void handleAPModeWeb();
    
    // GET /
    static void handleAPModeRoot();
    
    // GET /config
    static void handleAPModeConfig();
    
    // GET /status
    static void handleAPModeStatus();
    
    // GET /scan
    static void handleAPModeScan();
    
    // POST /save
    static void handleAPModeSave();
    
    // Změní stav na nový a zaloguje ho
    static void setState(WiFiManagerState newState);

    // Vytiskne detaily připojení (SSID, IP, Gateway, atd.)
    static void printNetworkInfo();
};

#endif // WIFI_MANAGER_H
