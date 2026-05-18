#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "config.h"

/**
 * Global Logger System
 * 
 * Jednotný systém pro logování zpráv na Serial (USB) a Bluetooth
 * Podporuje 4 úrovně: DEBUG, INFO, WARN, ERROR
 * 
 * Běžné použití:
 *   LOG_INFO(TAG_WIFI, "Connected to " + ssid);
 *   LOG_ERROR(TAG_RELAY, "Relay activation failed");
 */

class Logger {
public:
    // Úrovně detailnosti logů
    enum Level {
        DEBUG = 4,  // Detailní info pro debugování
        INFO = 3,   // Důležité informace o stavu
        WARN = 2,   // Varování, něco není v pořádku
        ERROR = 1,  // Kritické chyby
        NONE = 0    // Nic se nevypisuje
    };

    /**
     * Hlavní metoda pro logování zpráv
     * @param level - Úroveň importance (DEBUG/INFO/WARN/ERROR)
     * @param tag - Kategorie zprávy (např. TAG_WIFI, TAG_BT) max 4 znaky
     * @param message - Samotná zpráva
     */
    static void log(Level level, const String &tag, const String &message);
    static void log(Level level, const String &tag, const String &message, bool includeBT);
    
    // Zkrácené metody pro jednotlivé úrovně
    static void debug(const String &tag, const String &message);
    static void info(const String &tag, const String &message);
    static void warn(const String &tag, const String &message);
    static void error(const String &tag, const String &message);

    // Zapnutí/vypnutí výstupu na sériový port
    static void enableSerial(bool enable);
    
    // Zapnutí/vypnutí výstupu na Bluetooth
    static void enableBT(bool enable);

    // Zjištění aktuální úrovně logování
    static Level getCurrentLevel();
    
    /**
     * Nastavit BT stream pro wysílání logů na Bluetooth
     * Volí se automaticky v BTCommandHandler::begin()
     */
    static void setBTStream(Stream* stream);

private:
    // Bluetooth stream (z BluetoothSerial)
    static Stream* btStream;
    
    // Příznaky zda se má logovat na sériový port a BT
    static bool serialEnabled;
    static bool btEnabled;
    
    // Vytvoření časového razítka (ms od startu)
    static String getTimestamp();
    
    // Převod úrovně na text ("INFO", "ERROR", atd.)
    static String levelToString(Level level);
    
    // ANSI barvové kódy pro terminál (nepoužívá se na BT)
    static String getLevelColor(Level level);
};

// ============================================================================
// CONVENIENCE MACROS - Zkrácený zápis logování
// ============================================================================

// Příklady: LOG_DEBUG(TAG_WIFI, "Connected"); LOG_ERROR(TAG_RELAY, "Failed");
#define LOG_DEBUG(tag, msg) Logger::debug(tag, msg)
#define LOG_INFO(tag, msg) Logger::info(tag, msg)
#define LOG_WARN(tag, msg) Logger::warn(tag, msg)
#define LOG_ERROR(tag, msg) Logger::error(tag, msg)

// ============================================================================
// TAG CONSTANTS - Identifikátory kategorií
// ============================================================================

// Používáno v kódu: LOG_INFO(TAG_WIFI, "message");
#define TAG_MAIN "MAIN"    // Hlavní program
#define TAG_WIFI "WIFI"    // WiFi manager
#define TAG_BT "BT"        // Bluetooth rozhraní
#define TAG_RELAY "RLAY"   // Relé a motor
#define TAG_WEB "WEB"      // Webový server
#define TAG_SYS "SYS"      // Systémové zprávy

#endif // LOGGER_H
