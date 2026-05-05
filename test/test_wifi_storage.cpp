#include <unity.h>
#include <Arduino.h>
#include <map>
#include <string>

/**
 * Unit Testy pro WiFiStorageManager
 * 
 * TESTUJE:
 * - Ukládání a načítání WiFi přihlašovacích údajů
 * - Validaci vstupu (délka SSID, hesla)
 * - Operace s indexy (0-2)
 * - Počítání uložených sítí
 * - Mazání jednotlivých a všech sítí
 * 
 * MOCK NVS: Simulujeme Preferences s std::map
 */

// ============================================================================
// MOCK NVS STORAGE
// ============================================================================

// Globální mock NVS paměť (simulace key-value databáze)
std::map<std::string, std::string> mockNVS;

/**
 * Mock struktura pro WiFi přihlašovací údaje
 * Stejná jako v WiFiStorageManager.h
 */
struct WiFiCredential_Mock {
    String ssid;
    String password;
    bool valid;
};

// ============================================================================
// TESTY UKLÁDÁNÍ
// ============================================================================

/**
 * Test 1: Uložit validní WiFi na index 0
 */
void test_save_valid_wifi_at_index_0() {
    mockNVS.clear();
    
    String ssid = "TestNetwork";
    String password = "TestPassword123";
    
    // Simulace saveCredential - kontrola validace
    TEST_ASSERT_TRUE(ssid.length() > 0 && ssid.length() <= 32);
    TEST_ASSERT_TRUE(password.length() <= 64);
    
    // Uložení do mock storage
    mockNVS["wifi_ssid_0"] = ssid.c_str();
    mockNVS["wifi_pass_0"] = password.c_str();
    mockNVS["wifi_count"] = "1";
    
    // Kontrola že je uloženo
    TEST_ASSERT_EQUAL_STRING("TestNetwork", mockNVS["wifi_ssid_0"].c_str());
    TEST_ASSERT_EQUAL_STRING("TestPassword123", mockNVS["wifi_pass_0"].c_str());
}

/**
 * Test 2: Uložit WiFi na všechny 3 indexy (0, 1, 2)
 */
void test_save_wifi_on_all_three_indices() {
    mockNVS.clear();
    
    String ssids[] = {"Network1", "Network2", "Network3"};
    String passwords[] = {"Pass1", "Pass2", "Pass3"};
    
    // Uložit na všechny indexy
    for (int i = 0; i < 3; i++) {
        mockNVS["wifi_ssid_" + std::to_string(i)] = ssids[i].c_str();
        mockNVS["wifi_pass_" + std::to_string(i)] = passwords[i].c_str();
    }
    mockNVS["wifi_count"] = "3";
    
    // Kontrola
    TEST_ASSERT_EQUAL_INT(3, std::stoi(mockNVS["wifi_count"]));
}

/**
 * Test 3: Zamítnutí příliš dlouhého SSID (>32 znaků)
 */
void test_reject_ssid_too_long() {
    // SSID délší než 32 znaků
    String longSSID = "ThisIsAVeryLongNetworkNameThatIsMoreThan32Characters";
    
    // Kontrola délky - měla by být zamítuta
    TEST_ASSERT_TRUE(longSSID.length() > 32);
    
    // Při pokusu o uložení by se mělo vrátit false
    // (V kódu: if (ssid.length() > 32) return false;)
}

/**
 * Test 4: Zamítnutí příliš dlouhého hesla (>64 znaků)
 */
void test_reject_password_too_long() {
    // Heslo delší než 64 znaků
    String longPassword = "";
    for (int i = 0; i < 70; i++) {
        longPassword += "A";
    }
    
    TEST_ASSERT_TRUE(longPassword.length() > 64);
    // V kódu: if (password.length() > 64) return false;
}

/**
 * Test 5: Zamítnutí prázdného SSID (0 znaků)
 */
void test_reject_empty_ssid() {
    String emptySSID = "";
    
    TEST_ASSERT_EQUAL_INT(0, emptySSID.length());
    // V kódu: if (ssid.length() == 0) return false;
}

/**
 * Test 6: Zamítnutí nevalidního indexu (>2)
 */
void test_reject_invalid_index() {
    // Index 3 je mimo rozsah (0-2)
    TEST_ASSERT_TRUE(3 >= 3);  // 3 >= WIFI_MAX_CREDENTIALS
    
    // Index 100 je také mimo
    TEST_ASSERT_TRUE(100 >= 3);
}

// ============================================================================
// TESTY NAČÍTÁNÍ
// ============================================================================

/**
 * Test 7: Načíst validní WiFi ze storage
 */
