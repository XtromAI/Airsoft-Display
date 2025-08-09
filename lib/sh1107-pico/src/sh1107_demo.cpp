#include <math.h>
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
    display.drawString(display.centerx(), display.centery() - 8, "Hello World!");
    display.drawString(display.centerx(), display.centery() + 8, "CENTERED");
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


// Animates a scrolling sine waveform across the display
void wave_demo(SH1107_Display& display) {
    const uint8_t w = display.getWidth();
    const uint8_t h = display.getHeight();
    const float pi = 3.14159265f;
    float phase = 0.0f;
    const float speed = 0.12f; // phase increment per frame
    const uint32_t frame_delay = 16; // ~60 FPS
    const float amplitude = h / 3.0f;
    const float y_center = h / 2.0f;
    while (true) {
        display.clearDisplay();
        int prev_x = 0;
        int prev_y = (int)(y_center + amplitude * sinf(phase));
        for (int x = 1; x < w; ++x) {
            float theta = phase + (2 * pi * x / w);
            int y = (int)(y_center + amplitude * sinf(theta));
            display.drawLine(prev_x, prev_y, x, y);
            prev_x = x;
            prev_y = y;
        }
        display.display();
        phase += speed;
        if (phase > 2 * pi) phase -= 2 * pi;
        sleep_ms(frame_delay);
    }
}