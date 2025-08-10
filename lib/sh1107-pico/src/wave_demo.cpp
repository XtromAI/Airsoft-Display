#include "wave_demo.h"
#include <math.h>

void wave_demo_frame(SH1107_Display& display) {
    static float phase = 0.0f;
    const uint8_t w = display.getWidth();
    const uint8_t h = display.getHeight();
    const float pi = 3.14159265f;
    const float speed = 0.3f; // phase increment per frame
    const float amplitude = h / 3.0f;
    const float y_center = h / 2.0f;

    display.clearDisplay();
    int prev_x = 0;
    const float cycles = 4.0f; // Number of sine wave cycles across the screen
    // Draw a clean sine wave
    int prev_y = (int)(y_center + amplitude * sinf(phase));
    for (int x = 1; x < w; ++x) {
        float t = phase + (cycles * 2 * pi * x / w);
        int y = (int)(y_center + amplitude * sinf(t));
        display.drawLine(prev_x, prev_y, x, y);
        display.drawLine(prev_x, prev_y + 1, x, y + 1);
        prev_x = x;
        prev_y = y;
    }
    display.display();
    phase += speed;
    if (phase > 2 * pi) phase -= 2 * pi;
}
