#include "WiFiManagerCommands.h"
#include "BTCommandHandler.h"
#include "Logger.h"

const char* WiFiManagerCommands::TAG = "WiFiCmd";

// ============================================================================
// COMMAND ROUTING
// ============================================================================

WiFiManagerCommands::WiFiManagerCommands() {
    LOG_DEBUG(TAG, "WiFiManagerCommands initialized");
}
bool WiFiManagerCommands::handleCommand(const String& cmd, const String& args) {
    if (cmd == "wifi-set") {
        cmd_wifiset(args);
        return true;
    } else if (cmd == "wifi-clear") {
        cmd_wificlear(args);
        return true;
    } else if (cmd == "wifi-list") {
        cmd_wifilist();
        return true;
    } else if (cmd == "netconnect") {
        cmd_netconnect(args);
        return true;
    } else if (cmd == "status") {
        cmd_status();
        return true;
    } else if (cmd == "netstatus") {
        cmd_netstatus();
        return true;
    } else if (cmd == "netmonitor") {
        cmd_netmonitor();
        return true;
    } else if (cmd == "live") {
        cmd_live();
        return true;
    }
    return false;
}
String WiFiManagerCommands::getHelpText() {
    return R"(
Monitoring:
  status                        - System status
  netstatus                     - Network details
  netmonitor                   - Scan available networks
  live                         - Stream logger output

WiFi Configuration:
  wifi-set <idx> <ssid> <pass>  - Save WiFi credentials and connect
  wifi-clear <idx>              - Clear WiFi at index
  wifi-list                     - List stored networks
  netconnect <idx>              - Connect to stored network
)";
}
void WiFiManagerCommands::sendResponse(const String &message, bool newline) {
    BTCommandHandler::sendResponse(message, newline);
}
void WiFiManagerCommands::sendError(const String &message) {
    BTCommandHandler::sendError(message);
}
