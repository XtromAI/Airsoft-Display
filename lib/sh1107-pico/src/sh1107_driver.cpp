
// SH1107 driver implementation
#include "sh1107_driver.h"
#include <cstdint>
#include <cstring>
#include <cstdlib>  // for abs()
#include "bitmap_font.h"
#include "font8x8.h"


// Constructor & Destructor

extern const BitmapFont font8x8;

SH1107_Display::SH1107_Display(spi_inst_t* spi_inst, uint8_t cs, uint8_t dc, uint8_t reset, uint8_t w, uint8_t h)
    : spi(spi_inst), cs_pin(cs), dc_pin(dc), reset_pin(reset), width(w), height(h), currentFont(&font8x8), charSpacing(0) {
    buffer = new uint8_t[(width * height) / 8];
    memset(buffer, 0, (width * height) / 8);
}

// Set the current font to be used for drawString
void SH1107_Display::setFont(const BitmapFont* font) {
    currentFont = font;
}

// Set the character spacing (in pixels) between characters when drawing strings
void SH1107_Display::setCharSpacing(uint8_t spacing) {
    charSpacing = spacing;
}

SH1107_Display::~SH1107_Display() {
    delete[] buffer;
}

// Public API
bool SH1107_Display::begin() {
    spi_init(spi, 10000000);
    spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 1);
    gpio_init(dc_pin);
    gpio_set_dir(dc_pin, GPIO_OUT);
    gpio_put(dc_pin, 0);
    gpio_init(reset_pin);
    gpio_set_dir(reset_pin, GPIO_OUT);
    gpio_put(reset_pin, 1);
    sleep_ms(1);
    gpio_put(reset_pin, 0);
    sleep_ms(20);
    gpio_put(reset_pin, 1);
    sleep_ms(20);
    spi_write_command(SH1107_DISPLAYOFF);
    spi_write_command(SH1107_SETMULTIPLEX);
    spi_write_command(0x7F);
    spi_write_command(0x20);
    spi_write_command(0x00);
    spi_write_command(SH1107_PAGEADDR | 0x00);
    spi_write_command(SH1107_DCDC);
    spi_write_command(0x81);
    spi_write_command(SH1107_SETDISPLAYCLOCKDIV);
    spi_write_command(0x50);
    spi_write_command(SH1107_SETVCOMDETECT);
    spi_write_command(0x35);
    spi_write_command(SH1107_SETPRECHARGE);
    spi_write_command(0x22);
    spi_write_command(SH1107_SETCONTRAST);
    spi_write_command(0x00);
    spi_write_command(SH1107_DISPLAYNORMAL);
    spi_write_command(SH1107_SETDISPLAYOFFSET);
    spi_write_command(0x00);
    spi_write_command(SH1107_SEGREMAP | 0x00);
    spi_write_command(SH1107_COMSCANINC);
    spi_write_command(SH1107_DISPLAYON);
    clearDisplay();
    display();
    return true;
}

void SH1107_Display::display() {
    for (uint8_t page = 0; page < (height / 8); page++) {
        spi_write_command(SH1107_PAGEADDR | page);
        spi_write_command(SH1107_SETLOWCOLUMN | 0x00);
        spi_write_command(SH1107_SETHIGHCOLUMN | 0x00);
        spi_write_data_buffer(&buffer[page * width], width);
    }
}

void SH1107_Display::clearDisplay() {
    memset(buffer, 0, (width * height) / 8);
}

void SH1107_Display::setPixel(uint8_t x, uint8_t y, bool color /* = true */) {
    if (x >= width || y >= height) return;
    uint16_t index = x + (y / 8) * width;
    uint8_t bit = y % 8;
    
    // Additional bounds check for buffer safety
    if (index >= (width * height) / 8) return;
    
    if (color) {
        buffer[index] |= (1 << bit);
    } else {
        buffer[index] &= ~(1 << bit);
    }
}

