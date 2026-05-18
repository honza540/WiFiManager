#include <unity.h>
#include "WiFiManager.h"

static void reset_wifi_manager_connection_test() {
    WiFiManager::stopAPMode();
    WiFiStorageManager::begin();
    WiFiStorageManager::clearAll();
    WiFi.resetMock();
}

void test_reconnect_starts_from_requested_stored_index() {
    reset_wifi_manager_connection_test();
    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(0, "FirstSSID", "firstPass"));
    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(1, "SecondSSID", "secondPass"));

    WiFiManager::requestReconnect(1);

    TEST_ASSERT_EQUAL(WM_CONNECTING, WiFiManager::getState());
    TEST_ASSERT_EQUAL_INT(WIFI_STA, WiFi.getMode());
    TEST_ASSERT_EQUAL_STRING("SecondSSID", WiFi.getLastBeginSSID().c_str());
    TEST_ASSERT_EQUAL_STRING("secondPass", WiFi.getLastBeginPassword().c_str());

    WiFiManager::update();
    TEST_ASSERT_EQUAL(WM_CONNECTED, WiFiManager::getState());
    TEST_ASSERT_EQUAL_STRING("SecondSSID", WiFiManager::getSSID().c_str());
}

void test_reconnect_clamps_invalid_requested_index_to_zero() {
    reset_wifi_manager_connection_test();
    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(0, "FirstSSID", "firstPass"));
    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(1, "SecondSSID", "secondPass"));

    WiFiManager::requestReconnect(WIFI_MAX_CREDENTIALS + 5);

    TEST_ASSERT_EQUAL(WM_CONNECTING, WiFiManager::getState());
    TEST_ASSERT_EQUAL_STRING("FirstSSID", WiFi.getLastBeginSSID().c_str());
}

void test_reconnect_uses_fixed_fallback_when_no_credentials_exist() {
    reset_wifi_manager_connection_test();

    WiFiManager::requestReconnect();

    TEST_ASSERT_EQUAL(WM_CONNECTING, WiFiManager::getState());
    TEST_ASSERT_EQUAL_STRING(WIFI_FALLBACK_SSID, WiFi.getLastBeginSSID().c_str());
    TEST_ASSERT_EQUAL_STRING(WIFI_FALLBACK_PASSWORD, WiFi.getLastBeginPassword().c_str());

    WiFiManager::update();
    TEST_ASSERT_EQUAL(WM_CONNECTED, WiFiManager::getState());
    TEST_ASSERT_EQUAL_STRING(WIFI_FALLBACK_SSID, WiFiManager::getSSID().c_str());
}

void test_start_ap_mode_does_not_create_server_when_softap_fails() {
    reset_wifi_manager_connection_test();
    WiFi.setSoftAPResult(false);

    WiFiManager::startAPMode();

    TEST_ASSERT_NULL(WiFiManager::getWebServer());
    TEST_ASSERT_EQUAL(WM_RETRY_WAIT, WiFiManager::getState());
}
