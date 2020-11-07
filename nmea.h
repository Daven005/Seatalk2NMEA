#ifndef NMEA_H
#define NMEA_H
#include "Arduino.h"
#include "defs.h"
#include <Preferences.h>
Preferences prefs;
#define NameSpace "nmea"

#define RX_FLAG 0x37
#define TX_FLAG 0x81

class Nmea: public Stream {
    HardwareSerial serial;
  public:
    Nmea(): serial(2) {}
    typedef enum { RX, TX } Mode_t;
    Mode_t mode;
    void begin() {
      prefs.begin(NameSpace);
      serial.begin(4800, SERIAL_8N1, NMEA_IN, NMEA_OUT);
      switch (prefs.getUChar("flag")) {
        case TX_FLAG:
          setMode(TX);
          break;
        case RX_FLAG:
        default: // Not yet set
          Serial.println("Bad flag");
          setMode(RX);
      }
    }
    void setMode(Mode_t m) {
      digitalWrite(NMEA_TX, m == TX ? 1 : 0);
      if (m != mode) {
        prefs.putUChar("flag", (m == TX) ? TX_FLAG : RX_FLAG);
        Serial.println(prefs.getUChar("flag"));
        mode = m;
      }
      Serial.printf("NMEA %cx Mode", mode ? 'T' : 'R');
    }
    bool txMode() {
      return mode == TX;
    }
    int available() {
      return (mode == RX) && serial.available();
    }
    int read() {
      if (available()) return serial.read();
      return -1;
    }
    int peek() {
      return -1;
    }
    void flush() {}
    size_t write(const uint8_t *buffer, size_t size) {
      if (mode == TX) return serial.write(buffer, size);
      return 0;
    }
    size_t write(uint8_t c) {
      if (mode == TX) return serial.write(c);
      return 0;
    }
};

#endif