/**
 * @brief Draw a string of characters on the display using the current font.
 *
 * This function renders a null-terminated ASCII string at the specified (x, y) position
 * using the font currently set by setFont(). Each character is drawn using drawChar,
 * and the cursor advances by the font's width plus one pixel of spacing.
 *
 * @param x     X coordinate (in pixels) of the top-left corner of the first character.
 * @param y     Y coordinate (in pixels) of the top-left corner of the first character.
 * @param str   Null-terminated C string to draw.
 */
void SH1107_Display::drawString(uint8_t x, uint8_t y, const char* str) {
    // Calculate string length
    size_t len = strlen(str);
    if (len == 0 || !currentFont) return;

    // Calculate total width and height of the string in pixels
    uint8_t str_width = len * (currentFont->width + charSpacing);
    if (str_width > 0) str_width -= charSpacing; // No extra space after last char
    uint8_t str_height = currentFont->height;

    // Calculate top-left starting point to center the string at (x, y)
    int start_x = (int)x - (int)str_width / 2;
    int start_y = (int)y - (int)str_height / 2;

    uint8_t cur_x = (start_x < 0) ? 0 : (uint8_t)start_x;
    const char* p = str;
    while (*p) {
        drawChar(cur_x, (start_y < 0) ? 0 : (uint8_t)start_y, *p);
        cur_x += currentFont->width + charSpacing;
        p++;
    }
}

/**
 * @brief Draw a single character on the display using a bitmap font.
 * 
 * This function renders a single character from a bitmap font at the specified position.
 * The font data is interpreted as follows:
 * - For fonts with width <= 8: Each character consists of font.height bytes (one byte per row)
 * - For fonts with width > 8: Each character consists of font.height * ((width + 7) / 8) bytes
 * - Each byte represents part of a horizontal row of pixels within the character
 * - Within each byte, bit 0 (LSB) represents the leftmost pixel
 * - Within each byte, bit 7 (MSB) represents the rightmost pixel
 * 
 * @param x     X coordinate (in pixels) of the top-left corner of the character
 * @param y     Y coordinate (in pixels) of the top-left corner of the character  
 * @param c     ASCII character to draw
 * 
 * @note Characters outside the font's character range are silently ignored
 * @note Pixels are set using the setPixel() function with color=true (white/on)
 * @note Uses the font currently set by setFont()
 */
