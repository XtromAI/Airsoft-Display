// SH1107 driver implementation
#include "sh1107_driver.h"
#include <cstdint>
#include <cstring>
#include <cstdlib>  // for abs()
#include "bitmap_font.h"
#include "font8x8.h"

SH1107_Display::SH1107_Display(spi_inst_t* spi_inst, uint8_t cs, uint8_t dc, uint8_t reset, uint8_t w, uint8_t h)
    : spi(spi_inst), cs_pin(cs), dc_pin(dc), reset_pin(reset), width(w), height(h) {
    
    // Allocate buffer (1 bit per pixel, organized in pages)
    buffer = new uint8_t[(width * height) / 8];
    memset(buffer, 0, (width * height) / 8);
}

SH1107_Display::~SH1107_Display() {
    delete[] buffer;
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
// Draw a character from a BitmapFont at (x, y)
// ...existing code...

void SH1107_Display::drawBitmapChar(uint8_t x, uint8_t y, char c, const BitmapFont& font, bool color) {
    int glyph_index = c - font.first_char;
    if (glyph_index < 0 || glyph_index >= font.glyph_count) return;
    // Each glyph is font.width columns, each column is font.height bits
    const unsigned char* glyph = font.data + glyph_index * font.width;
    for (int col = 0; col < font.width; col++) {
        // Read columns in reverse order for horizontal flip
        unsigned char colData = glyph[font.width - 1 - col];
        for (int row = 0; row < font.height; row++) {
            if (colData & (1 << row)) {
                setPixel(x + row, y + col, color);
            }
        }
    }
}
// ...existing code...

// ...existing code...


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
    // Initialize SPI - SH1107 typically works with CPOL=1, CPHA=1 (Mode 3)
    spi_init(spi, 10000000); // 10MHz (Python uses 10_000_000)
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
    
    // SH1107 initialization sequence (following Python reference)
    spi_write_command(SH1107_DISPLAYOFF);
    spi_write_command(SH1107_SETMULTIPLEX);
    spi_write_command(0x7F); // 128-1 for 128x128 display
    spi_write_command(0x20); // Set Memory addressing mode (0x20 = horizontal, 0x21 = vertical)
    spi_write_command(0x00); // Horizontal addressing for SH1107
    spi_write_command(SH1107_PAGEADDR | 0x00); // Set page address to zero
    spi_write_command(SH1107_DCDC);
    spi_write_command(0x81); // Enable charge pump (matches Python 0xad81)
    spi_write_command(SH1107_SETDISPLAYCLOCKDIV);
    spi_write_command(0x50); // Python uses 0x50 as POR value
    spi_write_command(SH1107_SETVCOMDETECT);
    spi_write_command(0x35); // Python POR value 0x35
    spi_write_command(SH1107_SETPRECHARGE);
    spi_write_command(0x22); // Python default POR value 0x22
    spi_write_command(SH1107_SETCONTRAST);
    spi_write_command(0x00); // Start with 0 contrast (Python calls contrast(0))
    spi_write_command(SH1107_DISPLAYNORMAL); // Python calls invert(0)
    spi_write_command(SH1107_SETDISPLAYOFFSET);
    spi_write_command(0x00); // Row offset for 128x128
    spi_write_command(SH1107_SEGREMAP | 0x00); // Python flip() logic
    spi_write_command(SH1107_COMSCANINC); // Python flip() logic
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
    // Only printable ASCII (space to tilde)
    if (c < 32 || c > 126) return;
    const uint8_t* charData = font8x8[c - 32];
    // Transpose and flip horizontally
    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            if (charData[col] & (1 << row)) {
                setPixel(x + (7 - row), y + col, color);
            }
        }
    }
}

