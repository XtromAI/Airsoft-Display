#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "u8g2.h"
#include "u8x8.h"

// --- Pin assignments ---
// These pins are for SPI1, which uses GPIO 14 and 15 by default.
// Change these if you are using a different SPI instance or different pins.
#define PIN_SPI_SCK     14
#define PIN_SPI_MOSI    15
#define PIN_SPI_CS      13
#define PIN_SPI_DC      21
#define PIN_SPI_RESET   20

// --- Forward declarations for u8g2 callback functions ---
uint8_t u8x8_byte_4wire_hw_spi_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// --- u8g2 callback for SPI byte transfer ---
uint8_t u8x8_byte_4wire_hw_spi_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
            // Explicitly setting the SPI format is crucial for stability.
            // SH1107 and SSD1306 displays typically use CPOL=0 and CPHA=0.
            spi_init(spi1, 250 * 1000);
            spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

            gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
            gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);

            printf("SPI initialized with CPOL=0, CPHA=0\n");
            break;
        case U8X8_MSG_BYTE_SEND:
            // Send the data. arg_ptr is a pointer to the data, arg_int is the length.
            spi_write_blocking(spi1, (const uint8_t*)arg_ptr, arg_int);
            // The small delay here is a good idea to let the display controller
            // catch up, especially at higher SPI speeds.
            sleep_us(1);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            // Activate the Chip Select pin (CS, active low).
            gpio_put(PIN_SPI_CS, 0);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            // Deactivate the Chip Select pin.
            gpio_put(PIN_SPI_CS, 1);
            break;
        default:
            return 0;
    }
    return 1;
}

// --- u8g2 callback for GPIO control and delays ---
uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            // Initializing all the required GPIOs
            gpio_init(PIN_SPI_CS);
            gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
            gpio_put(PIN_SPI_CS, 1); // CS starts high (inactive)

            gpio_init(PIN_SPI_DC);
            gpio_set_dir(PIN_SPI_DC, GPIO_OUT);
            // The DC line determines if we are sending a command (0) or data (1).
            // It should be handled by u8g2, but we initialize it to a safe state.
            gpio_put(PIN_SPI_DC, 0);

            gpio_init(PIN_SPI_RESET);
            gpio_set_dir(PIN_SPI_RESET, GPIO_OUT);
            gpio_put(PIN_SPI_RESET, 1); // RESET starts high (inactive)
            printf("GPIOs for CS, DC, and RESET configured.\n");
            break;
        case U8X8_MSG_DELAY_MILLI:
            sleep_ms(arg_int);
            break;
        case U8X8_MSG_DELAY_10MICRO:
            sleep_us(arg_int * 10);
            break;
        case U8X8_MSG_DELAY_100NANO:
            sleep_us(1); // Minimum delay on Pico
            break;
        case U8X8_MSG_GPIO_CS:
            gpio_put(PIN_SPI_CS, arg_int);
            break;
        case U8X8_MSG_GPIO_DC:
            gpio_put(PIN_SPI_DC, arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:
            gpio_put(PIN_SPI_RESET, arg_int);
            break;
        default:
            break;
    }
    return 1;
}

// The main u8g2 object
u8g2_t u8g2;

int main() {
    stdio_init_all();
    
    printf("Starting SH1107 display test...\n");

    // --- U8g2 Initialization ---
    // The previous generic setup didn't work, so let's try the Pimoroni variant,
    // which has a slightly different set of initialization commands. This is a
    // common way to resolve display-specific issues.
    u8g2_Setup_sh1107_pimoroni_128x128_f(
        &u8g2,
        U8G2_R0, 
        u8x8_byte_4wire_hw_spi_pico,
        u8x8_gpio_and_delay_pico
    );

    // If this still doesn't work, try this one:
    // u8g2_Setup_sh1107_128x128_f(&u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi_pico, u8x8_gpio_and_delay_pico);

    // And then this one:
    // u8g2_Setup_sh1107_seeed_128x128_f(&u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi_pico, u8x8_gpio_and_delay_pico);


    printf("Initializing display...\n");
    
    // The SH1107 can be very sensitive to the reset timing.
    // This is a slightly more robust reset sequence.
    gpio_put(PIN_SPI_RESET, 1);    // Ensure reset is high initially
    sleep_ms(10);
    gpio_put(PIN_SPI_RESET, 0);    // Pull reset low
    sleep_ms(100);                 // Hold low for a sufficient period
    gpio_put(PIN_SPI_RESET, 1);    // Release reset, allowing the display to boot
    sleep_ms(200);                 // Wait for the display controller to stabilize

    // Initialize the display using the selected u8g2 setup function.
    u8g2_InitDisplay(&u8g2);
    // Turn on the display.
    u8g2_SetPowerSave(&u8g2, 0);

    printf("Display initialized and powered on.\n");

    // A simple, repeating loop to draw and update the screen
    while (1) {
        // Clear the buffer
        u8g2_ClearBuffer(&u8g2);
        
        // Set the font and draw some text
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(&u8g2, 0, 15, "Pico SH1107");
        u8g2_DrawStr(&u8g2, 0, 30, "TEST SUCCESS");
        
        // Draw a simple frame
        u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
        
        // Send the buffer to the display
        u8g2_SendBuffer(&u8g2);
        
        sleep_ms(2000); // Wait for 2 seconds
        
        // Clear the display for the next frame
        u8g2_ClearBuffer(&u8g2);
        u8g2_SendBuffer(&u8g2);
        
        sleep_ms(1000); // Wait for 1 second
    }
    
    return 0;
}
