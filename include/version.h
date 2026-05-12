#ifndef WIFIMANAGER_VERSION_H
#define WIFIMANAGER_VERSION_H

// ============================================================================
// WiFiManager Version
// ============================================================================
// Format: MAJOR.MINOR.PATCH
// 1.3.0 - Built-in BT startup, safer AP save flow, and project config override

#define WIFIMANAGER_VERSION_MAJOR 1
#define WIFIMANAGER_VERSION_MINOR 3
#define WIFIMANAGER_VERSION_PATCH 0
#define WIFIMANAGER_VERSION_STRING "1.3.0"

// Changelog:
// v1.3.0 (2026-05-13)
//   - WiFiManager::begin() starts the built-in Bluetooth service console
//   - BTCommandHandler::begin() is idempotent and cleans up failed starts
//   - Bluetooth SSP pairing callbacks are registered for easier phone/PC pairing
//   - AP save no longer tears down WebServer inside the active request handler
//   - Empty fixed fallback SSID is skipped instead of calling WiFi.begin("")
//   - Added optional application override through wifimanager_user_config.h
//
// v1.2.0 (2026-05-11)
//   - begin() starts WiFi connection in non-blocking mode; update() advances it
//   - Added reconnect backoff and AP timeout without forced reboot
//   - Added dedicated WIFI_AP_PASSWORD for valid ESP32 SoftAP WPA password length
//   - Bluetooth console now runs as slave/server so phones connect to ESP32
//   - WiFi scan endpoints use async scan and release scan results
//
// v1.1.0 (2026-05-07)
//   - Added ICommandHandler interface for modular BT commands
//   - Refactored BTCommandHandler as orchestrator with handler registry
//   - Extracted WiFiManagerCommands as separate handler module
//   - Supports external project command registration
//
// v1.0.0 (initial)
//   - WiFi management (auto-connect, AP fallback)
//   - Persistent credential storage
//   - BT configuration interface
//   - Logging system

#endif // WIFIMANAGER_VERSION_H
