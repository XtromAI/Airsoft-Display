#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "sh1107_driver.h"
#include "font8x8.h"

#include "font16x16.h"
#include "sh1107_demo.h"

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

    // Run the SH1107 demo once at startup

    // Main loop: (user can add custom drawing here)
    const uint32_t demo_delay_ms = 4000;
    while (1) {
        // Optionally, add your own drawing code here
        sh1107_demo(display, demo_delay_ms);
    }
    return 0;
}
