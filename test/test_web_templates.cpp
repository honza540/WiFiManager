#include <unity.h>
#include "WiFiManagerWebTemplates.h"

static void assert_contains(const String &content, const char *expected) {
    TEST_ASSERT_TRUE_MESSAGE(content.indexOf(expected) >= 0, expected);
}

void test_setup_page_contains_wifi_form_contract() {
    String html = WiFiManagerWebTemplates::setupPage();

    assert_contains(html, "<!DOCTYPE html>");
    assert_contains(html, "<form action=\"/save\" method=\"POST\">");
    assert_contains(html, "name=\"ssid\"");
    assert_contains(html, "name=\"pass\"");
    assert_contains(html, "name=\"idx\"");
    assert_contains(html, "min=\"0\"");
    assert_contains(html, "max=\"2\"");
    assert_contains(html, "href=\"/scan\"");
    assert_contains(html, "href=\"/status\"");
}

void test_saved_page_contains_confirmation_contract() {
    String html = WiFiManagerWebTemplates::savedPage();

    assert_contains(html, "<!DOCTYPE html>");
    assert_contains(html, "<h2>Saved!</h2>");
    assert_contains(html, "WiFi credentials saved");
    assert_contains(html, "Connecting...");
}
