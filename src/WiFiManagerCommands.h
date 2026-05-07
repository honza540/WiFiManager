#ifndef WIFI_MANAGER_COMMANDS_H
#define WIFI_MANAGER_COMMANDS_H

#include <Arduino.h>
#include "ICommandHandler.h"
#include "WiFiManager.h"
#include "WiFiStorageManager.h"

/**
 * WiFi Manager Commands
 * Handles all WiFi-related commands:
 * - wifi-set, wifi-clear, wifi-list, netconnect
 * - status, netstatus, netmonitor
 * - live (logger streaming)
 */
class WiFiManagerCommands : public ICommandHandler {
public:
    WiFiManagerCommands();

    bool handleCommand(const String& cmd, const String& args) override;
    String getHelpText() override;

private:
    static const char* TAG;

    // WiFi configuration commands
    void cmd_wifiset(const String &args);
    void cmd_wificlear(const String &args);
    void cmd_wifilist();
    void cmd_netconnect(const String &args);

    // Monitoring commands
    void cmd_status();
    void cmd_netstatus();
    void cmd_netmonitor();
    void cmd_live();

    // Helper methods
    void sendResponse(const String &message, bool newline = true);
    void sendError(const String &message);
};

#endif // WIFI_MANAGER_COMMANDS_H
