#ifndef ARDUINO_HOST_H
#define ARDUINO_HOST_H

#include <string>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <thread>
#include <cstdlib>
#include <sstream>
#include <functional>
#include <map>
#include <cstdint>

using std::string;

#ifdef ARDUINO
  #if defined(__has_include_next)
    #if __has_include_next(<Arduino.h>)
      #include_next <Arduino.h>
    #else
      #include <Arduino.h>
    #endif
  #else
    #include <Arduino.h>
  #endif
#else

class String {
public:
    String() : value("") {}
    String(const char* str) : value(str ? str : "") {}
    String(const string& str) : value(str) {}
    String(char c) : value(1, c) {}
    String(int n) : value(std::to_string(n)) {}
    String(unsigned int n) : value(std::to_string(n)) {}
    String(unsigned long n) : value(std::to_string(n)) {}
    String(unsigned int n, int base) : value(convertNumber((unsigned long)n, base)) {}
    String(unsigned long n, int base) : value(convertNumber(n, base)) {}
    String(int n, int base) : value(convertNumber((unsigned long)n, base)) {}

    const char* c_str() const { return value.c_str(); }
    size_t length() const { return value.length(); }
    int indexOf(char ch) const {
        size_t pos = value.find(ch);
        return pos == string::npos ? -1 : int(pos);
    }
    int indexOf(char ch, int fromIndex) const {
        if (fromIndex < 0 || static_cast<size_t>(fromIndex) >= value.size()) {
            return -1;
        }
        size_t pos = value.find(ch, static_cast<size_t>(fromIndex));
        return pos == string::npos ? -1 : int(pos);
    }
    int indexOf(const String& str) const {
        size_t pos = value.find(str.value);
        return pos == string::npos ? -1 : int(pos);
    }
    int indexOf(const char* str) const {
        return indexOf(String(str));
    }
    int lastIndexOf(char ch) const {
        size_t pos = value.find_last_of(ch);
        return pos == string::npos ? -1 : int(pos);
    }
    bool startsWith(const String& prefix) const {
        return value.rfind(prefix.value, 0) == 0;
    }
    bool endsWith(const String& suffix) const {
        if (suffix.value.size() > value.size()) {
            return false;
        }
        return value.compare(value.size() - suffix.value.size(), suffix.value.size(), suffix.value) == 0;
    }
    String substring(int from, int to) const {
        int len = int(value.length());
        if (from < 0) from = 0;
        if (from > len) from = len;
        if (to < 0 || to > len) to = len;
        if (to < from) to = from;
        return String(value.substr(from, to - from));
    }
    String substring(int from) const {
        int len = int(value.length());
        if (from < 0) from = 0;
        if (from > len) from = len;
        return String(value.substr(from));
    }
    void trim() {
        size_t start = 0;
        while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
            start++;
        }
        size_t end = value.size();
        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            end--;
        }
        value = value.substr(start, end - start);
    }
    void toLowerCase() {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    }
    int toInt() const {
        try {
            return std::stoi(value);
        } catch (...) {
            return 0;
        }
    }
    const char operator[](size_t index) const { return value[index]; }
    String& operator+=(const String& other) {
        value += other.value;
        return *this;
    }
    String& operator+=(const char* rhs) {
        value += rhs;
        return *this;
    }
    String& operator+=(char rhs) {
        value.push_back(rhs);
        return *this;
    }
    bool operator==(const char* rhs) const {
        return value == rhs;
    }
    bool operator==(const String& rhs) const {
        return value == rhs.value;
    }

    operator string() const { return value; }

private:
    string value;

    static string convertNumber(unsigned long n, int base) {
        if (base == 16) {
            std::ostringstream oss;
            oss << std::hex << n;
            return oss.str();
        }
        return std::to_string(n);
    }
};

inline String operator+(const String& a, const String& b) {
    return String(string(a.c_str()) + string(b.c_str()));
}

inline String operator+(const String& a, const char* b) {
    return String(string(a.c_str()) + string(b));
}

inline String operator+(const char* a, const String& b) {
    return String(string(a) + string(b.c_str()));
}

const int HEX = 16;

