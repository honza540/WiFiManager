#ifndef ARDUINO_HOST_H
#define ARDUINO_HOST_H

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

// Minimal host-side Arduino surface used by the native unit tests.
#include "ArduinoHostString.h"
#include "ArduinoHostIO.h"
#include "ArduinoHostWiFi.h"
#include "ArduinoHostWebServer.h"
#include "ArduinoHostBluetoothSerial.h"
#include "ArduinoHostPreferences.h"
#include "ArduinoHostESP.h"

#endif // ARDUINO

#endif // ARDUINO_HOST_H
