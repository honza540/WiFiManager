#include <unity.h>
#include <Arduino.h>
#include <map>

// Extern declarations for tests in other files so one Unity runner can execute all test cases.
#ifndef ARDUINO_ARCH_ESP32
extern void test_logger_format_contains_all_parts();
extern void test_logger_level_to_string();
extern void test_logger_long_message();
extern void test_logger_special_characters();
extern void test_logger_filtering_by_level();
extern void test_logger_info_level();
extern void test_logger_error_level();
extern void test_logger_disable_serial();
extern void test_logger_bt_enable_disable();
extern void test_logger_tag_constants_defined();
extern void test_logger_macros();
#endif // ARDUINO_ARCH_ESP32

extern void test_save_valid_wifi_at_index_0();
extern void test_save_wifi_on_all_three_indices();
extern void test_reject_ssid_too_long();
extern void test_reject_password_too_long();
extern void test_reject_empty_ssid();
extern void test_reject_invalid_index();
extern void test_load_valid_wifi();
extern void test_load_nonexistent_wifi();
extern void test_load_all_credentials_two_networks();
extern void test_count_zero_networks();
extern void test_count_three_networks();
extern void test_clear_single_credential();
extern void test_clear_all_credentials();
extern void test_overwrite_existing_wifi();
extern void test_special_characters_in_credentials();
extern void test_spaces_in_ssid();
extern void test_setup_page_contains_wifi_form_contract();
extern void test_saved_page_contains_confirmation_contract();
extern void test_storage_manager_save_load_roundtrip();
extern void test_storage_manager_rejects_invalid_inputs();
extern void test_storage_manager_clear_credential_recomputes_count();
extern void test_storage_manager_load_all_preserves_index_holes();
extern void test_ap_mode_registers_expected_web_routes();
extern void test_ap_root_route_returns_setup_page();
extern void test_ap_status_route_reports_state_and_credential_count();
extern void test_ap_save_route_rejects_missing_password();
extern void test_ap_save_route_stores_credentials_and_returns_saved_page();
extern void test_reconnect_starts_from_requested_stored_index();
extern void test_reconnect_clamps_invalid_requested_index_to_zero();
extern void test_reconnect_uses_fixed_fallback_when_no_credentials_exist();
extern void test_start_ap_mode_does_not_create_server_when_softap_fails();

/**
 * Unit Testy pro BTCommandHandler - parsování příkazů
 * 
 * TESTUJE:
 * - Parsování příkazů z BT vstupu
 * - Extrakci argumentů
 * - Validaci příkazů
 * - Case-insensitivity
 * - Detekci chyb
 * 
 * MOCK: Simulujeme BT příkazy bez skutečného Bluetooth
 */

// ============================================================================
// MOCK PŘÍKAZY A ARGPARSEOVÁNÍ
// ============================================================================

/**
 * Mock struktura pro parsovaný příkaz
 */
struct ParsedCommand {
    String cmd;           // Příkaz ("wifi-set", "status", atd.)
    String args;          // Argumenty za příkazem
    bool valid;           // Je příkaz validní
};

/**
 * Simulace parsování příkazu
 * Vstup: "wifi-set 0 MyNetwork MyPassword"
 * Výstup: cmd="wifi-set", args="0 MyNetwork MyPassword"
 */
ParsedCommand parseCommand(String input) {
    ParsedCommand result = {"", "", false};
    
    input.trim();  // Odstranit mezery na začátku/konci
    
    if (input.length() == 0) {
        return result;  // Prázdný vstup
    }
    
    // Najít pozici první mezery
    int spacePos = input.indexOf(' ');
    
    if (spacePos > 0) {
        // Je příkaz a argumenty
        result.cmd = input.substring(0, spacePos);
        result.args = input.substring(spacePos + 1);
    } else {
        // Jen příkaz bez argumentů
        result.cmd = input;
        result.args = "";
    }
    
    // Převést na lowercase
    result.cmd.toLowerCase();
    result.args.trim();
    result.valid = (result.cmd.length() > 0);
    
    return result;
}

/**
 * Extrakce N-tého argumentu z argumentů
 * Input: "0 MyNetwork MyPassword", argIndex=0 -> "0"
 * Input: "0 MyNetwork MyPassword", argIndex=1 -> "MyNetwork"
 */