class IPAddress {
public:
    IPAddress() : octets{0,0,0,0} {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : octets{a,b,c,d} {}
    String toString() const {
        return String(std::to_string(octets[0]) + "." + std::to_string(octets[1]) + "." + std::to_string(octets[2]) + "." + std::to_string(octets[3]));
    }
private:
    uint8_t octets[4];
};

class Stream {
public:
    virtual void print(const String& msg) = 0;
    virtual void println(const String& msg) { print(msg + "\n"); }
    virtual ~Stream() {}
};

class NativeSerial : public Stream {
public:
    void begin(unsigned long) {}
    void print(const String& msg) override {
        lastMessage += msg.c_str();
    }
    void println(const String& msg) override {
        lastMessage += msg.c_str();
        lastMessage += '\n';
    }
    void println(const IPAddress& ip) {
        println(ip.toString());
    }
    void println() {
        lastMessage += '\n';
    }
    void print(char c) {
        lastMessage += c;
    }
    static String lastMessage;
};

extern NativeSerial Serial;

inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void configTime(long, int, const char*, const char* = nullptr) {}

inline bool getLocalTime(struct tm* info, uint32_t = 5000) {
    if (info) {
        *info = tm();
    }
    return false;
}

unsigned long millis();

const int WIFI_STA = 1;
const int WIFI_AP = 2;
const int WL_CONNECTED = 3;
const int WIFI_SCAN_FAILED = -1;
const int WIFI_SCAN_RUNNING = -2;
const int WIFI_AUTH_OPEN = 0;
const int WIFI_AUTH_WPA_PSK = 4;

class WiFiClass {
public:
    WiFiClass() : connected(true), scanResultCount(0) {}

    void mode(int) {}
    void begin(const char*, const char*) { connected = true; }
    int status() const { return connected ? WL_CONNECTED : 0; }
    int RSSI() const { return -50; }
    int RSSI(int) const { return -50; }
    int channel() const { return 1; }
    bool softAP(const char*, const char*) { return true; }
    IPAddress softAPIP() const { return IPAddress(192,168,4,1); }
    void softAPdisconnect(bool) {}
    void disconnect(bool) { connected = false; }
    void scanNetworks(bool) { scanResultCount = 0; }
    int scanComplete() { return scanResultCount; }
    String SSID() const { return "MockSSID"; }
    String SSID(int) const { return "MockSSID"; }
    IPAddress localIP() const { return IPAddress(192,168,1,100); }
    IPAddress subnetMask() const { return IPAddress(255,255,255,0); }
    IPAddress gatewayIP() const { return IPAddress(192,168,1,1); }
    IPAddress dnsIP() const { return IPAddress(8,8,8,8); }
    uint8_t encryptionType(int) const { return WIFI_AUTH_OPEN; }

private:
    bool connected;
    int scanResultCount;
};

extern WiFiClass WiFi;

using THandlerFunction = std::function<void()>;

enum HTTPMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
};

class WebServer {
public:
    explicit WebServer(int) {}
    void on(const char*, HTTPMethod, THandlerFunction) {}
    void begin() {}
    void handleClient() {}
    void stop() {}
    void send(int, const char*, const String&) {}
    bool hasArg(const char*) const { return false; }
    String arg(const char*) const { return ""; }
};

class BluetoothSerial : public Stream {
public:
    BluetoothSerial() : connectedFlag(false) {}
    bool begin(const char*, bool) {
        connectedFlag = false;
        return true;
    }
    bool connected() const {
        return connectedFlag;
    }
    size_t available() const {
        return 0;
    }
    char read() {
        return 0;
    }
    void print(const String& msg) override {
        (void)msg;
    }
    void print(char c) {
        (void)c;
    }
    void println(const String& msg) {
        (void)msg;
    }

private:
    bool connectedFlag;
};

class Preferences {
public:
    bool begin(const char* ns, bool) {
        namespaceName = ns ? ns : "";
        started = true;
        return true;
    }
    void end() {
        started = false;
    }
    String getString(const char* key, const String& defaultValue) {
        if (!started) {
            return defaultValue;
        }
        auto it = storage.find(key);
        return it != storage.end() ? String(it->second) : defaultValue;
    }
    void putString(const char* key, const String& value) {
        if (!started) {
            return;
        }
        storage[key] = value.c_str();
    }
    uint8_t getUChar(const char* key, uint8_t defaultValue) {
        if (!started) {
            return defaultValue;
        }
        auto it = storage.find(key);
        if (it != storage.end()) {
            try {
                return static_cast<uint8_t>(std::stoi(it->second));
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    void putUChar(const char* key, uint8_t value) {
        if (!started) {
            return;
        }
        storage[key] = std::to_string(value);
    }
    void remove(const char* key) {
        if (!started) {
            return;
        }
        storage.erase(key);
    }
    void clear() {
        if (!started) {
            return;
        }
        storage.clear();
    }

private:
    bool started = false;
    string namespaceName;
    std::map<string, string> storage;
};

class ESPClass {
public:
    uint64_t getEfuseMac() const {
        return 0;
    }
    void restart() const {}
};

extern ESPClass ESP;

#endif // ARDUINO

#endif // ARDUINO_HOST_H
