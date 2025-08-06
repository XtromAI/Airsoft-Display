# SH1107 Custom Display Driver Documentation

## Overview

This is a custom C++ driver for the SH1107 OLED display controller, specifically designed for the Raspberry Pi Pico using the Pico SDK. The driver provides direct SPI communication with hardware-accelerated graphics functions and is optimized for 128x128 pixel displays.

## Features

- **Direct SPI Communication**: Optimized for Raspberry Pi Pico hardware SPI
- **Built-in Graphics**: Text, lines, rectangles, circles, and triangles
- **High Performance**: 10MHz SPI communication with efficient buffer management
- **Hardware Integration**: Seamless integration with Pico SDK GPIO and SPI functions
- **Memory Efficient**: Minimal RAM usage with optimized display buffer
- **Python-Compatible**: Initialization sequence based on proven MicroPython implementation

## Hardware Requirements

### Supported Displays
- SH1107 128x128 OLED displays (tested with HiLetgo B0CFF435XZ)
- SPI interface (7-pin configuration)
- 3.3V operation

### Pin Configuration
```cpp
#define PIN_SPI_SCK     14  // SPI Clock (GPIO 14)
#define PIN_SPI_MOSI    15  // SPI Data Out (GPIO 15)
#define PIN_SPI_CS      13  // Chip Select (GPIO 13)
#define PIN_SPI_DC      21  // Data/Command (GPIO 21)
#define PIN_SPI_RESET   20  // Reset (GPIO 20)
```

### Wiring Diagram
```
Pico GPIO  →  SH1107 Display
---------     ---------------
3V3       →  VCC
GND       →  GND
GPIO 15   →  MOSI/SDA
GPIO 14   →  SCK/SCL
GPIO 13   →  CS
GPIO 21   →  DC
GPIO 20   →  RES/RESET
```

## Installation

### 1. Add Driver Files
Copy the following files to your project's `src/` directory:
- `sh1107_driver.h`
- `sh1107_driver.cpp`

### 2. Update CMakeLists.txt
Add the driver to your CMakeLists.txt:
```cmake
# Add executable
add_executable(your_project
    src/main.cpp
    src/sh1107_driver.cpp
)

# Link required libraries
target_link_libraries(your_project
    pico_stdlib
    hardware_spi
    hardware_gpio
)
```

### 3. Include in Your Code
```cpp
#include "sh1107_driver.h"
```

## Quick Start

### Basic Setup
```cpp
#include "sh1107_driver.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// Pin definitions
#define PIN_SPI_CS      13
#define PIN_SPI_DC      21
#define PIN_SPI_RESET   20

// Create display object
SH1107_Display display(spi1, PIN_SPI_CS, PIN_SPI_DC, PIN_SPI_RESET, 128, 128);

int main() {
    stdio_init_all();
    
    // Configure SPI pins
    gpio_set_function(14, GPIO_FUNC_SPI);  // SCK
    gpio_set_function(15, GPIO_FUNC_SPI);  // MOSI
    
    // Initialize display
    if (!display.begin()) {
        printf("Display initialization failed!\n");
        return -1;
    }
    
    // Draw something
    display.clearDisplay();
    display.drawString(0, 0, "Hello World!", true);
    display.display();
    
    return 0;
}
```

## API Reference

### Constructor
```cpp
SH1107_Display(spi_inst_t* spi_inst, uint8_t cs, uint8_t dc, uint8_t reset, uint8_t w = 128, uint8_t h = 128)
```
- `spi_inst`: SPI instance (spi0 or spi1)
- `cs`: Chip Select pin
- `dc`: Data/Command pin
- `reset`: Reset pin
- `w`: Display width (default: 128)
- `h`: Display height (default: 128)

### Core Functions

#### `bool begin()`
Initializes the display with proper SH1107 command sequence.
- **Returns**: `true` if successful, `false` if failed
- **Note**: Must be called before any other display operations

#### `void display()`
Updates the physical display with the current buffer contents.
- **Note**: Call this after drawing operations to make them visible

#### `void clearDisplay()`
Clears the display buffer (sets all pixels to off).
- **Note**: Call `display()` afterward to update the screen

### Drawing Functions

#### Text Rendering
```cpp
void drawChar(uint8_t x, uint8_t y, char c, bool color)
void drawString(uint8_t x, uint8_t y, const char* str, bool color)
```
- `x, y`: Position coordinates
- `c`: Character to draw
- `str`: Null-terminated string
- `color`: `true` for white/on, `false` for black/off

#### Pixel Operations
```cpp
void setPixel(uint8_t x, uint8_t y, bool color)
```
- Sets individual pixel at coordinates (x, y)

#### Line Drawing
```cpp
void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool color)
```
- Draws line from (x0, y0) to (x1, y1) using Bresenham's algorithm

#### Rectangle Functions
```cpp
void drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color)
void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color)
```
- `x, y`: Top-left corner
- `w, h`: Width and height
- `drawRect`: Outline only
- `fillRect`: Filled rectangle

#### Circle Functions
```cpp
void drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, bool color, bool filled = false)
```
- `x0, y0`: Center coordinates
- `radius`: Circle radius
- `filled`: `true` for filled circle, `false` for outline

#### Triangle Functions
```cpp
void drawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool color, bool filled = false)
```
- `(x0,y0), (x1,y1), (x2,y2)`: Triangle vertices
- `filled`: `true` for filled triangle, `false` for outline

### Display Control

#### `void setContrast(uint8_t contrast)`
Sets display contrast (0-255).
- `contrast`: 0 = minimum, 255 = maximum

