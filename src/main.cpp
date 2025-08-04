
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "u8g2.h"
#include "u8x8.h"

// Pin assignments (adjust as needed for your wiring)
#define PIN_SPI_SCK   18
#define PIN_SPI_MOSI  19
#define PIN_SPI_CS    17
#define PIN_SPI_DC    16
#define PIN_SPI_RESET 20

// Forward declarations for u8g2 callback functions
uint8_t u8x8_byte_4wire_hw_spi_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// Minimal Pico SDK SPI and GPIO/delay callbacks for u8g2
uint8_t u8x8_byte_4wire_hw_spi_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
            spi_init(spi0, 1000 * 1000); // 1 MHz, adjust as needed
            gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
            gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
            break;
        case U8X8_MSG_BYTE_SEND:
            spi_write_blocking(spi0, (const uint8_t*)arg_ptr, arg_int);
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

    // Use the u8g2 C API for hardware SPI

    u8g2_Setup_sh1107_128x128_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_4wire_hw_spi_pico,
        u8x8_gpio_and_delay_pico
    );
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_luBIS08_tf); // Only include this font
    u8g2_DrawStr(&u8g2, 10, 64, "Hi");
    u8g2_SendBuffer(&u8g2);

    while (1) {
        tight_loop_contents();
    }
    return 0;
}
