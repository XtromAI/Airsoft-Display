#include <stdio.h>
#include "pico/stdlib.h"
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
uint8_t u8x8_byte_4wire_sw_spi_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// Software SPI implementation - sometimes more reliable
uint8_t u8x8_byte_4wire_sw_spi_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
            // Initialize pins for software SPI
            gpio_init(PIN_SPI_SCK);
            gpio_set_dir(PIN_SPI_SCK, GPIO_OUT);
            gpio_put(PIN_SPI_SCK, 0);
            
            gpio_init(PIN_SPI_MOSI);
            gpio_set_dir(PIN_SPI_MOSI, GPIO_OUT);
            gpio_put(PIN_SPI_MOSI, 0);
            break;
            
        case U8X8_MSG_BYTE_SEND:
            {
                uint8_t *data = (uint8_t*)arg_ptr;
                for(int i = 0; i < arg_int; i++) {
                    uint8_t byte = data[i];
                    for(int bit = 7; bit >= 0; bit--) {
                        // Set data bit
                        gpio_put(PIN_SPI_MOSI, (byte >> bit) & 1);
                        sleep_us(1); // Small delay
                        
                        // Clock pulse
                        gpio_put(PIN_SPI_SCK, 1);
                        sleep_us(1); // Small delay
                        gpio_put(PIN_SPI_SCK, 0);
                        sleep_us(1); // Small delay
                    }
                }
            }
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
    
    printf("Starting SH1107 display test with SOFTWARE SPI...\n");

    // Try software SPI - sometimes more reliable
    u8g2_Setup_sh1107_128x128_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_4wire_sw_spi_pico,  // Use software SPI
        u8x8_gpio_and_delay_pico
    );
    
    printf("Initializing display...\n");
    
    // Perform a manual reset sequence before initialization
    gpio_put(PIN_SPI_RESET, 0);  // Pull reset low
    sleep_ms(50);                // Hold longer
    gpio_put(PIN_SPI_RESET, 1);  // Release reset
    sleep_ms(100);               // Wait longer for display to stabilize
    
    // Initialize display with proper timing
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    
    // Add a longer delay after initialization
    sleep_ms(500);
    printf("Display initialized.\n");

    // Very simple test
    printf("Testing basic clear...\n");
    u8g2_ClearDisplay(&u8g2);
    sleep_ms(1000);
    
    printf("Testing simple text...\n");
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_DrawStr(&u8g2, 10, 20, "SW SPI TEST");
    u8g2_SendBuffer(&u8g2);
    printf("Test complete.\n");

    while (1) {
        tight_loop_contents();
    }
    return 0;
}
