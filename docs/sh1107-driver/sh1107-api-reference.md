# SH1107 Driver API Reference

## Class: SH1107_Display

### Constructor
```cpp
SH1107_Display(spi_inst_t* spi_inst, uint8_t cs, uint8_t dc, uint8_t reset, uint8_t w = 128, uint8_t h = 128)
```
**Parameters:**
- `spi_inst` - SPI instance pointer (spi0 or spi1)
- `cs` - Chip Select GPIO pin number
- `dc` - Data/Command GPIO pin number  
- `reset` - Reset GPIO pin number
- `w` - Display width in pixels (default: 128)
- `h` - Display height in pixels (default: 128)

**Example:**
```cpp
SH1107_Display display(spi1, 13, 21, 20, 128, 128);
```

---

## Core Functions

### `bool begin()`
Initialize the display hardware and configure SH1107.

**Returns:** `true` if initialization successful, `false` if failed

**Description:** Sets up SPI communication, performs hardware reset, and sends initialization command sequence. Must be called before any other display operations.

**Example:**
```cpp
if (!display.begin()) {
    printf("Display init failed!\n");
    return -1;
}
```

### `void display()`
Update the physical display with current buffer contents.

**Parameters:** None  
**Returns:** void

**Description:** Transfers the internal framebuffer to the SH1107 display memory. Call this after drawing operations to make them visible.

**Example:**
```cpp
display.drawString(0, 0, "Hello", true);
display.display();  // Make it visible
```

### `void clearDisplay()`
Clear the display buffer (set all pixels to off/black).

**Parameters:** None  
**Returns:** void

**Description:** Sets all pixels in the framebuffer to 0 (black/off). Does not update the physical display until `display()` is called.

**Example:**
```cpp
display.clearDisplay();
display.display();  // Show blank screen
```

---

## Drawing Functions

### `void setPixel(uint8_t x, uint8_t y, bool color)`
Set individual pixel state.

**Parameters:**
- `x` - X coordinate (0-127)
- `y` - Y coordinate (0-127)  
- `color` - `true` for white/on, `false` for black/off

**Example:**
```cpp
display.setPixel(64, 32, true);   // White pixel at center
display.setPixel(65, 32, false);  // Black pixel next to it
```

### `void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool color)`
Draw line between two points using Bresenham's algorithm.

**Parameters:**
- `x0, y0` - Start point coordinates
- `x1, y1` - End point coordinates
- `color` - Line color (`true`=white, `false`=black)

**Example:**
```cpp
display.drawLine(0, 0, 127, 127, true);    // Diagonal line
display.drawLine(10, 50, 100, 50, true);   // Horizontal line
```

### `void drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color)`
Draw rectangle outline.

**Parameters:**
- `x, y` - Top-left corner coordinates
- `w` - Width in pixels
- `h` - Height in pixels
- `color` - Border color

**Example:**
```cpp
display.drawRect(10, 10, 50, 30, true);  // 50×30 rectangle at (10,10)
```

### `void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color)`
Draw filled rectangle.

**Parameters:**
- `x, y` - Top-left corner coordinates
- `w` - Width in pixels
- `h` - Height in pixels
- `color` - Fill color

**Example:**
```cpp
display.fillRect(20, 20, 40, 20, true);  // Solid white rectangle
```

### `void drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, bool color, bool filled = false)`
Draw circle (outline or filled).

**Parameters:**
- `x0, y0` - Center coordinates
- `radius` - Circle radius in pixels
- `color` - Circle color
- `filled` - `true` for filled circle, `false` for outline only

**Example:**
```cpp
display.drawCircle(64, 64, 20, true, false); // Circle outline
display.drawCircle(30, 30, 10, true, true);  // Filled circle
```

### `void drawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool color, bool filled = false)`
Draw triangle (outline or filled).

**Parameters:**
- `x0, y0` - First vertex coordinates
- `x1, y1` - Second vertex coordinates
- `x2, y2` - Third vertex coordinates
- `color` - Triangle color
- `filled` - `true` for filled triangle, `false` for outline only

