#ifndef ARDUINO_HOST_ESP_H
#define ARDUINO_HOST_ESP_H

#include <cstdint>

// ============================================================================
// ESP STUB
// ============================================================================

class ESPClass {
public:
    uint64_t getEfuseMac() const {
        return 0;
    }
    void restart() const {}
};

extern ESPClass ESP;

#endif // ARDUINO_HOST_ESP_H