#### `void invertDisplay(bool invert)`
Inverts all pixels on the display.
- `invert`: `true` to invert, `false` for normal

#### `void displayOn(bool on)`
Turns display on/off.
- `on`: `true` to turn on, `false` to turn off

#### `void flip(bool horizontal, bool vertical)`
Flips display orientation.
- `horizontal`: Mirror horizontally
- `vertical`: Mirror vertically

#### `void setDisplayStartLine(uint8_t line)`
Sets display start line for scrolling (0-127).

## Font Information

The driver includes a built-in 5x7 pixel font supporting:
- ASCII characters 32-126 (printable characters)
- Character spacing: 6 pixels wide (5 + 1 space)
- Character height: 8 pixels (7 + 1 space)

## Performance Characteristics

- **SPI Speed**: 10MHz (optimized for SH1107)
- **Buffer Size**: 2KB (128×128÷8 pixels)
- **Display Update**: ~16ms for full screen refresh
- **Memory Usage**: <4KB total (including driver code)

## Example Applications

### Simple Text Display
```cpp
display.clearDisplay();
display.drawString(0, 0, "Temperature:", true);
display.drawString(0, 16, "25.3°C", true);
display.display();
```

### Horizontal Scrolling Text
```cpp
const char* text = "Scrolling message! ";
int textLen = strlen(text) * 6;

for (int pos = 128; pos > -textLen; pos -= 2) {
    display.clearDisplay();
    display.drawString(pos, 32, text, true);
    display.display();
    sleep_ms(50);
}
```

### Status Indicators
```cpp
// Draw battery level
display.drawRect(100, 0, 26, 12, true);       // Battery outline
display.fillRect(126, 3, 2, 6, true);         // Battery tip
display.fillRect(102, 2, 20, 8, true);        // Battery fill

// Draw signal strength
for (int i = 0; i < 4; i++) {
    int height = (i + 1) * 3;
    display.fillRect(110 + i * 4, 12 - height, 2, height, true);
}
```

### Game Interface Elements
```cpp
// Crosshair
display.drawLine(59, 64, 69, 64, true);       // Horizontal
display.drawLine(64, 59, 64, 69, true);       // Vertical
display.drawCircle(64, 64, 20, true, false);  // Outer ring

// Ammo counter
display.drawString(0, 110, "AMMO: 30/90", true);

// Health bar
display.drawRect(90, 110, 36, 8, true);
display.fillRect(92, 112, 28, 4, true);  // 80% health
```

## Troubleshooting

### Display Not Working
1. **Check Wiring**: Verify all connections match the pin configuration
2. **SPI Configuration**: Ensure SPI pins are set to GPIO_FUNC_SPI
3. **Power Supply**: Confirm 3.3V supply is stable
4. **Initialization**: Check that `begin()` returns `true`

### Garbled Display
1. **SPI Speed**: Try reducing SPI speed in `begin()` function
2. **Timing**: Add delays between commands if needed
3. **Reset Sequence**: Ensure reset pin is properly connected

### Missing Pixels
1. **Buffer Update**: Make sure to call `display()` after drawing
2. **Coordinate Bounds**: Check that coordinates are within 0-127 range
3. **Color Parameter**: Verify `true`/`false` for on/off pixels

### Performance Issues
1. **Minimize Updates**: Only call `display()` when necessary
2. **Partial Updates**: Consider implementing dirty rectangle tracking
3. **SPI Optimization**: Ensure DMA is available for large transfers

## Technical Details

### Memory Layout
The display buffer uses vertical byte addressing:
- Each byte represents 8 vertical pixels
- Buffer size: 128 × (128÷8) = 2048 bytes
- Bit 0 = top pixel, Bit 7 = bottom pixel in each byte

### SH1107 Commands Used
```cpp
#define SH1107_SETCONTRAST      0x81
#define SH1107_DISPLAYON        0xAF
#define SH1107_DISPLAYOFF       0xAE
#define SH1107_SETMULTIPLEX     0xA8
#define SH1107_PAGEADDR         0xB0
// ... and more
```

### Initialization Sequence
Based on proven MicroPython implementation:
1. Reset pulse (20ms low, 20ms high)
2. Display off
3. Set multiplex ratio (128-1)
4. Configure memory addressing
5. Set charge pump and timing
6. Configure scan direction
7. Display on

## Compatibility

### Tested Hardware
- ✅ Raspberry Pi Pico (RP2040)
- ✅ HiLetgo SH1107 128x128 OLED (7-pin SPI)
- ✅ Pico SDK 2.1.1
- ✅ CMake build system

### Future Compatibility
- Should work with other RP2040-based boards
- Compatible with other SH1107 displays
- Easily portable to other SPI-capable microcontrollers

## Version History

- **v1.0**: Initial release with basic graphics functions
- **v1.1**: Added Python-compatible initialization sequence
- **v1.2**: Added circle and triangle drawing functions
- **v1.3**: Performance optimizations and enhanced documentation

## License

This driver is released under the MIT License. See project LICENSE file for details.

## Contributing

Contributions are welcome! Please:
1. Test on your hardware configuration
2. Document any changes
3. Follow existing code style
4. Update this documentation as needed

## References

- [SH1107 Datasheet](../hardware/SH1107Datasheet.pdf)
- [Raspberry Pi Pico Documentation](../hardware/pico-datasheet.pdf)
- [MicroPython SH1107 Driver](../python-reference/sh1107.py) (reference implementation)
- [Extended Framebuffer](../python-reference/framebuf2.py) (graphics algorithms)
