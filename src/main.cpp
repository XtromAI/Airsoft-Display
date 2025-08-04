
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "u8g2.h"
#include "u8x8.h"

// Pin assignments (from design-guide.md)
#define PIN_SPI_SCK   14
#define PIN_SPI_MOSI  15
#define PIN_SPI_CS    13
#define PIN_SPI_DC    21
#define PIN_SPI_RESET 20

// Forward declarations for u8g2 callback functions
uint8_t u8x8_byte_4wire_hw_spi_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// Minimal Pico SDK SPI and GPIO/delay callbacks for u8g2
uint8_t u8x8_byte_4wire_hw_spi_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
            spi_init(spi1, 500 * 1000); // Try slower 500kHz speed
            gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
            gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
            break;
        case U8X8_MSG_BYTE_SEND:
            spi_write_blocking(spi1, (const uint8_t*)arg_ptr, arg_int);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            gpio_put(PIN_SPI_CS, 0);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            gpio_put(PIN_SPI_CS, 1);
            break;
        default:
            return 0;
    }
    return 1;
}

uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            gpio_init(PIN_SPI_CS);
            gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
            gpio_put(PIN_SPI_CS, 1);
            gpio_init(PIN_SPI_DC);
            gpio_set_dir(PIN_SPI_DC, GPIO_OUT);
            gpio_put(PIN_SPI_DC, 0);
            gpio_init(PIN_SPI_RESET);
            gpio_set_dir(PIN_SPI_RESET, GPIO_OUT);
            gpio_put(PIN_SPI_RESET, 1);
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

u8g2_t u8g2;

int main() {
    stdio_init_all();
    
    // Add some debug output
    printf("Starting SH1107 display test...\n");

    // Try different SH1107 variants - some displays need specific initialization
    // Try the Seeed Studio variant (often works better)
    u8g2_Setup_sh1107_seeed_128x128_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_4wire_hw_spi_pico,
        u8x8_gpio_and_delay_pico
    );
    
    // Alternative setups to try:
    // u8g2_Setup_sh1107_128x128_f(&u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi_pico, u8x8_gpio_and_delay_pico);
    // u8g2_Setup_sh1107_i2c_128x128_f(&u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_gpio_and_delay_pico);
    
    printf("Initializing display...\n");
    
    // Perform a manual reset sequence before initialization
    gpio_put(PIN_SPI_RESET, 0);  // Pull reset low
    sleep_ms(10);                // Hold for 10ms
    gpio_put(PIN_SPI_RESET, 1);  // Release reset
    sleep_ms(50);                // Wait for display to stabilize
    
    // Initialize display with proper timing
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    
    // Add a longer delay after initialization
    sleep_ms(200);
    printf("Display initialized.\n");

    // Test 1: Clear and show text
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_luBIS08_tf);
    u8g2_DrawStr(&u8g2, 10, 20, "SH1107 Test");
    u8g2_DrawStr(&u8g2, 10, 40, "Line 2");
    u8g2_DrawStr(&u8g2, 10, 60, "Line 3");
    u8g2_SendBuffer(&u8g2);
    printf("Text sent to display.\n");
    
    sleep_ms(2000);
    
    // Test 2: Draw some graphics
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawBox(&u8g2, 10, 10, 20, 20);      // Filled rectangle
    u8g2_DrawFrame(&u8g2, 40, 10, 20, 20);    // Empty rectangle
    u8g2_DrawCircle(&u8g2, 80, 20, 10, U8G2_DRAW_ALL);  // Circle
    u8g2_DrawLine(&u8g2, 0, 50, 127, 50);     // Horizontal line
    u8g2_DrawLine(&u8g2, 64, 0, 64, 127);     // Vertical line
    u8g2_SendBuffer(&u8g2);
    printf("Graphics sent to display.\n");

    while (1) {
        tight_loop_contents();
    }
    return 0;
}
