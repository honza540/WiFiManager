#include <unity.h>
#include "WiFiManager.h"

static WebServer* start_ap_server_for_test() {
    WiFiManager::stopAPMode();
    WiFiStorageManager::begin();
    WiFiStorageManager::clearAll();
    WiFiManager::startAPMode();

    WebServer *server = WiFiManager::getWebServer();
    TEST_ASSERT_NOT_NULL(server);
    return server;
}

static void assert_contains(const String &content, const char *expected) {
    TEST_ASSERT_TRUE_MESSAGE(content.indexOf(expected) >= 0, expected);
}

void test_ap_mode_registers_expected_web_routes() {
    WebServer *server = start_ap_server_for_test();

    TEST_ASSERT_TRUE(server->isStarted());
    TEST_ASSERT_EQUAL_INT(WEB_SERVER_PORT, server->port());
    TEST_ASSERT_TRUE(server->routeRegistered("/", HTTP_GET));
    TEST_ASSERT_TRUE(server->routeRegistered("/config", HTTP_GET));
    TEST_ASSERT_TRUE(server->routeRegistered("/status", HTTP_GET));
    TEST_ASSERT_TRUE(server->routeRegistered("/scan", HTTP_GET));
    TEST_ASSERT_TRUE(server->routeRegistered("/save", HTTP_POST));
    TEST_ASSERT_FALSE(server->routeRegistered("/save", HTTP_GET));

    WiFiManager::stopAPMode();
}

void test_ap_root_route_returns_setup_page() {
    WebServer *server = start_ap_server_for_test();

    TEST_ASSERT_TRUE(server->simulateRequest("/", HTTP_GET));

    TEST_ASSERT_EQUAL_INT(200, server->getLastStatusCode());
    TEST_ASSERT_EQUAL_STRING("text/html", server->getLastContentType().c_str());
    assert_contains(server->getLastBody(), "WiFi Configuration");
    assert_contains(server->getLastBody(), "<form action=\"/save\" method=\"POST\">");

    WiFiManager::stopAPMode();
}

void test_ap_status_route_reports_state_and_credential_count() {
    WebServer *server = start_ap_server_for_test();
    TEST_ASSERT_TRUE(WiFiStorageManager::saveCredential(0, "StoredOne", "pass"));

    TEST_ASSERT_TRUE(server->simulateRequest("/status", HTTP_GET));

    TEST_ASSERT_EQUAL_INT(200, server->getLastStatusCode());
    TEST_ASSERT_EQUAL_STRING("text/plain", server->getLastContentType().c_str());
    assert_contains(server->getLastBody(), "State: AP_MODE");
    assert_contains(server->getLastBody(), "Credentials stored: 1");

    WiFiManager::stopAPMode();
}

void test_ap_save_route_rejects_missing_password() {
    WebServer *server = start_ap_server_for_test();
    server->setArg("ssid", "OnlySSID");

    TEST_ASSERT_TRUE(server->simulateRequest("/save", HTTP_POST));

    TEST_ASSERT_EQUAL_INT(400, server->getLastStatusCode());
    TEST_ASSERT_EQUAL_STRING("text/plain", server->getLastContentType().c_str());
    TEST_ASSERT_EQUAL_STRING("Missing SSID or password", server->getLastBody().c_str());
    TEST_ASSERT_EQUAL_UINT8(0, WiFiStorageManager::getCredentialCount());

    WiFiManager::stopAPMode();
}

void test_ap_save_route_stores_credentials_and_returns_saved_page() {
    WebServer *server = start_ap_server_for_test();
    server->setArg("ssid", "SetupSSID");
    server->setArg("pass", "SetupPass");
    server->setArg("idx", "1");

    TEST_ASSERT_TRUE(server->simulateRequest("/save", HTTP_POST));

    TEST_ASSERT_EQUAL_INT(200, server->getLastStatusCode());
    TEST_ASSERT_EQUAL_STRING("text/html", server->getLastContentType().c_str());
    assert_contains(server->getLastBody(), "Saved!");

    WiFiCredential credential = WiFiStorageManager::loadCredential(1);
    TEST_ASSERT_TRUE(credential.valid);
    TEST_ASSERT_EQUAL_STRING("SetupSSID", credential.ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("SetupPass", credential.password.c_str());

    WiFiManager::stopAPMode();
}
