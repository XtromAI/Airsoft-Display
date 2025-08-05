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
            // Set SPI to 1 MHz to match working MicroPython config
            spi_init(spi1, 1000 * 1000);
            spi_set_format(spi1, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
            gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
            gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
            printf("[SPI INIT] SCK=%d MOSI=%d Mode=3 1MHz\n", PIN_SPI_SCK, PIN_SPI_MOSI);
            break;
        case U8X8_MSG_BYTE_SEND:
            printf("[SPI SEND] %d bytes: ", arg_int);
            for (int i = 0; i < arg_int; ++i) {
                printf("%02X ", ((uint8_t*)arg_ptr)[i]);
            }
            printf("\n");
            spi_write_blocking(spi1, (const uint8_t*)arg_ptr, arg_int);
            sleep_us(1);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            printf("[SPI CS LOW]\n");
            gpio_put(PIN_SPI_CS, 0);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            printf("[SPI CS HIGH]\n");
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
            gpio_init(PIN_SPI_CS);
            gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
            gpio_put(PIN_SPI_CS, 1);
            gpio_init(PIN_SPI_DC);
            gpio_set_dir(PIN_SPI_DC, GPIO_OUT);
            gpio_put(PIN_SPI_DC, 0);
            gpio_init(PIN_SPI_RESET);
            gpio_set_dir(PIN_SPI_RESET, GPIO_OUT);
            gpio_put(PIN_SPI_RESET, 1);
            printf("[GPIO INIT] CS=%d DC=%d RESET=%d\n", PIN_SPI_CS, PIN_SPI_DC, PIN_SPI_RESET);
            break;
        case U8X8_MSG_DELAY_MILLI:
            printf("[DELAY] %d ms\n", arg_int);
            sleep_ms(arg_int);
            break;
        case U8X8_MSG_DELAY_10MICRO:
            printf("[DELAY] %d0 us\n", arg_int);
            sleep_us(arg_int * 10);
            break;
        case U8X8_MSG_DELAY_100NANO:
            printf("[DELAY] 100ns (rounded to 1us)\n");
            sleep_us(1);
            break;
        case U8X8_MSG_GPIO_CS:
            printf("[GPIO CS] set to %d\n", arg_int);
            gpio_put(PIN_SPI_CS, arg_int);
            break;
        case U8X8_MSG_GPIO_DC:
            printf("[GPIO DC] set to %d\n", arg_int);
            gpio_put(PIN_SPI_DC, arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:
            printf("[GPIO RESET] set to %d\n", arg_int);
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
    // Use the standard SH1107 128x128 full buffer SPI constructor (with offset workaround)
    u8g2_Setup_sh1107_128x128_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_4wire_hw_spi_pico,
        u8x8_gpio_and_delay_pico
    );


    printf("Initializing display...\n");
    
    // Match MicroPython reset sequence: high 1ms, low 20ms, high 20ms
    gpio_put(PIN_SPI_RESET, 1);
    sleep_ms(1);
    gpio_put(PIN_SPI_RESET, 0);
    sleep_ms(20);
    gpio_put(PIN_SPI_RESET, 1);
    sleep_ms(20);

    // Initialize the display using the selected u8g2 setup function.
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    // Set contrast to 128 (matches MicroPython)
    u8g2_SetContrast(&u8g2, 128);
    // Clear and send buffer after init
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);

    printf("Display initialized, powered on, and cleared.\n");

    // A simple, repeating loop to draw and update the screen
    while (1) {
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
        // Apply 32-pixel offset for SH1107 128x128 SPI (U8G2)
        u8g2_DrawStr(&u8g2, 32, 15, "Hello World!");
        u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
        u8g2_SendBuffer(&u8g2);
        sleep_ms(2000);
        u8g2_ClearBuffer(&u8g2);
        u8g2_SendBuffer(&u8g2);
        sleep_ms(1000);
    }
    
    return 0;
}
