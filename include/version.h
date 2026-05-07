#ifndef WIFIMANAGER_VERSION_H
#define WIFIMANAGER_VERSION_H

// ============================================================================
// WiFiManager Version
// ============================================================================
// Format: MAJOR.MINOR.PATCH
// 1.1.0 - Added modular BT command handler system with ICommandHandler interface

#define WIFIMANAGER_VERSION_MAJOR 1
#define WIFIMANAGER_VERSION_MINOR 1
#define WIFIMANAGER_VERSION_PATCH 0
#define WIFIMANAGER_VERSION_STRING "1.1.0"

// Changelog:
// v1.1.0 (2026-05-07)
//   - Added ICommandHandler interface for modular BT commands
//   - Refactored BTCommandHandler as orchestrator with handler registry
//   - Extracted WiFiManagerCommands as separate handler module
//   - Supports external command registration (e.g., PoolFilterWeb)
//
// v1.0.0 (initial)
//   - WiFi management (auto-connect, AP fallback)
//   - Persistent credential storage
//   - BT configuration interface
//   - Logging system

#endif // WIFIMANAGER_VERSION_H
