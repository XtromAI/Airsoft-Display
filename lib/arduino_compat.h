#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

// Arduino compatibility layer for Pico SDK
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/time.h"
#include <algorithm>  // for std::min, std::max

// Arduino-style types
typedef uint8_t byte;
typedef bool boolean;

// Arduino constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// Arduino-style functions
#define delay(ms) sleep_ms(ms)
#define delayMicroseconds(us) sleep_us(us)
#define millis() to_ms_since_boot(get_absolute_time())
#define micros() to_us_since_boot(get_absolute_time())

// GPIO functions
#define pinMode(pin, mode) do { \
    gpio_init(pin); \
    if (mode == OUTPUT) gpio_set_dir(pin, GPIO_OUT); \
    else if (mode == INPUT) gpio_set_dir(pin, GPIO_IN); \
    else if (mode == INPUT_PULLUP) { gpio_set_dir(pin, GPIO_IN); gpio_pull_up(pin); } \
} while(0)

#define digitalWrite(pin, val) gpio_put(pin, val)
#define digitalRead(pin) gpio_get(pin)

// Use std::min/max instead of defining our own
using std::min;
using std::max;

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

// Flash string helper (Arduino-specific)
class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))
#define PSTR(s) (__extension__({static const char __c[] __attribute__((__progmem__)) = (s); &__c[0];}))

// Minimal String class
class String {
private:
    char* buffer;
    size_t len;
public:
    String() : buffer(nullptr), len(0) {}
    String(const char* str) {
        if (str) {
            len = strlen(str);
            buffer = (char*)malloc(len + 1);
            strcpy(buffer, str);
        } else {
            buffer = nullptr;
            len = 0;
        }
    }
    String(int val) {
        char temp[16];
        snprintf(temp, sizeof(temp), "%d", val);
        len = strlen(temp);
        buffer = (char*)malloc(len + 1);
        strcpy(buffer, temp);
    }
    ~String() { if (buffer) free(buffer); }
    
    const char* c_str() const { return buffer ? buffer : ""; }
    operator const char*() const { return c_str(); }
    size_t length() const { return len; }
};

// Print class for compatibility
class Print {
public:
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) {
        size_t n = 0;
        while (size--) {
            if (write(*buffer++)) n++;
            else break;
        }
        return n;
    }
    
    size_t print(const char* str) {
        return write((const uint8_t*)str, strlen(str));
    }
    
    size_t print(int val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", val);
        return print(buf);
    }
    
    size_t println(const char* str) {
        size_t n = print(str);
        n += print("\r\n");
        return n;
    }
    
    size_t println() {
        return print("\r\n");
    }
};

// SPI stub - we'll handle SPI in the display driver
extern spi_inst_t *spi1;

#endif // ARDUINO_COMPAT_H
