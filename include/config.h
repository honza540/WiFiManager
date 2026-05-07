/*
 * WiFiManager Library Configuration
 * 
 * Copy this file to your project's include/ folder and customize as needed.
 * This template shows the required configuration constants.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "version.h"

// ============================================================================
// VERSION CONSTANTS
// ============================================================================

#define FIRMWARE_VERSION "1.0.0"
#define WIFIMANAGER_VERSION WIFIMANAGER_VERSION_STRING
#define WEB_SERVER_PORT 80

// ============================================================================
// WIFI MANAGER CONFIGURATION
// ============================================================================

// Maximální počet WiFi sítí, které lze uložit v NVS (0-2 = 3 sítě max)
#define WIFI_MAX_CREDENTIALS 3

// Pevné fallback WiFi údaje. Neukladají se do NVS, proto nejdou měnit přes BT.
#define WIFI_FALLBACK_SSID "BeSmarter"
#define WIFI_FALLBACK_PASSWORD "123456789"

// Timeout pro připojování k jedné WiFi síti (ms)
// Po uplynutí se zkusí další síť ze seznamu
#define WIFI_CONNECT_TIMEOUT 15000

// Timeout pro skenování dostupných WiFi sítí (ms)
#define WIFI_SCAN_TIMEOUT 10000

// Timeout pro AP mode (sekundy) - jak dlouho čekat na konfiguraci přes web
// Pokud v tomto čase neobjeví konfigurace, deska se restartuje
#define AP_MODE_TIMEOUT 300

// ============================================================================
// BLUETOOTH CONFIGURATION
// ============================================================================

// Jméno Bluetooth zařízení (viditelné při párování)
#define BT_DEVICE_NAME "PoolFilter"

// Heslo pro připojení přes BT (PIN kód)
#define BT_PASSWORD "37"

// ============================================================================
// LOGGER CONFIGURATION
// ============================================================================

// Úrovně logování - jaké zprávy se mají vypsat
// DEBUG (4): detailní informace pro vývojáře
// INFO (3): důležité informace
// WARN (2): varování
// ERROR (1): chyby
// NONE (0): nic se nevypisuje
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_NONE 0

// Aktuální nastavená úroveň logování
// Zprávy s nižší prioritou se nebudou vypisovat
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

// Povolit výstup logů na seriovou linku (Serial console)
#define LOG_TO_SERIAL 1

// Povolit výstup logů na Bluetooth
#define LOG_TO_BT 1

// Velikost bufferu pro log zprávy (znaky)
#define LOG_BUFFER_SIZE 256

// ============================================================================
// STORAGE (Preferences/NVS)
// ============================================================================

// Namespace pro uložení WiFi údajů v NVS paměti
// NVS = Non-Volatile Storage (flash paměť s klíč-hodnotou)
#define NVRAM_NAMESPACE "poolfilter"

#endif // CONFIG_H