String getArg(String args, uint8_t argIndex) {
    uint8_t currentArg = 0;
    int startPos = 0;
    
    for (int i = 0; i <= args.length(); i++) {
        // Když narazíme na mezeru nebo konec stringu
        if (i == args.length() || args[i] == ' ') {
            if (currentArg == argIndex) {
                return args.substring(startPos, i);
            }
            
            currentArg++;
            startPos = i + 1;
        }
    }
    
    return "";  // Argument nenalezen
}

// ============================================================================
// TESTY PARSOVÁNÍ ZÁKLADNÍCH PŘÍKAZŮ
// ============================================================================

/**
 * Test 1: Parsování "help"
 */
void test_parse_help_command() {
    ParsedCommand cmd = parseCommand("help");
    
    TEST_ASSERT_EQUAL_STRING("help", cmd.cmd.c_str());
    TEST_ASSERT_EQUAL_STRING("", cmd.args.c_str());
    TEST_ASSERT_TRUE(cmd.valid);
}

/**
 * Test 2: Parsování "status"
 */
void test_parse_status_command() {
    ParsedCommand cmd = parseCommand("status");
    
    TEST_ASSERT_EQUAL_STRING("status", cmd.cmd.c_str());
    TEST_ASSERT_TRUE(cmd.valid);
}

/**
 * Test 3: Parsování "reboot"
 */
void test_parse_reboot_command() {
    ParsedCommand cmd = parseCommand("reboot");
    
    TEST_ASSERT_EQUAL_STRING("reboot", cmd.cmd.c_str());
    TEST_ASSERT_TRUE(cmd.valid);
}

/**
 * Test 4: Parsování "wifi-list"
 */
void test_parse_wifi_list_command() {
    ParsedCommand cmd = parseCommand("wifi-list");
    
    TEST_ASSERT_EQUAL_STRING("wifi-list", cmd.cmd.c_str());
    TEST_ASSERT_TRUE(cmd.valid);
}

// ============================================================================
// TESTY PARSOVÁNÍ PŘÍKAZŮ S ARGUMENTY
// ============================================================================

/**
 * Test 5: Parsování "wifi-clear 0"
 */
void test_parse_wifi_clear_with_index() {
    ParsedCommand cmd = parseCommand("wifi-clear 0");
    
    TEST_ASSERT_EQUAL_STRING("wifi-clear", cmd.cmd.c_str());
    TEST_ASSERT_EQUAL_STRING("0", cmd.args.c_str());
    TEST_ASSERT_TRUE(cmd.valid);
}

/**
 * Test 6: Parsování "wifi-set" se 3 argumenty
 * "wifi-set 0 MyNetwork MyPassword"
 */
void test_parse_wifi_set_full() {
    ParsedCommand cmd = parseCommand("wifi-set 0 MyNetwork MyPassword");
    
    TEST_ASSERT_EQUAL_STRING("wifi-set", cmd.cmd.c_str());
    TEST_ASSERT_EQUAL_STRING("0 MyNetwork MyPassword", cmd.args.c_str());
    
    String arg0 = getArg(cmd.args, 0);
    String arg1 = getArg(cmd.args, 1);
    String arg2 = getArg(cmd.args, 2);
    
    TEST_ASSERT_EQUAL_STRING("0", arg0.c_str());
    TEST_ASSERT_EQUAL_STRING("MyNetwork", arg1.c_str());
    TEST_ASSERT_EQUAL_STRING("MyPassword", arg2.c_str());
}

/**
 * Test 7: Parsování "netconnect 1"
 */
void test_parse_netconnect_command() {
    ParsedCommand cmd = parseCommand("netconnect 1");
    
    TEST_ASSERT_EQUAL_STRING("netconnect", cmd.cmd.c_str());
    String index = getArg(cmd.args, 0);
    TEST_ASSERT_EQUAL_STRING("1", index.c_str());
}

// ============================================================================
// TESTY CASE INSENSITIVITY
// ============================================================================

/**
 * Test 8: Příkazy velkými písmeny se převedou na lowercase
 */
void test_command_uppercase_converted() {
    ParsedCommand cmd = parseCommand("STATUS");
    
    TEST_ASSERT_EQUAL_STRING("status", cmd.cmd.c_str());
}

/**
 * Test 9: Mixované písmeno se převede
 */
void test_command_mixed_case_converted() {
    ParsedCommand cmd = parseCommand("WiFi-SeT");
    
    TEST_ASSERT_EQUAL_STRING("wifi-set", cmd.cmd.c_str());
}

