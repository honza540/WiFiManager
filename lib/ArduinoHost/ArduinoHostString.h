#ifndef ARDUINO_HOST_STRING_H
#define ARDUINO_HOST_STRING_H

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <string>

using std::string;
using boolean = bool;

// ============================================================================
// ARDUINO STRING STUB
// ============================================================================

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

#endif // ARDUINO_HOST_STRING_H
