#ifndef ADAFRUIT_I2CDEVICE_H
#define ADAFRUIT_I2CDEVICE_H

// Stub for Adafruit_I2CDevice - not needed for SPI-only displays
#include "../arduino_compat.h"

class TwoWire {
public:
    void begin() {}
    void beginTransmission(uint8_t) {}
    uint8_t endTransmission() { return 0; }
    uint8_t write(uint8_t) { return 1; }
    uint8_t requestFrom(uint8_t, uint8_t) { return 0; }
    uint8_t read() { return 0; }
};

class Adafruit_I2CDevice {
public:
    Adafruit_I2CDevice(uint8_t addr, TwoWire *wire = nullptr) {}
    bool begin() { return true; }
    bool detected() { return false; }
    bool write(uint8_t *buffer, size_t len) { return false; }
    bool read(uint8_t *buffer, size_t len) { return false; }
};

#endif // ADAFRUIT_I2CDEVICE_H
