#include "sh1107_demo.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "font8x8.h"

// Runs a demonstration of SH1107 display features.
// If init_display is true, the demo will initialize the display (calls display.begin()).
// Returns true if successful, false if initialization failed.
bool sh1107_demo(SH1107_Display& display, uint32_t delay_ms) {
    display.clearDisplay();
    display.drawString(display.centerx(), display.centery(), "Hello World!");
    display.drawString(display.centerx(), display.centery() + 16, "CENTERED");
    display.display();
    sleep_ms(delay_ms);
    display.clearDisplay();
    for (int y = 0; y < display.getHeight(); y += 16) {
        for (int x = 0; x < display.getWidth(); x += 16) {
            if (((x / 16) + (y / 16)) % 2 == 0) {
                display.drawRect(x, y, 16, 16, 0);
            } else {
                display.fillRect(x, y, 16, 16, 1);
            }
        }
    }
    display.display();
    sleep_ms(delay_ms);
    display.clearDisplay();
    // Draw circles fully within the display area, starting at (0,0)
    int radius = 8;
    // Center circles in each 16x16 cell, starting at (7,7)
    for (int y = 8, row = 0; y + radius <= display.getHeight(); y += 16, ++row) {
        for (int x = 8, col = 0; x + radius <= display.getWidth(); x += 16, ++col) {
            bool fill = ((row + col) % 2 == 0);
            display.drawCircle(x, y, radius, 1, fill ? 1 : 0);
        }
    }
    display.display();
    sleep_ms(delay_ms);
    display.clearDisplay();
    return true;
}