void SH1107_Display::drawString(uint8_t x, uint8_t y, const char* str, bool color) {
    uint8_t cur_x = x;
    while (*str) {
        drawChar(cur_x, y, *str, color);
        cur_x += 8 + 1; // 8 pixels + 1 space
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

void SH1107_Display::drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, bool color, bool filled) {
    if (!filled) {
        // Bresenham's circle algorithm (from framebuf2.py)
        int f = 1 - radius;
        int ddF_x = 1;
        int ddF_y = -2 * radius;
        int x = 0;
        int y = radius;
        
        setPixel(x0, y0 + radius, color);
        setPixel(x0, y0 - radius, color);
        setPixel(x0 + radius, y0, color);
        setPixel(x0 - radius, y0, color);
        
        while (x < y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;
            
            setPixel(x0 + x, y0 + y, color);
            setPixel(x0 - x, y0 + y, color);
            setPixel(x0 + x, y0 - y, color);
            setPixel(x0 - x, y0 - y, color);
            setPixel(x0 + y, y0 + x, color);
            setPixel(x0 - y, y0 + x, color);
            setPixel(x0 + y, y0 - x, color);
            setPixel(x0 - y, y0 - x, color);
        }
    } else {
        // Filled circle (from framebuf2.py)
        drawLine(x0, y0 - radius, x0, y0 + radius, color);
        int f = 1 - radius;
        int ddF_x = 1;
        int ddF_y = -2 * radius;
        int x = 0;
        int y = radius;
        
        while (x < y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;
            
            drawLine(x0 + x, y0 - y, x0 + x, y0 + y, color);
            drawLine(x0 + y, y0 - x, x0 + y, y0 + x, color);
            drawLine(x0 - x, y0 - y, x0 - x, y0 + y, color);
            drawLine(x0 - y, y0 - x, x0 - y, y0 + x, color);
        }
    }
}

void SH1107_Display::drawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool color, bool filled) {
    if (!filled) {
        drawLine(x0, y0, x1, y1, color);
        drawLine(x1, y1, x2, y2, color);
        drawLine(x2, y2, x0, y0, color);
    } else {
        // Filled triangle algorithm (from framebuf2.py)
        // Sort vertices by y coordinate
        if (y0 > y1) {
            uint8_t temp;
            temp = y0; y0 = y1; y1 = temp;
            temp = x0; x0 = x1; x1 = temp;
        }
        if (y1 > y2) {
            uint8_t temp;
            temp = y2; y2 = y1; y1 = temp;
            temp = x2; x2 = x1; x1 = temp;
        }
        if (y0 > y1) {
            uint8_t temp;
            temp = y0; y0 = y1; y1 = temp;
            temp = x0; x0 = x1; x1 = temp;
        }
        
        if (y0 == y2) {
            // Degenerate case
            uint8_t a = x0, b = x0;
            if (x1 < a) a = x1;
            else if (x1 > b) b = x1;
            if (x2 < a) a = x2;
            else if (x2 > b) b = x2;
            drawLine(a, y0, b, y0, color);
            return;
        }
        
        int dx01 = x1 - x0;
        int dy01 = y1 - y0;
        int dx02 = x2 - x0;
        int dy02 = y2 - y0;
        int dx12 = x2 - x1;
        int dy12 = y2 - y1;
        
        if (dy01 == 0) dy01 = 1;
        if (dy02 == 0) dy02 = 1;
        if (dy12 == 0) dy12 = 1;
        
        int sa = 0, sb = 0;
        int y = y0;
        int last = (y0 == y1) ? y1 - 1 : y1;
        
        while (y <= last) {
            int a = x0 + sa / dy01;
            int b = x0 + sb / dy02;
            sa += dx01;
            sb += dx02;
            if (a > b) {
                int temp = a; a = b; b = temp;
            }
            drawLine(a, y, b, y, color);
            y++;
        }
        
        sa = dx12 * (y - y1);
        sb = dx02 * (y - y0);
        while (y <= y2) {
            int a = x1 + sa / dy12;
            int b = x0 + sb / dy02;
            sa += dx12;
            sb += dx02;
            if (a > b) {
                int temp = a; a = b; b = temp;
            }
            drawLine(a, y, b, y, color);
            y++;
        }
    }
}

void SH1107_Display::setDisplayStartLine(uint8_t line) {
    // From Python reference: valid values are 0 to 127
    spi_write_command(SH1107_SETDISPLAYSTARTLINE);
    spi_write_command(line & 0x7F);
}

void SH1107_Display::flip(bool horizontal, bool vertical) {
    // Based on Python flip() function logic
    uint8_t remap = horizontal ? 0x01 : 0x00;
    uint8_t direction = vertical ? 0x08 : 0x00;
    
    spi_write_command(SH1107_SEGREMAP | remap);
    spi_write_command(SH1107_COMSCANINC | direction);
}
