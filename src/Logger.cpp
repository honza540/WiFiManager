#include "Logger.h"
#include "config.h"

// ============================================================================
// STATIC MEMBER INITIALIZATION - Inicializace statických proměnných třídy
// ============================================================================

// Bluetooth stream - ukazatel na BluetoothSerial objekt (inicializuje BTCommandHandler)
Stream* Logger::btStream = nullptr;

// Příznak zda se loguje na sériový port (výchozí: ANO)
bool Logger::serialEnabled = true;

// Příznak zda se loguje na Bluetooth (výchozí: NE, zapne se až když se BT připojí)
bool Logger::btEnabled = false;

// ============================================================================
// PUBLIC METHODS - Veřejné metody třídy
// ============================================================================

/**
 * Hlavní logování - filtruje zprávy podle úrovně a vysílá na Serial/BT
 * 
 * Postup:
 * 1. Kontrola: Je tato úroveň důležitá než nastavená minimální úroveň?
 * 2. Pokud NE - ignoruj zprávu
 * 3. Pokud ANO - vytvoř kompletní log řetězec s časem a tagem
 * 4. Pošli na Serial a/nebo BT
 */
void Logger::log(Level level, const String &tag, const String &message) {
    log(level, tag, message, true);
}

void Logger::log(Level level, const String &tag, const String &message, bool includeBT) {
    // Kontrola: Je tato zpráva důležitá dost na to, aby se vypsala?
    // Pokud máme nastavenu úroveň INFO, DEBUG zprávy se ignorují
    if (level > CURRENT_LOG_LEVEL) {
        return;
    }

    // Vytvoření kompletního log řetězce s formátem:
    // [čas] [ÚROVEŇ] [tag] zpráva
    String levelStr = levelToString(level);
    String timestamp = getTimestamp();
    String logMessage = "[" + timestamp + "] [" + levelStr + "] [" + tag + "] " + message;

    // Odeslání na sériový port (USB, vidět v Serial Monitoru)
    if (serialEnabled) {
        Serial.println(logMessage);
    }

    // Odeslání na Bluetooth (vidět v BT terminálu)
    if (includeBT && btEnabled && btStream != nullptr) {
        btStream->println(logMessage);
    }
}

/**
 * DEBUG úroveň - nejpodrobněji, pro vývojáře
 * Používáno: LOG_DEBUG(TAG_WIFI, "Trying network 0");
 */
void Logger::debug(const String &tag, const String &message) {
    log(DEBUG, tag, message);
}

/**
 * INFO úroveň - důležité informace
 * Používáno: LOG_INFO(TAG_WIFI, "Connected successfully");
 */
void Logger::info(const String &tag, const String &message) {
    log(INFO, tag, message);
}

/**
 * WARN úroveň - varování, možný problém
 * Používáno: LOG_WARN(TAG_WIFI, "Signal strength very weak");
 */
void Logger::warn(const String &tag, const String &message) {
    log(WARN, tag, message);
}

/**
 * ERROR úroveň - kritické chyby
 * Používáno: LOG_ERROR(TAG_RELAY, "Relay stuck on GPIO 13");
 */
void Logger::error(const String &tag, const String &message) {
    log(ERROR, tag, message);
}

// Vypnout výstup na Serial (zůstane jen BT)
void Logger::enableSerial(bool enable) {
    serialEnabled = enable;
}

// Vypnout výstup na Bluetooth (zůstane jen Serial)
void Logger::enableBT(bool enable) {
    btEnabled = enable;
}

// Vrátit aktuální nastavenou úroveň logování
Logger::Level Logger::getCurrentLevel() {
    return (Level)CURRENT_LOG_LEVEL;
}

/**
 * Nastavit Bluetooth stream pro logování
 * Volá se z BTCommandHandler::begin()
 * @param stream - Ukazatel na BluetoothSerial objekt
 */
void Logger::setBTStream(Stream* stream) {
    btStream = stream;
    if (stream != nullptr) {
        btEnabled = true;
        LOG_INFO(TAG_SYS, "BT Logger enabled");
    } else {
        btEnabled = false;
    }
}

// ============================================================================
// PRIVATE HELPER METHODS - Pomocné privátní metody
// ============================================================================

/**
 * Vytvoření časového razítka od startu destiček
 * Formát: sekund.milisekundy (např. "12.345" = 12.345 sekund od startu)
 * 
 * @return String s časovým razítkem
 */
String Logger::getTimestamp() {
    // Celkový čas v ms od startu
    unsigned long ms = millis();
    
    // Oddělení na sekund a zbývající ms
    unsigned long sec = ms / 1000;
    unsigned long ms_part = ms % 1000;
    
    // Formátování na řetězec
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%lu.%03lu", sec, ms_part);
    return String(buffer);
}

/**
 * Převod úrovně na text pro výpis
 * DEBUG -> "DEBUG"
 * INFO -> "INFO "
 * WARN -> "WARN "
 * ERROR -> "ERROR"
 * 
 * @param level - Úroveň logu
 * @return Текстовое представление
 */
String Logger::levelToString(Level level) {
    switch (level) {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO ";  // Stejná délka jako ERROR
        case WARN:
            return "WARN ";  // Stejná délka jako ERROR
        case ERROR:
            return "ERROR";
        default:
            return "NONE ";
    }
}

/**
 * ANSI barvové kódy pro terminál
 * Zajišťuje barevný výstup v Serial Monitoru VS Code
 * (Bluetooth kódy ignoruje, zobrazuje jen jako text)
 * 
 * Barvy:
 * DEBUG  - Cyan (světle modré)
 * INFO   - Green (zelené)
 * WARN   - Yellow (žluté)
 * ERROR  - Red (červené)
 * 
 * @param level - Úroveň logu
 * @return ANSI escape sekvence
 */
String Logger::getLevelColor(Level level) {
    // Příklad: "\033[32m" znamená "barva zelená"
    // "\033[0m" znamená "zrušit všechny barvy"
    switch (level) {
        case DEBUG:
            return "\033[36m";  // Cyan - DEBUG zprávy
        case INFO:
            return "\033[32m";  // Green - INFO zprávy
        case WARN:
            return "\033[33m";  // Yellow - WARNING zprávy
        case ERROR:
            return "\033[31m";  // Red - ERROR zprávy
        default:
            return "\033[0m";   // Reset na normální
    }
}
