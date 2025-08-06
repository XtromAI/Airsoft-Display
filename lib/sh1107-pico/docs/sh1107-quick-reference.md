# SH1107 Driver Quick Reference

## Setup
```cpp
#include "sh1107_driver.h"

// Pin definitions (adjust as needed)
#define PIN_SPI_CS      13
#define PIN_SPI_DC      21  
#define PIN_SPI_RESET   20

// Create display object
SH1107_Display display(spi1, PIN_SPI_CS, PIN_SPI_DC, PIN_SPI_RESET, 128, 128);

// In main()
gpio_set_function(14, GPIO_FUNC_SPI);  // SCK
gpio_set_function(15, GPIO_FUNC_SPI);  // MOSI
display.begin();
```

## Basic Operations
```cpp
display.clearDisplay();              // Clear buffer
display.display();                   // Update screen
display.setPixel(x, y, true);       // Set pixel
display.drawString(x, y, "text", true); // Draw text
```

## Drawing Functions
```cpp
// Lines and shapes
display.drawLine(x0, y0, x1, y1, color);
display.drawRect(x, y, w, h, color);
display.fillRect(x, y, w, h, color);
display.drawCircle(x, y, radius, color, filled);
display.drawTriangle(x0, y0, x1, y1, x2, y2, color, filled);

// Display control
display.setContrast(0-255);
display.invertDisplay(true/false);
display.displayOn(true/false);
display.flip(horizontal, vertical);
```

## Common Patterns

### Status Display
```cpp
display.clearDisplay();
display.drawString(0, 0, "Status: OK", true);
display.drawString(0, 16, "Temp: 25C", true);
display.drawRect(0, 32, 128, 1, true);  // Separator
display.display();
```

### Progress Bar
```cpp
void drawProgress(int percent) {
    display.drawRect(10, 50, 108, 12, true);     // Border
    int fill = (percent * 104) / 100;
    display.fillRect(12, 52, fill, 8, true);     // Fill
    
    char text[10];
    sprintf(text, "%d%%", percent);
    display.drawString(50, 70, text, true);
}
```

### Scrolling Text
```cpp
void scrollText(const char* text, int position) {
    display.clearDisplay();
    display.drawString(position, 32, text, true);
    display.display();
}

// Usage in loop:
for (int pos = 128; pos > -strlen(text)*6; pos -= 2) {
    scrollText("Hello World!", pos);
    sleep_ms(50);
}
```

## Memory and Performance
- **Buffer**: 2KB (128×128÷8)
- **SPI Speed**: 10MHz
- **Update Time**: ~16ms full screen
- **Font**: 5×7 pixels, ASCII 32-126

## Pin Mapping (Default)
| Pico GPIO | SH1107 Pin | Function |
|-----------|------------|----------|
| 14        | SCK        | SPI Clock |
| 15        | MOSI       | SPI Data |
| 13        | CS         | Chip Select |
| 21        | DC         | Data/Command |
| 20        | RES        | Reset |
| 3V3       | VCC        | Power |
| GND       | GND        | Ground |

## Troubleshooting
- **No display**: Check wiring and power
- **Garbled**: Reduce SPI speed, check reset
- **Blank**: Call `display()` after drawing
- **Wrong colors**: Use `true`=white, `false`=black
