# WiFi Manager v1.3

Reusable WiFi management module for ESP32 projects with persistent credential storage and BT configuration interface.

## Features

✅ **Persistent Storage**
- Store up to 3 WiFi networks in NVS (Non-Volatile Storage)
- Auto-load credentials on boot
- Clear/update credentials via BT interface

✅ **Auto-Connection**
- Try connecting to stored networks by priority
- Automatic timeout and failover
- Connection state monitoring
- Non-blocking reconnect state machine in `update()`
- Backoff reconnect after disconnects

✅ **Fallback AP Mode**
- Web-based configuration interface
- Network scanning and listing
- Set credentials remotely
- Dedicated `WIFI_AP_PASSWORD` for valid ESP32 SoftAP WPA password length
- AP timeout returns to reconnect loop instead of rebooting the board

✅ **Bluetooth Interface**
- Command-based control via BT (password: "37")
- ESP32 runs as Bluetooth SPP server/slave so phones connect to it as a console
- Monitor WiFi status
- Save/list/clear credentials
- Scan available networks
- Live logger streaming

✅ **Memory Efficient**
- Minimal dependencies (built-in Arduino/ESP32 libraries only)
- JSON support optional (ArduinoJson v7)
- Fits in ~1.3MB FLASH with room for other features

## Architecture

```
WiFiManager/
├── src/
│   ├── WiFiManager.cpp/h       - Main connection logic & AP mode
│   ├── WiFiStorageManager.cpp/h - Handles NVS persistence
│   ├── BTCommandHandler.cpp/h   - Bluetooth CLI interface
│   └── Logger.cpp/h             - Unified logging (serial + BT)
├── include/
│   └── config.h                 - Configuration constants
└── library.json                 - PlatformIO metadata
```

## Installation

### Using PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/honza540/WiFiManager.git
```

Or with a specific version:

```ini
lib_deps =
    https://github.com/honza540/WiFiManager.git#v1.3.0
```

### Manual Installation

Copy the entire folder to your project's `lib/` directory:
```bash
cp -r WiFiManager /path/to/your/project/lib/
```

## Usage

### Basic Setup (main.cpp)

```cpp
#include "WiFiManager.h"
#include "BTCommandHandler.h"

void setup() {
    Serial.begin(115200);
    Logger::debug(TAG_SYS, "Starting...");
    
    WiFiManager::begin();        // Initialize WiFi and built-in Bluetooth console
}

void loop() {
    WiFiManager::update();       // Check WiFi status & AP mode
    BTCommandHandler::update();  // Process BT commands
}
```

### Configuration

Edit `include/config.h` in your project to customize:
- Hardware pins
- WiFi timeouts
- WiFi AP setup password (`WIFI_AP_PASSWORD`, 8+ chars for WPA/WPA2)
- BT device name & password
- Log levels
- Storage namespace

To make the library consume your project `include/config.h`, add a small
`include/wifimanager_user_config.h` wrapper:

```cpp
#pragma once
#include "config.h"
```

**Compatibility:** The library still ships defaults in its own `include/config.h`.
Projects can override any macro from `wifimanager_user_config.h`.

**Behavior note:** `WiFiManager::begin()` now starts connection work and returns quickly. Call `WiFiManager::update()` in `loop()` as before, and gate network-dependent application work on `WiFiManager::getState() == WM_CONNECTED` or `WiFi.status() == WL_CONNECTED`.

### BT Commands

Connect via Bluetooth (password: "37") and type commands:

**Configuration:**
- `wifi-set 0 MyNetwork MyPassword` - Save WiFi
- `wifi-list` - Show stored networks
- `wifi-clear 0` - Remove WiFi at index 0

**Monitoring:**
- `status` - System status
- `netstatus` - Network details (IP, RSSI, etc.)
- `netmonitor` - Scan available networks

**Utilities:**
- `reboot` - Restart device
- `help` - Show all commands

## Memory Requirements

- **FLASH**: ~60-80 KB (core code)
- **RAM**: ~15 KB runtime
- **NVS Storage**: ~512 B per WiFi network (3 networks max = ~1.5 KB)

Compatible with az-delivery-devkit-v4 and other ESP32 boards with 1.3 MB+ FLASH.

## Logging

The library uses a unified logging system that supports both Serial and Bluetooth output:

```cpp
#include "Logger.h"

// Log at different levels
LOG_DEBUG(TAG_WIFI, "Debug message");
LOG_INFO(TAG_WIFI, "Info message");
LOG_WARN(TAG_WIFI, "Warning message");
LOG_ERROR(TAG_WIFI, "Error message");
```

Configure logging level in `config.h`:
```cpp
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#define LOG_TO_SERIAL 1
#define LOG_TO_BT 1
```

## Extending BT Commands

To add custom BT commands in your project, implement the `ICommandHandler` interface:

```cpp
#include "ICommandHandler.h"

class MyCommands : public ICommandHandler {
public:
    bool handleCommand(const String& cmd, const String& args) override {
        if (cmd == "mycommand") {
            // Handle your command
            BTCommandHandler::sendResponse("My command executed!");
            return true;
        }
        return false;
    }
    
    String getHelpText() override {
        return "mycommand - Execute my custom command";
    }
};

// In setup():
MyCommands* myCmds = new MyCommands();
BTCommandHandler::registerCommandHandler(myCmds);
```

## API Reference

### WiFiManager

```cpp
WiFiManager::begin();                           // Initialize
WiFiManager::update();                          // Main loop call
bool WiFiManager::connectToStoredNetwork();     // Connect to saved WiFi
void WiFiManager::startAPMode();                // Start AP config mode
void WiFiManager::stopAPMode();                 // Stop AP mode
WiFiManagerState WiFiManager::getState();       // Get current state
String WiFiManager::getStatusString();          // Get status text
String WiFiManager::getSSID();                  // Get connected SSID
String WiFiManager::getIP();                    // Get IP address
String WiFiManager::getSignalStrength();        // Get signal strength
```

### WiFiStorageManager

```cpp
WiFiStorageManager::begin();                           // Initialize NVS
bool WiFiStorageManager::saveCredential(idx, ssid, pwd);  // Save WiFi
WiFiCredential WiFiStorageManager::loadCredential(idx);   // Load WiFi
WiFiCredential* WiFiStorageManager::loadAllCredentials(count); // Load all
uint8_t WiFiStorageManager::getCredentialCount();      // Count stored
void WiFiStorageManager::clearCredential(idx);         // Clear one
void WiFiStorageManager::clearAll();                   // Clear all
```

### BTCommandHandler

```cpp
BTCommandHandler::begin();      // Initialize Bluetooth
BTCommandHandler::update();     // Main loop call
bool BTCommandHandler::isConnected();  // Check BT connection
```

## Future Enhancements

- [ ] HTTPS support for AP mode config
- [x] WiFi automatic reconnection on disconnect
- [ ] Signal strength monitoring log
- [ ] Web dashboard with React/Vue frontend
- [ ] mDNS autodiscovery
- [ ] Over-the-air (OTA) updates
- [ ] Multiple project profiles (save/load configurations)

## License

MIT

## Author

Reusable WiFiManager module (2026)

---

**Version**: 1.3.0 (Stable)
**Status**: Production Ready for Testing
