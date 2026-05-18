#include <unity.h>
#include "WiFiStorageManager.h"

static void reset_storage_manager() {
    WiFiStorageManager::begin();
    WiFiStorageManager::clearAll();
}

void test_storage_manager_save_load_roundtrip() {
    reset_storage_manager();

    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(0, "HomeWiFi", "SecretPass"));

    WiFiCredential credential = WiFiStorageManager::loadCredential(0);
    TEST_ASSERT_TRUE(credential.valid);
    TEST_ASSERT_EQUAL_STRING("HomeWiFi", credential.ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("SecretPass", credential.password.c_str());
    TEST_ASSERT_EQUAL_UINT8(1, WiFiStorageManager::getCredentialCount());
}

void test_storage_manager_rejects_invalid_inputs() {
    reset_storage_manager();

    TEST_ASSERT_FALSE(WiFiStorageManager::saveCredential(WIFI_MAX_CREDENTIALS, "BadIndex", "pass"));
    TEST_ASSERT_FALSE(WiFiStorageManager::saveCredential(0, "", "pass"));
    TEST_ASSERT_FALSE(WiFiStorageManager::saveCredential(0, "ThisSSIDIsLongerThanThirtyTwoCharacters", "pass"));
    TEST_ASSERT_FALSE(WiFiStorageManager::saveCredential(0, "ValidSSID", "12345678901234567890123456789012345678901234567890123456789012345"));
    TEST_ASSERT_EQUAL_UINT8(0, WiFiStorageManager::getCredentialCount());
}

void test_storage_manager_clear_credential_recomputes_count() {
    reset_storage_manager();

    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(0, "First", "one"));
    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(1, "Second", "two"));
    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(2, "Third", "three"));

    WiFiStorageManager::clearCredential(2);
    TEST_ASSERT_EQUAL_UINT8(2, WiFiStorageManager::getCredentialCount());

    WiFiStorageManager::clearCredential(0);
    TEST_ASSERT_EQUAL_UINT8(2, WiFiStorageManager::getCredentialCount());
    TEST_ASSERT_FALSE(WiFiStorageManager::loadCredential(0).valid);
    TEST_ASSERT_TRUE(WiFiStorageManager::loadCredential(1).valid);

    WiFiStorageManager::clearCredential(1);
    TEST_ASSERT_EQUAL_UINT8(0, WiFiStorageManager::getCredentialCount());
}

void test_storage_manager_load_all_preserves_index_holes() {
    reset_storage_manager();

    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(2, "ThirdOnly", "pass"));

    uint8_t count = 0;
    WiFiCredential *credentials = WiFiStorageManager::loadAllCredentials(count);

    TEST_ASSERT_EQUAL_UINT8(3, count);
    TEST_ASSERT_NOT_NULL(credentials);
    TEST_ASSERT_FALSE(credentials[0].valid);
    TEST_ASSERT_FALSE(credentials[1].valid);
    TEST_ASSERT_TRUE(credentials[2].valid);
    TEST_ASSERT_EQUAL_STRING("ThirdOnly", credentials[2].ssid.c_str());

    delete[] credentials;
}
