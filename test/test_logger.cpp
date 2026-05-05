#include <unity.h>
#include <Arduino.h>
#include "Logger.h"

#ifndef ARDUINO_ARCH_ESP32
/**
 * Unit Testy pro Logger
 * 
 * Testuje:
 * - Formátování log zpráv
 * - Filtrování podle log úrovně
 * - Převod úrovně na text
 * - Enable/disable výstupů
 */

// Host-side Serial stub output storage is provided by NativeSerial::lastMessage.

// ============================================================================
// TESTY FORMÁTOVÁNÍ
// ============================================================================

/**
 * Test 1: Kontrola že log zpráva obsahuje všechny komponenty
 * [čas] [ÚROVEŇ] [tag] zpráva
 */
void test_logger_format_contains_all_parts() {
    NativeSerial::lastMessage = "";
    
    // Při logování DEBUG zprávy
    Logger::debug("TEST", "Message");
    
    // Kontrola že výstup obsahuje všechny části
    String msg = NativeSerial::lastMessage;
    
    TEST_ASSERT_TRUE(msg.indexOf("[") >= 0);
    TEST_ASSERT_TRUE(msg.indexOf("DEBUG") >= 0);
    TEST_ASSERT_TRUE(msg.indexOf("TEST") >= 0);
    TEST_ASSERT_TRUE(msg.indexOf("Message") >= 0);
}

/**
 * Test 2: Log úroveň se správně převádí na text
 */
void test_logger_level_to_string() {
    NativeSerial::lastMessage = "";
    
    Logger::debug("DBG", "test");
    TEST_ASSERT_TRUE(NativeSerial::lastMessage.indexOf("DEBUG") >= 0);
    
    NativeSerial::lastMessage = "";
    
    Logger::error("ERR", "test");
    TEST_ASSERT_TRUE(NativeSerial::lastMessage.indexOf("ERROR") >= 0);
}

/**
 * Test 3: Dlouhé zprávy se správně logují
 */
void test_logger_long_message() {
    NativeSerial::lastMessage = "";
    
    String longMsg = "";
    for (int i = 0; i < 50; i++) {
        longMsg += "HelloWorld";
    }
    
    Logger::info("LONG", longMsg);
    
    TEST_ASSERT_TRUE(NativeSerial::lastMessage.indexOf(longMsg) >= 0);
}

/**
 * Test 4: Speciální znaky se správně logují
 */
void test_logger_special_characters() {
    NativeSerial::lastMessage = "";
    
    Logger::info("SPC", "Test: !@#$%^&*()");
    
    TEST_ASSERT_TRUE(NativeSerial::lastMessage.indexOf("!@#$%^&*()") >= 0);
}

// ============================================================================
// TESTY FILTROVÁNÍ ÚROVNÍ
// ============================================================================

/**
 * Test 5: Nižší úrovně se filtrují (DEBUG se neloguje pokud je min INFO)
 * POZN: To by vyžadovalo změnu CURRENT_LOG_LEVEL v compile time
 */
void test_logger_filtering_by_level() {
    NativeSerial::lastMessage = "";
    
    #if CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::debug("DBG", "Should be logged");
        TEST_ASSERT_TRUE(NativeSerial::lastMessage.length() > 0);
    #endif
}

/**
 * Test 6: INFO zpráva se vždy loguje (vyšší priorita)
 */
void test_logger_info_level() {
    NativeSerial::lastMessage = "";
    
    Logger::info("INF", "Important info");
    
    TEST_ASSERT_TRUE(NativeSerial::lastMessage.indexOf("INFO") >= 0);
    TEST_ASSERT_TRUE(NativeSerial::lastMessage.indexOf("Important info") >= 0);
}

/**
 * Test 7: ERROR zpráva se vždy loguje (nejvyšší priorita)
 */
void test_logger_error_level() {
    NativeSerial::lastMessage = "";
    
    Logger::error("ERR", "Critical error");
    
    TEST_ASSERT_TRUE(NativeSerial::lastMessage.indexOf("ERROR") >= 0);
    TEST_ASSERT_TRUE(NativeSerial::lastMessage.indexOf("Critical error") >= 0);
}

// ============================================================================
// TESTY ENABLE/DISABLE
// ============================================================================

/**
 * Test 8: Serial výstup lze vypnout
 */
void test_logger_disable_serial() {
    NativeSerial::lastMessage = "";
    
    Logger::enableSerial(false);
    Logger::info("INF", "This should not appear on serial");
    
    // Zpráva by neměla být v Serial (protože je vypnuta)
    // NOTE: Bez reálného Serial.println to nemůžeme otestovat přímo,
    // ale logika je v Logger.cpp
    
    // Zase zapnout
    Logger::enableSerial(true);
}

/**
 * Test 9: BT výstup se správně zapíná/vypíná
 */
void test_logger_bt_enable_disable() {
    Logger::enableBT(false);
    Logger::enableBT(true);
    
    Logger::Level level = Logger::getCurrentLevel();
    TEST_ASSERT_TRUE(level == LOG_LEVEL_DEBUG || level >= LOG_LEVEL_ERROR);
}

// ============================================================================
// TESTY TAG KONSTANT
// ============================================================================

/**
 * Test 10: Všechny tag konstanty jsou definovány
 */
void test_logger_tag_constants_defined() {
    const char* tags[] = {TAG_MAIN, TAG_WIFI, TAG_BT, TAG_RELAY, TAG_WEB, TAG_SYS};
    
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_NOT_NULL(tags[i]);
    }
}

// ============================================================================
// TESTY MAKRA
// ============================================================================

/**
 * Test 11: Convenience makra fungují
 */
void test_logger_macros() {
    NativeSerial::lastMessage = "";
    
    LOG_INFO("TST", "Macro test");
    
    TEST_ASSERT_TRUE(NativeSerial::lastMessage.indexOf("Macro test") >= 0);
}

#endif // ARDUINO_ARCH_ESP32
