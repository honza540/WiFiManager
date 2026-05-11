#ifndef ARDUINO
#include "Arduino.h"

NativeSerial Serial;
WiFiClass WiFi;
ESPClass ESP;

unsigned long millis() {
    using namespace std::chrono;
    static const steady_clock::time_point start = steady_clock::now();
    return static_cast<unsigned long>(duration_cast<milliseconds>(steady_clock::now() - start).count());
}

String NativeSerial::lastMessage = "";

#if !defined(UNIT_TEST) && !defined(PIO_UNIT_TESTING)
int main() {
    return 0;
}
#endif
#endif