**Example:**
```cpp
// Triangle outline
display.drawTriangle(64, 20, 40, 60, 88, 60, true, false);
// Filled triangle  
display.drawTriangle(20, 80, 35, 100, 5, 100, true, true);
```

---

## Text Functions

### `void drawChar(uint8_t x, uint8_t y, char c, bool color)`
Draw single character.

**Parameters:**
- `x, y` - Character position (top-left)
- `c` - Character to draw (ASCII 32-126)
- `color` - Text color

**Example:**
```cpp
display.drawChar(0, 0, 'A', true);    // Draw 'A' at top-left
display.drawChar(6, 0, 'B', true);    // Draw 'B' next to it
```

### `void drawString(uint8_t x, uint8_t y, const char* str, bool color)`
Draw text string.

**Parameters:**
- `x, y` - Text position (top-left of first character)
- `str` - Null-terminated string to draw
- `color` - Text color

**Example:**
```cpp
display.drawString(0, 0, "Hello World!", true);
display.drawString(0, 16, "Line 2", true);
```

**Note:** Characters are 5×7 pixels with 6-pixel spacing (5 + 1 gap).

---

## Display Control

### `void setContrast(uint8_t contrast)`
Set display contrast/brightness.

**Parameters:**
- `contrast` - Contrast level (0-255, where 0=min, 255=max)

**Example:**
```cpp
display.setContrast(128);  // Medium contrast
display.setContrast(255);  // Maximum brightness
```

### `void invertDisplay(bool invert)`
Invert all display pixels.

**Parameters:**
- `invert` - `true` to invert (white↔black), `false` for normal

**Example:**
```cpp
display.invertDisplay(true);   // Invert colors
display.invertDisplay(false);  // Normal colors
```

### `void displayOn(bool on)`
Turn display on or off.

**Parameters:**
- `on` - `true` to turn display on, `false` to turn off

**Example:**
```cpp
display.displayOn(false);  // Sleep mode (saves power)
display.displayOn(true);   // Wake up display
```

### `void flip(bool horizontal, bool vertical)`
Flip/mirror display orientation.

**Parameters:**
- `horizontal` - `true` to mirror horizontally
- `vertical` - `true` to mirror vertically

**Example:**
```cpp
display.flip(true, false);   // Mirror horizontally only
display.flip(false, true);   // Mirror vertically only
display.flip(true, true);    // Rotate 180 degrees
display.flip(false, false);  // Normal orientation
```

### `void setDisplayStartLine(uint8_t line)`
Set display start line for hardware scrolling.

**Parameters:**
- `line` - Start line (0-127)

**Example:**
```cpp
// Scroll display up by 10 lines
display.setDisplayStartLine(10);
```

---

## Constants and Definitions

### Color Values
```cpp
true   // White/On pixel
false  // Black/Off pixel
```

### Display Dimensions
```cpp
Width:  128 pixels (0-127)
Height: 128 pixels (0-127)
```

### Font Specifications
```cpp
Character size: 5×7 pixels
Character spacing: 6 pixels (5 + 1 gap)
Supported chars: ASCII 32-126
Line height: 8 pixels (recommended)
```

### SPI Configuration
```cpp
Speed: 10MHz
Mode: CPOL=1, CPHA=1 (Mode 3)
Bit order: MSB first
```

---

## Usage Patterns

### Basic Drawing Sequence
```cpp
display.clearDisplay();        // 1. Clear buffer
display.drawString(...);       // 2. Draw content
display.drawCircle(...);       // 3. Draw more content
display.display();             // 4. Update screen
```

### Animation Loop
```cpp
while (true) {
    display.clearDisplay();
    
    // Draw frame
    display.drawCircle(x, y, radius, true);
    
    display.display();
    sleep_ms(50);
    
    // Update animation variables
    x++;
}
```

### Error Handling
```cpp
if (!display.begin()) {
    printf("Display initialization failed\n");
    return -1;
}

// Continue with display operations...
```

### Memory Management
The driver automatically manages the display buffer. No manual memory allocation required.

**Buffer size:** 2048 bytes (128×128÷8)  
**Stack usage:** Minimal (<100 bytes)  
**Heap usage:** 2KB for display buffer
