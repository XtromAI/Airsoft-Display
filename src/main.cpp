#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    const uint LED_PIN = 25; // Onboard LED for Raspberry Pi Pico
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    printf("Blinking onboard LED on pin %d.\n", LED_PIN);
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
    return 0;
}
