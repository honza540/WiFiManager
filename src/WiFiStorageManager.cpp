#include "WiFiStorageManager.h"

// ============================================================================
// STATIC MEMBER INITIALIZATION - Inicializace statických proměnných
// ============================================================================

// NVS paměť pro ukládání dat
Preferences WiFiStorageManager::nvs;

// Tag pro logování (4 znaky max)
const char* WiFiStorageManager::TAG = "WiFiStorage";

// ============================================================================
// PUBLIC METHODS - Veřejné metody
// ============================================================================

/**
 * Inicializace NVS paměti
 * Musí se zavolat v setup() JEŠTĚ PŘED všemi ostatními operacemi
 * 
 * @note Vytváří namespace "poolfilter" pro oddělení dat od ostatních aplikací
 */
void WiFiStorageManager::begin() {
    // Otevření NVS paměti v read-write módu
    // NVRAM_NAMESPACE je definován v config.h ("poolfilter")
    // Parametr false znamená read-write (true by znamenal read-only)
    if (!nvs.begin(NVRAM_NAMESPACE, false)) {
        LOG_ERROR(TAG, "Failed to initialize NVS");
        return;
    }
    LOG_INFO(TAG, "Storage initialized");
}

/**
 * Uložit WiFi přihlašovací údaje
 * 
 * Postup:
 * 1. Kontrola indexu (0-2)
 * 2. Kontrola délky SSID (1-32 znaků)
 * 3. Kontrola délky hesla (max 64 znaků)
 * 4. Uložení do NVS pod klíči "wifi_ssid_0" a "wifi_pass_0" atd.
 * 5. Aktualizace počtu uložených sítí
 * 
 * @param index - Pozice (0, 1, nebo 2)
 * @param ssid - Jméno WiFi (musí být 1-32 znaků)
 * @param password - Heslo (max 64 znaků)
 * @return true = úspěšně, false = chyba
 */
bool WiFiStorageManager::saveCredential(uint8_t index, const String &ssid, const String &password) {
    // Kontrola: Je index validní? (0-2)
    if (index >= WIFI_MAX_CREDENTIALS) {
        LOG_ERROR(TAG, "Invalid credential index: " + String(index));
        return false;
    }

    // Kontrola: Je SSID v pořádku?
    // Musí mít aspoň 1 znak a max 32 (WiFi standard)
    if (ssid.length() == 0 || ssid.length() > 32) {
        LOG_ERROR(TAG, "Invalid SSID length");
        return false;
    }

    // Kontrola: Je heslo v pořádku?
    // Max 64 znaků (WiFi standard)
    if (password.length() > 64) {
        LOG_ERROR(TAG, "Invalid password length");
        return false;
    }

    try {
        // Vytvoření klíčů pro ukládání
        // Příklad: "wifi_ssid_0", "wifi_pass_0"
        String ssidKey = "wifi_ssid_" + String(index);
        String passKey = "wifi_pass_" + String(index);

        // Uložení SSID a hesla do NVS
        nvs.putString(ssidKey.c_str(), ssid);
        nvs.putString(passKey.c_str(), password);

        // Aktualizace počtu uložených sítí
        // Pokud jsme právě uložili na vyšší index, zvýšíme počet
        uint8_t count = getCredentialCount();
        if (index >= count) {
            nvs.putUChar("wifi_count", index + 1);
        }

        LOG_INFO(TAG, "Credential " + String(index) + " saved: SSID=" + ssid);
        return true;
    } catch (...) {
        // Chyba - pravděpodobně NVS je plná
        LOG_ERROR(TAG, "Exception saving credential");
        return false;
    }
}

/**
 * Načíst jednu WiFi síť z paměti podle indexu
 * 
 * @param index - Pozice (0-2)
 * @return WiFiCredential struktura
 *         - Pokud existuje: ssid, password a valid=true
 *         - Pokud neexistuje: prázdný string a valid=false
 */
WiFiCredential WiFiStorageManager::loadCredential(uint8_t index) {
    // Vytvoření prázdné struktury (default odpověď - nic na indexu)
    WiFiCredential cred = {"", "", false};

    // Kontrola: Je index validní?
    if (index >= WIFI_MAX_CREDENTIALS) {
        LOG_WARN(TAG, "Invalid credential index: " + String(index));
        return cred;
    }

    // Vytvoření klíčů pro čtení
    String ssidKey = "wifi_ssid_" + String(index);
    String passKey = "wifi_pass_" + String(index);

    // Načtení SSID a hesla z NVS
    // getString("key", "defaultValue") vrací defaultValue pokud klíč neexistuje
    String ssid = nvs.getString(ssidKey.c_str(), "");
    String pass = nvs.getString(passKey.c_str(), "");

    // Pokud jsme něco načetli, označit jako validní
    if (ssid.length() > 0) {
        cred.ssid = ssid;
        cred.password = pass;
        cred.valid = true;
        LOG_DEBUG(TAG, "Loaded credential " + String(index) + ": " + ssid);
    }

    return cred;
}

