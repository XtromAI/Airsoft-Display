#include "sh1107_demo.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "font8x8.h"

// Runs a demonstration of SH1107 display features.
// If init_display is true, the demo will initialize the display (calls display.begin()).
// Returns true if successful, false if initialization failed.
bool sh1107_demo(SH1107_Display& display, bool init_display) {
    if (init_display) {
        if (!display.begin()) {
            printf("[sh1107_demo] Display initialization failed!\n");
            return false;
        }
    }

    // Clear the display buffer
    display.clearDisplay();
    // Draw only the string 'SH1107 DEMO' at the top using the default font
    display.setFont(&font8x8);
    display.drawString(0, 0, "SH1107 DEMO");
    display.display();
    return true;
}
