#include "BTCommandHandler.h"
#include "WiFiManagerCommands.h"

// Static member initialization
BluetoothSerial* BTCommandHandler::serialBT = nullptr;
bool BTCommandHandler::btConnected = false;
String BTCommandHandler::commandBuffer = "";
unsigned long BTCommandHandler::lastHeartbeat = 0;
const char* BTCommandHandler::TAG = "BT";
std::vector<ICommandHandler*> BTCommandHandler::commandHandlers;

void BTCommandHandler::begin() {
    serialBT = new BluetoothSerial();

    LOG_INFO(TAG, "Starting Bluetooth with name: " BT_DEVICE_NAME);

    // BluetoothSerial role matters:
    // - Slave/server mode (isMaster=false): ESP32 advertises an SPP service.
    //   A phone/PC initiates the connection and uses ESP32 as a serial console.
    //   This is what we want for a maintenance/debug terminal.
    // - Master/client mode (isMaster=true): ESP32 actively connects to another
    //   Bluetooth SPP server. That is useful for peripherals, but awkward for
    //   a phone terminal because the phone expects to connect to the ESP32.
    serialBT->setPin(BT_PASSWORD);
    if (!serialBT->begin(BT_DEVICE_NAME, false)) {
        LOG_ERROR(TAG, "Failed to start Bluetooth");
        return;
    }

    // Set BT as logger output
    Logger::setBTStream(serialBT);

    LOG_INFO(TAG, "Bluetooth started. Pin: " BT_PASSWORD);
    sendResponse("WiFiManager BT Interface Ready");

    // Register WiFi command handler
    registerCommandHandler(new WiFiManagerCommands());

    printHelp();
}

void BTCommandHandler::update() {
    if (serialBT == nullptr) {
        return;
    }

    // Check connection status
    if (serialBT->connected()) {
        btConnected = true;
        lastHeartbeat = millis();
    } else {
        if (btConnected) {
            LOG_WARN(TAG, "BT disconnected");
            btConnected = false;
        }
    }

    // Read incoming data
    while (serialBT->available()) {
        char c = serialBT->read();

        if (c == '\r' || c == '\n') {
            if (commandBuffer.length() > 0) {
                parseCommand(commandBuffer);
                commandBuffer = "";
            }
        } else if (c >= 32 && c < 127) { // Printable characters
            commandBuffer += c;
            serialBT->print(c); // Echo
        }
    }
}

bool BTCommandHandler::isConnected() {
    return serialBT != nullptr && serialBT->connected();
}

BluetoothSerial* BTCommandHandler::getSerialStream() {
    return serialBT;
}

void BTCommandHandler::registerCommandHandler(ICommandHandler* handler) {
    if (handler != nullptr) {
        commandHandlers.push_back(handler);
        LOG_DEBUG(TAG, "Command handler registered");
    }
}

void BTCommandHandler::parseCommand(const String &fullCommand) {
    String cmd = fullCommand;
    String args = "";

    // Split command and arguments
    int spacePos = cmd.indexOf(' ');
    if (spacePos > 0) {
        args = cmd.substring(spacePos + 1);
        cmd = cmd.substring(0, spacePos);
    }

    // Convert to lowercase
    cmd.toLowerCase();
    args.trim();

    LOG_DEBUG(TAG, "Command: " + cmd + " | Args: " + args);

    executeCommand(cmd, args);
}

void BTCommandHandler::executeCommand(const String &cmd, const String &args) {
    // Check built-in commands first
    if (cmd == "help") {
        cmd_help();
        return;
    } else if (cmd == "reboot") {
        cmd_reboot();
        return;
    }

    // Try registered command handlers
    for (ICommandHandler* handler : commandHandlers) {
        if (handler->handleCommand(cmd, args)) {
            return; // Command was handled
        }
    }

    // Command not found
    sendError("Unknown command: " + cmd + ". Type 'help' for available commands.");
}

void BTCommandHandler::cmd_help() {
    printHelp();
}

void BTCommandHandler::cmd_reboot() {
    sendResponse("Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();
}

void BTCommandHandler::printHelp() {
    String help = R"(
=== WiFiManager BT Command Help ===

System:
  help                       - Show this help
  reboot                     - Reboot device

)";
    sendResponse(help, false);

    // Append help from registered handlers
    for (ICommandHandler* handler : commandHandlers) {
        sendResponse(handler->getHelpText(), false);
    }

    sendResponse("=====================================");
}

void BTCommandHandler::sendResponse(const String &message, bool newline) {
    if (serialBT != nullptr && serialBT->connected()) {
        if (newline) {
            serialBT->println(message);
        } else {
            serialBT->print(message);
        }
    }
}

void BTCommandHandler::sendError(const String &message) {
    if (serialBT != nullptr && serialBT->connected()) {
        serialBT->println("[ERROR] " + message);
    }
}
