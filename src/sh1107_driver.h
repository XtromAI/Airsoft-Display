#ifndef SH1107_DRIVER_H
#define SH1107_DRIVER_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// SH1107 Commands (matching Python reference)
#define SH1107_SETLOWCOLUMN     0x00
#define SH1107_SETHIGHCOLUMN    0x10
#define SH1107_MEMORYMODE       0x20  // Set Memory addressing mode
#define SH1107_SETSTARTLINE     0x40
#define SH1107_SETCONTRAST      0x81
#define SH1107_SEGREMAP         0xA0
#define SH1107_DISPLAYALLON     0xA4
#define SH1107_DISPLAYALLOFF    0xA5
#define SH1107_DISPLAYNORMAL    0xA6
#define SH1107_DISPLAYINVERT    0xA7
#define SH1107_SETMULTIPLEX     0xA8
#define SH1107_DCDC             0xAD
#define SH1107_DISPLAYOFF       0xAE
#define SH1107_DISPLAYON        0xAF
#define SH1107_PAGEADDR         0xB0
#define SH1107_COMSCANINC       0xC0
#define SH1107_COMSCANDEC       0xC8
#define SH1107_SETDISPLAYOFFSET 0xD3
#define SH1107_SETDISPLAYCLOCKDIV 0xD5
#define SH1107_SETPRECHARGE     0xD9
#define SH1107_SETCOMPINS       0xDA
#define SH1107_SETVCOMDETECT    0xDB
#define SH1107_SETDISPLAYSTARTLINE 0xDC  // Python reference uses this

class SH1107_Display {
private:
    spi_inst_t* spi;
    uint8_t cs_pin;
    uint8_t dc_pin;
    uint8_t reset_pin;
    uint8_t width;
    uint8_t height;
    uint8_t* buffer;
    
    void spi_write_command(uint8_t cmd);
    void spi_write_data(uint8_t data);
    void spi_write_data_buffer(uint8_t* data, size_t len);
    
public:
    SH1107_Display(spi_inst_t* spi_inst, uint8_t cs, uint8_t dc, uint8_t reset, uint8_t w = 128, uint8_t h = 128);
    ~SH1107_Display();
    
    bool begin();
    void display();
    void clearDisplay();
    void setPixel(uint8_t x, uint8_t y, bool color);
    void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool color);
    void drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color);
    void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color);
    void drawChar(uint8_t x, uint8_t y, char c, bool color);
    void drawString(uint8_t x, uint8_t y, const char* str, bool color);
    void setContrast(uint8_t contrast);
    void invertDisplay(bool invert);
    void displayOn(bool on);
    
    // Additional functions from Python reference
    void drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, bool color, bool filled = false);
    void drawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool color, bool filled = false);
    void setDisplayStartLine(uint8_t line);
    void flip(bool horizontal = false, bool vertical = false);
};

#endif // SH1107_DRIVER_H
