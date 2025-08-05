#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "sh1107_driver.h"

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

    // Main loop: draw and update the screen
    while (1) {
        display.clearDisplay();
        display.drawString(0, 0, "Hello World!", true);
        display.drawString(0, 16, "Custom Driver", true);
        display.drawString(0, 32, "SH1107 128x128", true);
        display.drawRect(0, 48, 128, 16, true);
        display.display();
        sleep_ms(2000);
        
        display.clearDisplay();
        display.drawString(32, 32, "WORKING!", true);
        display.display();
        sleep_ms(1000);
    }
    
    return 0;
}
