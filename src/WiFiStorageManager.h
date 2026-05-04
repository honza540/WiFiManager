#ifndef WIFI_STORAGE_MANAGER_H
#define WIFI_STORAGE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "Logger.h"

/**
 * WiFi Storage Manager
 * 
 * Spravuje ukládání a načítání WiFi přihlašovacích údajů (SSID + heslo)
 * do netrvalé paměti ESP32 (NVS = Non-Volatile Storage)
 * 
 * NVS paměť:
 * - Funguje jako key-value databáze
 * - Zůstává uchována i po restartování desky
 * - Lze uložit až 3 WiFi sítě (index 0-2)
 * - Obsah se neztrácí ani po vypnutí desky
 * 
 * Příklad:
 *   WiFiStorageManager::begin();
 *   WiFiStorageManager::saveCredential(0, "MyWiFi", "MyPassword123");
 *   WiFiCredential cred = WiFiStorageManager::loadCredential(0);
 */

// Struktura pro jednu WiFi síť
struct WiFiCredential {
    String ssid;        // Jméno sítě (do 32 znaků)
    String password;    // Heslo (do 64 znaků)
    bool valid;         // Příznak zda jsou data v pořádku
};

class WiFiStorageManager {
public:
    /**
     * Inicializace NVS paměti
     * MUSÍ se zavolat jednou v setup() před ostatními operacemi
     */
    static void begin();

    /**
     * Uložit WiFi přihlašovací údaje do paměti
     * @param index - Pozice (0-2, max 3 sítě)
     * @param ssid - Jméno WiFi (max 32 znaků)
     * @param password - Heslo WiFi (max 64 znaků)
     * @return true = úspěšně uloženo, false = chyba (špatný index, přílišné dlouhé)
     */
    static bool saveCredential(uint8_t index, const String &ssid, const String &password);

    /**
     * Načíst jednu WiFi síť z paměti
     * @param index - Pozice (0-2)
     * @return Struktura WiFiCredential s údaji (valid = false pokud nic nemáme)
     */
    static WiFiCredential loadCredential(uint8_t index);

    /**
     * Načíst všechny uložené WiFi sítě najednou
     * @param count - Reference na proměnnou, která bude obsahovat počet sítí
     * @return Pole WiFiCredential struktur (musíš smazat: delete[] credentials)
     *         nebo nullptr pokud nic nemáme
     */
    static WiFiCredential* loadAllCredentials(uint8_t &count);

    /**
     * Zjistit kolik WiFi sítí máme v paměti
     * @return Počet uložených sítí (0-3)
     */
    static uint8_t getCredentialCount();

    /**
     * Smazat všechny uložené WiFi přihlašovací údaje
     * Používáno pro reset na tovární nastavení
     */
    static void clearAll();

    /**
     * Smazat jednu WiFi síť ze paměti
     * @param index - Pozice (0-2)
     */
    static void clearCredential(uint8_t index);

    /**
     * Vypsat všechny uložené WiFi sítě na Serial (pro debug)
     */
    static void printAllCredentials();

private:
    // NVS objekt (key-value paměť)
    static Preferences nvs;
    
    // Tag pro logování
    static const char* TAG;
};

#endif // WIFI_STORAGE_MANAGER_H
