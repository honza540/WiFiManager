# WiFiManager BT Integration Guide for PoolFilterWeb

## Architektura

WiFiManager je nyní modulární projekt s **registračním mechanismem** pro Bluetooth příkazy.

```
BTCommandHandler (Orchestrator)
├── Vestavěné příkazy: help, reboot
├── WiFiManagerCommands (zaregistrováno)
└── [PoolFilterWeb] PoolFilterCommands (zaregistrujete vy)
```

## Poznamky pro v1.2

- `WiFiManager::begin()` uz dlouze neblokuje v `setup()`. Pripojovani bezi pres `WiFiManager::update()`, takze pool filter logika musi dal volat `update()` v kazde iteraci `loop()`.
- Kod, ktery potrebuje internet/LAN, by mel cekat na `WiFiManager::getState() == WM_CONNECTED`.
- Bluetooth zustava zapnute jako servisni/debug konzole, ale ESP32 je ted SPP slave/server. Telefon nebo notebook se pripojuje k ESP32; ESP32 se nepokousi aktivne pripojit k telefonu.
- Pro konfiguracni AP pridejte do vlastniho `include/config.h` idealne `#define WIFI_AP_PASSWORD "alespon8znaku"`. Knihovna ma default, takze stare projekty kvuli tomu neprestanou kompilovat.

## Jak zaregistrovat vlastní příkazy z PoolFilterWeb

### 1. Vytvořit třídu implementující `ICommandHandler`

Vytvořte `PoolFilterCommands.h`:

```cpp
#ifndef POOL_FILTER_COMMANDS_H
#define POOL_FILTER_COMMANDS_H

#include <Arduino.h>
#include "ICommandHandler.h"  // Cestu přizpůsobte SVÉ struktuře

class PoolFilterCommands : public ICommandHandler {
public:
    PoolFilterCommands();

    bool handleCommand(const String& cmd, const String& args) override;
    String getHelpText() override;

private:
    // Relay commands
    void cmd_relaystatus();
    void cmd_relayset(const String &args);
    void cmd_start();
    void cmd_stop();
    void cmd_relayreset();

    // Helper
    void sendResponse(const String &message, bool newline = true);
};

#endif // POOL_FILTER_COMMANDS_H
```

### 2. Implementovat v `PoolFilterCommands.cpp`

```cpp
#include "PoolFilterCommands.h"
#include "BTCommandHandler.h"  // Pro sendResponse

PoolFilterCommands::PoolFilterCommands() {
    // Inicializace
}

bool PoolFilterCommands::handleCommand(const String& cmd, const String& args) {
    if (cmd == "relaystatus") {
        cmd_relaystatus();
        return true;
    } else if (cmd == "relayset") {
        cmd_relayset(args);
        return true;
    } else if (cmd == "start") {
        cmd_start();
        return true;
    } else if (cmd == "stop") {
        cmd_stop();
        return true;
    } else if (cmd == "relayreset") {
        cmd_relayreset();
        return true;
    }
    return false;  // Command not handled
}

String PoolFilterCommands::getHelpText() {
    return R"(
Relay Control:
  relaystatus                  - Show relay config
  relayset <params>            - Configure relay intervals
  start                        - Start filtering
  stop                         - Stop filtering
  relayreset                   - Reset to defaults
)";
}

void PoolFilterCommands::cmd_relaystatus() {
    sendResponse("=== Relay Status ===");
    // Vaše implementace
    sendResponse("====================");
}

void PoolFilterCommands::cmd_relayset(const String &args) {
    // Vaše implementace
    sendResponse("Relay settings updated");
}

void PoolFilterCommands::cmd_start() {
    sendResponse("Filter started");
}

void PoolFilterCommands::cmd_stop() {
    sendResponse("Filter stopped");
}

void PoolFilterCommands::cmd_relayreset() {
    sendResponse("Relay reset to defaults");
}

void PoolFilterCommands::sendResponse(const String &message, bool newline) {
    BluetoothSerial* btSerial = BTCommandHandler::getSerialStream();
    if (btSerial != nullptr && btSerial->connected()) {
        if (newline) {
            btSerial->println(message);
        } else {
            btSerial->print(message);
        }
    }
}
```

### 3. Zaregistrovat v PoolFilterWeb main

Někde ve vašem main/setup kódu:

```cpp
#include "BTCommandHandler.h"
#include "PoolFilterCommands.h"

void setup() {
    // ... ostatní inicializace ...
    
    // Inicializovat Bluetooth
    BTCommandHandler::begin();
    
    // Zaregistrovat pool filter příkazy
    BTCommandHandler::registerCommandHandler(new PoolFilterCommands());
    
    // ... zbytek kodu ...
}

void loop() {
    BTCommandHandler::update();  // Nezapomeňte volat v loop!
    // ... zbytek kodu ...
}
```

## Výsledek

Vaši uživatelé uvidí v `help`:

```
=== WiFiManager BT Command Help ===

System:
  help                       - Show this help
  reboot                     - Reboot device

WiFi Configuration:
  wifi-set <idx> <ssid> <pass>  - Save WiFi credentials
  wifi-clear <idx>              - Clear WiFi at index
  wifi-list                     - List stored networks
  netconnect <idx>              - Connect to stored network

Monitoring:
  status                        - System status
  netstatus                     - Network details
  netmonitor                   - Scan available networks
  live                         - Stream logger output

Relay Control:
  relaystatus                  - Show relay config
  relayset <params>            - Configure relay intervals
  start                        - Start filtering
  stop                         - Stop filtering
  relayreset                   - Reset to defaults

=====================================
```

## Přednosti architektury

✅ **Čistě odděleno**: WiFiManager se stará jen o WiFi  
✅ **Reusable**: Můžete WiFiManager použít v kterémkoliv projektu  
✅ **Modulární**: Snadno přidáte nové příkazy z jiných modulů  
✅ **Nezávislé**: Žádné cross-dependencies  
✅ **Testovatelné**: Každou třídu lze testovat независимо  

---

**Poznámka**: `ICommandHandler` interface najdete v `src/ICommandHandler.h`
