#include <unity.h>
#include "WiFiManagerWebTemplates.h"

static void assert_contains(const String &content, const char *expected) {
    TEST_ASSERT_TRUE_MESSAGE(content.indexOf(expected) >= 0, expected);
}

void test_setup_page_contains_wifi_form_contract() {
    String html = WiFiManagerWebTemplates::setupPage();

    assert_contains(html, "<!DOCTYPE html>");
    assert_contains(html, "<form action=\"/save\" method=\"POST\"");
    assert_contains(html, "name=\"ssid\"");
    assert_contains(html, "name=\"pass\"");
    assert_contains(html, "name=\"idx\"");
    assert_contains(html, "type=\"radio\" name=\"idx\" value=\"0\"");
    assert_contains(html, "id=\"credentialRows\"");
    assert_contains(html, "id=\"scanRows\"");
    assert_contains(html, "/api/setup");
    assert_contains(html, "/api/scan");
    assert_contains(html, "AP mode active");
}

void test_saved_page_contains_confirmation_contract() {
    String html = WiFiManagerWebTemplates::savedPage();

    assert_contains(html, "<!DOCTYPE html>");
    assert_contains(html, "<h2>Saved!</h2>");
    assert_contains(html, "WiFi credentials saved");
    assert_contains(html, "Connecting...");
    assert_contains(html, "location.replace('/')");
}