void SH1107_Display::drawChar(uint8_t x, uint8_t y, char c) {
    if (!currentFont) return; // Safety check
    int glyph_index = c - currentFont->first_char;
    if (glyph_index < 0 || glyph_index >= currentFont->glyph_count) return;
    
    // Calculate bytes per row based on font width
    int bytes_per_row = (currentFont->width + 7) / 8;
    int bytes_per_glyph = currentFont->height * bytes_per_row;
    
    const unsigned char* glyph = currentFont->data + glyph_index * bytes_per_glyph;
    
    for (int row = 0; row < currentFont->height; row++) {
        for (int byte_in_row = 0; byte_in_row < bytes_per_row; byte_in_row++) {
            unsigned char rowData = glyph[row * bytes_per_row + byte_in_row];
            int base_col = byte_in_row * 8;
            
            for (int bit = 0; bit < 8 && (base_col + bit) < currentFont->width; bit++) {
                if (rowData & (1 << bit)) { // LSB is leftmost pixel
                    int pixel_x = x + base_col + bit;
                    int pixel_y = y + row;
                    // Bounds check to prevent crashes
                    if (pixel_x < width && pixel_y < height) {
                        setPixel(pixel_x, pixel_y);
                    }
                }
            }
        }
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
    drawLine(x, y, x + w - 1, y, color);
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    drawLine(x, y, x, y + h - 1, color);
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void SH1107_Display::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color) {
    for (uint8_t i = x; i < x + w; i++) {
        for (uint8_t j = y; j < y + h; j++) {
            setPixel(i, j, color);
        }
    }
}

void SH1107_Display::drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, bool color, bool filled) {
    if (!filled) {
        // Classic midpoint circle algorithm, diameter = 2*radius (not 2*radius+1)
        int r = radius - 1;
        int x = r;
        int y = 0;
        int p = 1 - r;
        while (x >= y) {
            setPixel(x0 + x, y0 + y, color);
            setPixel(x0 - x, y0 + y, color);
            setPixel(x0 + x, y0 - y, color);
            setPixel(x0 - x, y0 - y, color);
            setPixel(x0 + y, y0 + x, color);
            setPixel(x0 - y, y0 + x, color);
            setPixel(x0 + y, y0 - x, color);
            setPixel(x0 - y, y0 - x, color);
            y++;
            if (p <= 0) {
                p = p + 2 * y + 1;
            } else {
                x--;
                p = p + 2 * y - 2 * x + 1;
            }
        }
    } else {
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
        if (y0 > y1) { uint8_t temp; temp = y0; y0 = y1; y1 = temp; temp = x0; x0 = x1; x1 = temp; }
        if (y1 > y2) { uint8_t temp; temp = y2; y2 = y1; y1 = temp; temp = x2; x2 = x1; x1 = temp; }
        if (y0 > y1) { uint8_t temp; temp = y0; y0 = y1; y1 = temp; temp = x0; x0 = x1; x1 = temp; }
        if (y0 == y2) {
            uint8_t a = x0, b = x0;
            if (x1 < a) a = x1; else if (x1 > b) b = x1;
            if (x2 < a) a = x2; else if (x2 > b) b = x2;
            drawLine(a, y0, b, y0, color);
            return;
        }
        int dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1;
        if (dy01 == 0) dy01 = 1;
        if (dy02 == 0) dy02 = 1;
        if (dy12 == 0) dy12 = 1;
        int sa = 0, sb = 0, y = y0, last = (y0 == y1) ? y1 - 1 : y1;
        while (y <= last) {
            int a = x0 + sa / dy01, b = x0 + sb / dy02;
            sa += dx01; sb += dx02;
            if (a > b) { int temp = a; a = b; b = temp; }
            drawLine(a, y, b, y, color);
            y++;
        }
        sa = dx12 * (y - y1); sb = dx02 * (y - y0);
        while (y <= y2) {
            int a = x1 + sa / dy12, b = x0 + sb / dy02;
            sa += dx12; sb += dx02;
            if (a > b) { int temp = a; a = b; b = temp; }
            drawLine(a, y, b, y, color);
            y++;
        }
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

void SH1107_Display::setDisplayStartLine(uint8_t line) {
    spi_write_command(SH1107_SETDISPLAYSTARTLINE);
    spi_write_command(line & 0x7F);
}

void SH1107_Display::flip(bool horizontal, bool vertical) {
    uint8_t remap = horizontal ? 0x01 : 0x00;
    uint8_t direction = vertical ? 0x08 : 0x00;
    spi_write_command(SH1107_SEGREMAP | remap);
    spi_write_command(SH1107_COMSCANINC | direction);
}

// Private SPI helpers
void SH1107_Display::spi_write_command(uint8_t cmd) {
    gpio_put(dc_pin, 0);
    gpio_put(cs_pin, 0);
    spi_write_blocking(spi, &cmd, 1);
    gpio_put(cs_pin, 1);
}

void SH1107_Display::spi_write_data(uint8_t data) {
    gpio_put(dc_pin, 1);
    gpio_put(cs_pin, 0);
    spi_write_blocking(spi, &data, 1);
    gpio_put(cs_pin, 1);
}

void SH1107_Display::spi_write_data_buffer(uint8_t* data, size_t len) {
    gpio_put(dc_pin, 1);
    gpio_put(cs_pin, 0);
    spi_write_blocking(spi, data, len);
    gpio_put(cs_pin, 1);
}