/**
 * Načíst všechny uložené WiFi sítě najednou
 * 
 * @param count - Reference na proměnnou, která bude obsahovat počet sítí
 * @return Dynamické pole WiFiCredential
 *         MUSÍŠ smazat: delete[] credentials;
 *         nebo nullptr pokud nic nemáme
 * 
 * Příklad:
 *   uint8_t count;
 *   WiFiCredential* creds = WiFiStorageManager::loadAllCredentials(count);
 *   for (uint8_t i = 0; i < count; i++) {
 *       Serial.println(creds[i].ssid);
 *   }
 *   delete[] creds;  // DŮLEŽITÉ!
 */
WiFiCredential* WiFiStorageManager::loadAllCredentials(uint8_t &count) {
    // Zjistit kolik sítí máme
    count = getCredentialCount();
    
    // Pokud nemáme nic, vrátit nullptr
    if (count == 0) {
        LOG_INFO(TAG, "No stored credentials");
        return nullptr;
    }

    // Alokace paměti pro všechny sítě
    // Příklad: count=2 -> pole pro 2 WiFiCredential struktury
    WiFiCredential* credentials = new WiFiCredential[count];

    // Načtení jednotlivých sítí
    for (uint8_t i = 0; i < count; i++) {
        credentials[i] = loadCredential(i);
    }

    LOG_INFO(TAG, "Loaded " + String(count) + " credentials");
    return credentials;
}

/**
 * Zjistit kolik WiFi sítí máme uloženo
 * 
 * @return Počet (0-3)
 */
uint8_t WiFiStorageManager::getCredentialCount() {
    // Čtení z NVS klíče "wifi_count"
    // Pokud klíč neexistuje, vrátit default 0
    return nvs.getUChar("wifi_count", 0);
}

/**
 * Smazat VŠECHNY uložené WiFi sítě
 * Vrátí desku na tovární nastavení (bez WiFi)
 * 
 * @warning Zavolá se to automaticky když se nepodaří připojit k žádné síti
 */
void WiFiStorageManager::clearAll() {
    nvs.clear();
    LOG_INFO(TAG, "All credentials cleared");
}

/**
 * Smazat jednu konkrétní WiFi síť
 * 
 * @param index - Pozice (0-2)
 * 
 * Příklad:
 *   WiFiStorageManager::clearCredential(1);  // Smazat druhou síť
 */
void WiFiStorageManager::clearCredential(uint8_t index) {
    // Kontrola: Je index validní?
    if (index >= WIFI_MAX_CREDENTIALS) {
        return;
    }

    // Vytvoření klíčů
    String ssidKey = "wifi_ssid_" + String(index);
    String passKey = "wifi_pass_" + String(index);

    // Smazání klíčů z NVS
    nvs.remove(ssidKey.c_str());
    nvs.remove(passKey.c_str());

    uint8_t newCount = 0;
    for (uint8_t i = 0; i < WIFI_MAX_CREDENTIALS; i++) {
        String key = "wifi_ssid_" + String(i);
        if (nvs.getString(key.c_str(), "").length() > 0) {
            newCount = i + 1;
        }
    }
    nvs.putUChar("wifi_count", newCount);

    LOG_INFO(TAG, "Credential " + String(index) + " cleared");
}

/**
 * Vypsat všechny uložené WiFi sítě
 * Používá se pro debug přes Serial/Bluetooth
 * 
 * Příklad výstupu:
 * === Stored WiFi Credentials ===
 * [0] SSID: MyWiFi
 * [1] SSID: GuestNetwork
 * ==============================
 */
void WiFiStorageManager::printAllCredentials() {
    // Zjistit počet sítí
    uint8_t count = getCredentialCount();
    LOG_INFO(TAG, "=== Stored WiFi Credentials ===");

    // Pokud nemáme nic, informovat uživatele
    if (count == 0) {
        LOG_INFO(TAG, "No credentials stored");
        return;
    }

    // Vypsat každou síť
    for (uint8_t i = 0; i < count; i++) {
        WiFiCredential cred = loadCredential(i);
        if (cred.valid) {
            LOG_INFO(TAG, "[" + String(i) + "] SSID: " + cred.ssid);
        }
    }
    LOG_INFO(TAG, "==============================");
}