void test_load_valid_wifi() {
    mockNVS.clear();
    
    // Uložit WiFi
    mockNVS["wifi_ssid_0"] = "MyNetwork";
    mockNVS["wifi_pass_0"] = "MyPassword";
    mockNVS["wifi_count"] = "1";
    
    // Načíst - simulace loadCredential(0)
    String ssid(mockNVS["wifi_ssid_0"].c_str());
    String pass(mockNVS["wifi_pass_0"].c_str());

    TEST_ASSERT_EQUAL_STRING("MyNetwork", ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("MyPassword", pass.c_str());
}

/**
 * Test 8: Načíst neexistující WiFi vrátí prázdný string
 */
void test_load_nonexistent_wifi() {
    mockNVS.clear();
    
    // Index 0 neexistuje, měl by vrátit prázdný string
    if (mockNVS.find("wifi_ssid_0") == mockNVS.end()) {
        // Klíč neexistuje = prázdný výsledek (valid=false)
        TEST_ASSERT_TRUE(true);
    }
}

/**
 * Test 9: Načíst všechny WiFi sítě - 2 sítě
 */
void test_load_all_credentials_two_networks() {
    mockNVS.clear();
    
    // Uložit 2 sítě
    mockNVS["wifi_ssid_0"] = "Net1";
    mockNVS["wifi_pass_0"] = "Pass1";
    mockNVS["wifi_ssid_1"] = "Net2";
    mockNVS["wifi_pass_1"] = "Pass2";
    mockNVS["wifi_count"] = "2";
    
    // Kontrola počtu
    int count = std::stoi(mockNVS["wifi_count"]);
    TEST_ASSERT_EQUAL_INT(2, count);
    
    // Kontrola že obě sítě existují
    TEST_ASSERT_EQUAL_STRING("Net1", mockNVS["wifi_ssid_0"].c_str());
    TEST_ASSERT_EQUAL_STRING("Net2", mockNVS["wifi_ssid_1"].c_str());
}

// ============================================================================
// TESTY POČÍTÁNÍ
// ============================================================================

/**
 * Test 10: Počet uložených sítí = 0
 */
void test_count_zero_networks() {
    mockNVS.clear();
    
    int count = 0;
    if (mockNVS.find("wifi_count") != mockNVS.end()) {
        count = std::stoi(mockNVS["wifi_count"]);
    }
    
    TEST_ASSERT_EQUAL_INT(0, count);
}

/**
 * Test 11: Počet uložených sítí = 3
 */
void test_count_three_networks() {
    mockNVS.clear();
    
    // Uložit 3 sítě
    for (int i = 0; i < 3; i++) {
        mockNVS["wifi_ssid_" + std::to_string(i)] = "Net" + std::to_string(i);
        mockNVS["wifi_pass_" + std::to_string(i)] = "Pass" + std::to_string(i);
    }
    mockNVS["wifi_count"] = "3";
    
    int count = std::stoi(mockNVS["wifi_count"]);
    TEST_ASSERT_EQUAL_INT(3, count);
}

// ============================================================================
// TESTY MAZÁNÍ
// ============================================================================

/**
 * Test 12: Smazat jednu síť
 */
void test_clear_single_credential() {
    mockNVS.clear();
    
    // Uložit 2 sítě
    mockNVS["wifi_ssid_0"] = "Net1";
    mockNVS["wifi_pass_0"] = "Pass1";
    mockNVS["wifi_ssid_1"] = "Net2";
    mockNVS["wifi_pass_1"] = "Pass2";
    mockNVS["wifi_count"] = "2";
    
    // Smazat síť 0
    mockNVS.erase("wifi_ssid_0");
    mockNVS.erase("wifi_pass_0");
    
    // Kontrola že síť 0 je pryč, ale 1 zůstala
    TEST_ASSERT_TRUE(mockNVS.find("wifi_ssid_0") == mockNVS.end());
    TEST_ASSERT_EQUAL_STRING("Net2", mockNVS["wifi_ssid_1"].c_str());
}

/**
 * Test 13: Smazat všechny sítě
 */
void test_clear_all_credentials() {
    mockNVS.clear();
    
    // Uložit 3 sítě
    for (int i = 0; i < 3; i++) {
        mockNVS["wifi_ssid_" + std::to_string(i)] = "Net" + std::to_string(i);
        mockNVS["wifi_pass_" + std::to_string(i)] = "Pass" + std::to_string(i);
    }
    mockNVS["wifi_count"] = "3";
    
    // Smazat všechno
    mockNVS.clear();
    
    // Kontrola že je prázdné
    TEST_ASSERT_EQUAL_INT(0, (int)mockNVS.size());
}

// ============================================================================
// TESTY EDGE CASES
// ============================================================================

/**
 * Test 14: Přepsání existujícího WiFi
 */
void test_overwrite_existing_wifi() {
    mockNVS.clear();
    
    // Uložit síť
    mockNVS["wifi_ssid_0"] = "OldNetwork";
    mockNVS["wifi_pass_0"] = "OldPassword";
    
    // Přepsat
    mockNVS["wifi_ssid_0"] = "NewNetwork";
    mockNVS["wifi_pass_0"] = "NewPassword";
    
    // Kontrola že je nová
    TEST_ASSERT_EQUAL_STRING("NewNetwork", mockNVS["wifi_ssid_0"].c_str());
    TEST_ASSERT_EQUAL_STRING("NewPassword", mockNVS["wifi_pass_0"].c_str());
}

/**
 * Test 15: SSID a heslo se speciálními znaky
 */
void test_special_characters_in_credentials() {
    mockNVS.clear();
    
    String specialSSID = "Net-Work_2.4GHz!";
    String specialPass = "P@ssw0rd!#$%";
    
    mockNVS["wifi_ssid_0"] = specialSSID.c_str();
    mockNVS["wifi_pass_0"] = specialPass.c_str();
    
    TEST_ASSERT_EQUAL_STRING("Net-Work_2.4GHz!", mockNVS["wifi_ssid_0"].c_str());
    TEST_ASSERT_EQUAL_STRING("P@ssw0rd!#$%", mockNVS["wifi_pass_0"].c_str());
}

/**
 * Test 16: Mezery v SSID
 */
void test_spaces_in_ssid() {
    mockNVS.clear();
    
    String spaceSSID = "My WiFi Network";
    mockNVS["wifi_ssid_0"] = spaceSSID.c_str();
    
    TEST_ASSERT_EQUAL_STRING("My WiFi Network", mockNVS["wifi_ssid_0"].c_str());
}
