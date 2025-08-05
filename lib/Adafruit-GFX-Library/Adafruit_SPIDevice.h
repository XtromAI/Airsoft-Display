#ifndef ADAFRUIT_SPIDEVICE_H
#define ADAFRUIT_SPIDEVICE_H

// Stub for Adafruit_SPIDevice - we'll handle SPI directly in display driver
#include "../arduino_compat.h"

class SPIClass {
public:
    void begin() {}
    void end() {}
    void beginTransaction(uint32_t) {}
    void endTransaction() {}
    uint8_t transfer(uint8_t data) { return 0; }
    void transfer(void *buf, size_t count) {}
};

class Adafruit_SPIDevice {
public:
    Adafruit_SPIDevice(int8_t cs, uint32_t freq = 1000000, uint8_t dataOrder = 0, uint8_t dataMode = 0, SPIClass *spi = nullptr) {}
    bool begin() { return true; }
    bool write(uint8_t *buffer, size_t len) { return false; }
    bool read(uint8_t *buffer, size_t len) { return false; }
    bool write_then_read(uint8_t *write_buffer, size_t write_len, uint8_t *read_buffer, size_t read_len) { return false; }
};

#endif // ADAFRUIT_SPIDEVICE_H
