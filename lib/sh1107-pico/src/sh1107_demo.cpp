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
    for (int y = 8; y + 8 <= display.getHeight(); y += 16) {
        for (int x = 8; x + 8 <= display.getWidth(); x += 16) {
            display.drawCircle(x, y, 8, 1);
        }
    }
    display.display();
    sleep_ms(delay_ms);
    display.clearDisplay();
    return true;
}
