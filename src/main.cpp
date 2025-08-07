#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "sh1107_driver.h"
#include "font16x16.h"

// --- Pin assignments ---
// These pins are for SPI1, which uses GPIO 14 and 15 by default.
// Change these if you are using a different SPI instance or different pins.
#define PIN_SPI_SCK     14
#define PIN_SPI_MOSI    15
#define PIN_SPI_CS      13
#define PIN_SPI_DC      21
#define PIN_SPI_RESET   20

// Create display object
SH1107_Display display(spi1, PIN_SPI_CS, PIN_SPI_DC, PIN_SPI_RESET, 128, 128);

int main() {
    stdio_init_all();
    printf("Starting SH1107 display test with custom driver...\n");

    // Configure SPI pins
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);

    // Initialize display
    if (!display.begin()) {
        printf("Display initialization failed!\n");
        while (1) sleep_ms(1000);
    }
    
    printf("Display initialized successfully!\n");

    // Main loop: draw and update the screen with enhanced graphics
    while (1) {
        // // Test 1: Basic text and rectangles
        // display.clearDisplay();
        // display.drawString(0, 0, "Hello World!");
        // display.drawString(0, 16, "Custom Driver");
        // display.drawString(0, 32, "SH1107 128x128");
        // display.drawRect(0, 48, 128, 16, true);
        // display.display();
        // sleep_ms(2000);
        
        // // Test 2: Circle graphics (new function)
        // display.clearDisplay();
        // display.drawString(16, 0, "Circle Test");
        // display.drawCircle(64, 40, 20, true, false);  // Outline circle
        // display.drawCircle(32, 70, 8, true, true);    // Filled circle
        // display.drawCircle(96, 70, 8, true, true);    // Filled circle
        // display.display();
        // sleep_ms(2000);
        
        // // Test 3: Triangle graphics (new function)
        // display.clearDisplay();
        // display.drawString(12, 0, "Triangle Test");
        // display.drawTriangle(64, 20, 40, 60, 88, 60, true, false); // Outline
        // display.drawTriangle(20, 80, 35, 100, 5, 100, true, true); // Filled
        // display.drawTriangle(93, 80, 108, 100, 78, 100, true, true); // Filled
        // display.display();
        // sleep_ms(2000);
        
        // // Test 4: Lines and patterns
        // display.clearDisplay();
        // display.drawString(24, 0, "Line Test");
        // for (int i = 0; i < 128; i += 8) {
        //     display.drawLine(0, 16, i, 128, true);
        // }
        // display.display();
        // sleep_ms(2000);

        // // Test 6: 16x16 Digit Demo
        display.clearDisplay();
        display.drawString(0, 0, "8x8 FONT");
        display.display();
        sleep_ms(2000);
    }
    
    return 0;
}
