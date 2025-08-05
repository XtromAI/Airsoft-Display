#include "sh1107_driver.h"
#include <cstring>
#include <cstdlib>  // for abs()

// Simple 5x7 font (96 characters starting from space)
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x00, 0x7F, 0x41, 0x41}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
    {0x41, 0x41, 0x7F, 0x00, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x00, 0x7F, 0x10, 0x28, 0x44}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x08, 0x04, 0x08, 0x10, 0x08}, // ~
};

SH1107_Display::SH1107_Display(spi_inst_t* spi_inst, uint8_t cs, uint8_t dc, uint8_t reset, uint8_t w, uint8_t h)
    : spi(spi_inst), cs_pin(cs), dc_pin(dc), reset_pin(reset), width(w), height(h) {
    
    // Allocate buffer (1 bit per pixel, organized in pages)
    buffer = new uint8_t[(width * height) / 8];
    memset(buffer, 0, (width * height) / 8);
}

SH1107_Display::~SH1107_Display() {
    delete[] buffer;
}

void SH1107_Display::spi_write_command(uint8_t cmd) {
    gpio_put(dc_pin, 0); // Command mode
    gpio_put(cs_pin, 0); // Select device
    spi_write_blocking(spi, &cmd, 1);
    gpio_put(cs_pin, 1); // Deselect device
}

void SH1107_Display::spi_write_data(uint8_t data) {
    gpio_put(dc_pin, 1); // Data mode
    gpio_put(cs_pin, 0); // Select device
    spi_write_blocking(spi, &data, 1);
    gpio_put(cs_pin, 1); // Deselect device
}

void SH1107_Display::spi_write_data_buffer(uint8_t* data, size_t len) {
    gpio_put(dc_pin, 1); // Data mode
    gpio_put(cs_pin, 0); // Select device
    spi_write_blocking(spi, data, len);
    gpio_put(cs_pin, 1); // Deselect device
}

bool SH1107_Display::begin() {
    // Initialize SPI
    spi_init(spi, 1000000); // 1MHz
    spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    
    // Initialize GPIO pins
    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 1);
    
    gpio_init(dc_pin);
    gpio_set_dir(dc_pin, GPIO_OUT);
    gpio_put(dc_pin, 0);
    
    gpio_init(reset_pin);
    gpio_set_dir(reset_pin, GPIO_OUT);
    
    // Reset sequence
    gpio_put(reset_pin, 1);
    sleep_ms(1);
    gpio_put(reset_pin, 0);
    sleep_ms(20);
    gpio_put(reset_pin, 1);
    sleep_ms(20);
    
    // SH1107 initialization sequence
    spi_write_command(SH1107_DISPLAYOFF);
    spi_write_command(SH1107_SETDISPLAYCLOCKDIV);
    spi_write_command(0x51);
    spi_write_command(SH1107_SETMULTIPLEX);
    spi_write_command(0x7F); // 128-1
    spi_write_command(SH1107_SETDISPLAYOFFSET);
    spi_write_command(0x00);
    spi_write_command(SH1107_SETSTARTLINE | 0x00);
    spi_write_command(SH1107_DCDC);
    spi_write_command(0x8A);
    spi_write_command(SH1107_SEGREMAP | 0x01);
    spi_write_command(SH1107_COMSCANDEC);
    spi_write_command(SH1107_SETCOMPINS);
    spi_write_command(0x12);
    spi_write_command(SH1107_SETCONTRAST);
    spi_write_command(0x80);
    spi_write_command(SH1107_SETPRECHARGE);
    spi_write_command(0x22);
    spi_write_command(SH1107_SETVCOMDETECT);
    spi_write_command(0x35);
    spi_write_command(SH1107_DISPLAYALLON);
    spi_write_command(SH1107_DISPLAYNORMAL);
    spi_write_command(SH1107_DISPLAYON);
    
    clearDisplay();
    display();
    
    return true;
}

void SH1107_Display::display() {
    // SH1107 uses page addressing mode
    for (uint8_t page = 0; page < (height / 8); page++) {
        spi_write_command(SH1107_PAGEADDR | page);
        spi_write_command(SH1107_SETLOWCOLUMN | 0x00);
        spi_write_command(SH1107_SETHIGHCOLUMN | 0x00);
        
        // Send one page worth of data
        spi_write_data_buffer(&buffer[page * width], width);
    }
}

void SH1107_Display::clearDisplay() {
    memset(buffer, 0, (width * height) / 8);
}

void SH1107_Display::setPixel(uint8_t x, uint8_t y, bool color) {
    if (x >= width || y >= height) return;
    
    uint16_t index = x + (y / 8) * width;
    uint8_t bit = y % 8;
    
    if (color) {
        buffer[index] |= (1 << bit);
    } else {
        buffer[index] &= ~(1 << bit);
    }
}

void SH1107_Display::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        setPixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void SH1107_Display::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color) {
    drawLine(x, y, x + w - 1, y, color);         // Top
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color); // Bottom
    drawLine(x, y, x, y + h - 1, color);         // Left
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
}

void SH1107_Display::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color) {
    for (uint8_t i = x; i < x + w; i++) {
        for (uint8_t j = y; j < y + h; j++) {
            setPixel(i, j, color);
        }
    }
}

void SH1107_Display::drawChar(uint8_t x, uint8_t y, char c, bool color) {
    if (c < 32 || c > 126) return; // Only printable ASCII
    
    const uint8_t* charData = font5x7[c - 32];
    
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = charData[col];
        for (uint8_t row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                setPixel(x + col, y + row, color);
            }
        }
    }
}

void SH1107_Display::drawString(uint8_t x, uint8_t y, const char* str, bool color) {
    uint8_t cur_x = x;
    while (*str) {
        drawChar(cur_x, y, *str, color);
        cur_x += 6; // 5 pixels + 1 space
        str++;
    }
}

void SH1107_Display::setContrast(uint8_t contrast) {
    spi_write_command(SH1107_SETCONTRAST);
    spi_write_command(contrast);
}

void SH1107_Display::invertDisplay(bool invert) {
    spi_write_command(invert ? SH1107_DISPLAYINVERT : SH1107_DISPLAYNORMAL);
}

void SH1107_Display::displayOn(bool on) {
    spi_write_command(on ? SH1107_DISPLAYON : SH1107_DISPLAYOFF);
}
