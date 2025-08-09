#include "wave_demo.h"
#include <math.h>

void wave_demo_frame(SH1107_Display& display) {
    static float phase = 0.0f;
    const uint8_t w = display.getWidth();
    const uint8_t h = display.getHeight();
    const float pi = 3.14159265f;
    const float speed = 0.3f; // phase increment per frame
    const float amplitude = h / 6.0f;
    const float y_center = h / 4.0f;

    display.clearDisplay();
    int prev_x = 0;
    int prev_y = (int)(y_center + amplitude * sinf(phase));
    // Increase cycles by multiplying the frequency
    const float cycles = 4.0f; // Number of sine wave cycles across the screen
    // Simulate a highly variable audio waveform by modulating both amplitude and frequency with random noise
    for (int x = 1; x < w; ++x) {
        // Variable amplitude
        float amp_mod = ((rand() % 100) / 100.0f - 0.5f) * amplitude * 1.2f; // [-0.6A, 0.6A]
        float local_amplitude = amplitude + amp_mod;
        // Variable frequency
        float freq_mod = ((rand() % 100) / 100.0f - 0.5f) * 2.0f; // [-1.0, 1.0] cycles
        float local_cycles = cycles + freq_mod;
        float theta = phase + (local_cycles * 2 * pi * x / w);
        int y = (int)(y_center + local_amplitude * sinf(theta));
        // Draw the main line
        display.drawLine(prev_x, prev_y, x, y);
        // Draw a second line offset by 1 pixel vertically for thickness
        display.drawLine(prev_x, prev_y + 1, x, y + 1);
        prev_x = x;
        prev_y = y;
    }
    display.display();
    phase += speed;
    if (phase > 2 * pi) phase -= 2 * pi;
}