/**
 * Test 10: Help si bude bez ohledu na case
 */
void test_command_help_any_case() {
    ParsedCommand cmd1 = parseCommand("HELP");
    ParsedCommand cmd2 = parseCommand("Help");
    ParsedCommand cmd3 = parseCommand("help");
    
    TEST_ASSERT_EQUAL_STRING("help", cmd1.cmd.c_str());
    TEST_ASSERT_EQUAL_STRING("help", cmd2.cmd.c_str());
    TEST_ASSERT_EQUAL_STRING("help", cmd3.cmd.c_str());
}

// ============================================================================
// TESTY MEZERY A SPECIÁLNÍ ZNAKY
// ============================================================================

/**
 * Test 11: Extra mezery se ignorují
 */
void test_extra_spaces_trimmed() {
    ParsedCommand cmd = parseCommand("  status  ");
    
    TEST_ASSERT_EQUAL_STRING("status", cmd.cmd.c_str());
    TEST_ASSERT_EQUAL_STRING("", cmd.args.c_str());
}

void runAllTests() {
    RUN_TEST(test_parse_help_command);
    RUN_TEST(test_parse_status_command);
    RUN_TEST(test_parse_reboot_command);
    RUN_TEST(test_parse_wifi_list_command);
    RUN_TEST(test_parse_wifi_clear_with_index);
    RUN_TEST(test_parse_wifi_set_full);
    RUN_TEST(test_parse_netconnect_command);
    RUN_TEST(test_command_uppercase_converted);
    RUN_TEST(test_command_mixed_case_converted);
    RUN_TEST(test_command_help_any_case);
    RUN_TEST(test_extra_spaces_trimmed);

    // Delegate to other test groups if available
#ifndef ARDUINO_ARCH_ESP32
    RUN_TEST(test_logger_format_contains_all_parts);
    RUN_TEST(test_logger_level_to_string);
    RUN_TEST(test_logger_long_message);
    RUN_TEST(test_logger_special_characters);
    RUN_TEST(test_logger_filtering_by_level);
    RUN_TEST(test_logger_info_level);
    RUN_TEST(test_logger_error_level);
    RUN_TEST(test_logger_disable_serial);
    RUN_TEST(test_logger_bt_enable_disable);
    RUN_TEST(test_logger_tag_constants_defined);
    RUN_TEST(test_logger_macros);
#endif
    RUN_TEST(test_save_valid_wifi_at_index_0);
    RUN_TEST(test_save_wifi_on_all_three_indices);
    RUN_TEST(test_reject_ssid_too_long);
    RUN_TEST(test_reject_password_too_long);
    RUN_TEST(test_reject_empty_ssid);
    RUN_TEST(test_reject_invalid_index);
    RUN_TEST(test_load_valid_wifi);
    RUN_TEST(test_load_nonexistent_wifi);
    RUN_TEST(test_load_all_credentials_two_networks);
    RUN_TEST(test_count_zero_networks);
    RUN_TEST(test_count_three_networks);
    RUN_TEST(test_clear_single_credential);
    RUN_TEST(test_clear_all_credentials);
    RUN_TEST(test_overwrite_existing_wifi);
    RUN_TEST(test_special_characters_in_credentials);
    RUN_TEST(test_spaces_in_ssid);
    RUN_TEST(test_setup_page_contains_wifi_form_contract);
    RUN_TEST(test_saved_page_contains_confirmation_contract);
    RUN_TEST(test_storage_manager_save_load_roundtrip);
    RUN_TEST(test_storage_manager_rejects_invalid_inputs);
    RUN_TEST(test_storage_manager_clear_credential_recomputes_count);
    RUN_TEST(test_storage_manager_load_all_preserves_index_holes);
    RUN_TEST(test_ap_mode_registers_expected_web_routes);
    RUN_TEST(test_ap_root_route_returns_setup_page);
    RUN_TEST(test_ap_status_route_reports_state_and_credential_count);
    RUN_TEST(test_ap_save_route_rejects_missing_password);
    RUN_TEST(test_ap_save_route_stores_credentials_and_returns_saved_page);
    RUN_TEST(test_reconnect_starts_from_requested_stored_index);
    RUN_TEST(test_reconnect_clamps_invalid_requested_index_to_zero);
    RUN_TEST(test_reconnect_uses_fixed_fallback_when_no_credentials_exist);
    RUN_TEST(test_start_ap_mode_does_not_create_server_when_softap_fails);
}
