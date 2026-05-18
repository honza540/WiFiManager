#ifndef WIFIMANAGER_CONFIG_H
#define WIFIMANAGER_CONFIG_H

#include "version.h"

// Optional project-side overrides.
// Put a wifimanager_user_config.h file in the application include path when
// the defaults below should be replaced by project-specific values.
#if defined(__has_include)
#  if __has_include("wifimanager_user_config.h")
#    include "wifimanager_user_config.h"
#  endif
#endif

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0"
#endif

#ifndef WIFIMANAGER_VERSION
#define WIFIMANAGER_VERSION WIFIMANAGER_VERSION_STRING
#endif

#ifndef WEB_SERVER_PORT
#define WEB_SERVER_PORT 80
#endif

#ifndef WIFI_MAX_CREDENTIALS
#define WIFI_MAX_CREDENTIALS 3
#endif

#ifndef WIFI_FALLBACK_SSID
#define WIFI_FALLBACK_SSID "BeSmarter"
#endif

#ifndef WIFI_FALLBACK_PASSWORD
#define WIFI_FALLBACK_PASSWORD "123456789"
#endif

#ifndef WIFI_CONNECT_TIMEOUT
#define WIFI_CONNECT_TIMEOUT 15000
#endif

#ifndef WIFI_SCAN_TIMEOUT
#define WIFI_SCAN_TIMEOUT 10000
#endif

#ifndef WIFI_RECONNECT_BASE_DELAY_MS
#define WIFI_RECONNECT_BASE_DELAY_MS 5000
#endif

#ifndef WIFI_RECONNECT_MAX_DELAY_MS
#define WIFI_RECONNECT_MAX_DELAY_MS 60000
#endif

#ifndef AP_MODE_TIMEOUT
#define AP_MODE_TIMEOUT 300
#endif

#ifndef WIFI_AP_SSID_PREFIX
#define WIFI_AP_SSID_PREFIX "AP-BeSmarter-"
#endif

#ifndef WIFI_AP_PASSWORD
#define WIFI_AP_PASSWORD "123456789"
#endif

#ifndef WIFI_AP_CHANNEL
#define WIFI_AP_CHANNEL 1
#endif

#ifndef WIFI_AP_MAX_CLIENTS
#define WIFI_AP_MAX_CLIENTS 1
#endif

#ifndef BT_DEVICE_NAME
#define BT_DEVICE_NAME "BT-BeSmarter"
#endif

#ifndef BT_PASSWORD
#define BT_PASSWORD "0000"
#endif

#ifndef LOG_LEVEL_DEBUG
#define LOG_LEVEL_DEBUG 4
#endif

#ifndef LOG_LEVEL_INFO
#define LOG_LEVEL_INFO 3
#endif

#ifndef LOG_LEVEL_WARN
#define LOG_LEVEL_WARN 2
#endif

#ifndef LOG_LEVEL_ERROR
#define LOG_LEVEL_ERROR 1
#endif

#ifndef LOG_LEVEL_NONE
#define LOG_LEVEL_NONE 0
#endif

#ifndef CURRENT_LOG_LEVEL
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#ifndef LOG_TO_SERIAL
#define LOG_TO_SERIAL 1
#endif

#ifndef LOG_TO_BT
#define LOG_TO_BT 1
#endif

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 256
#endif

#ifndef NVRAM_NAMESPACE
#define NVRAM_NAMESPACE "wifimanager"
#endif

#endif // WIFIMANAGER_CONFIG_H
