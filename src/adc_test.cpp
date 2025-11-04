#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "adc_config.h"

int main() {
    stdio_init_all();
    sleep_ms(500);
    printf("ADC test starting...\n");

    // Initialize ADC hardware
    adc_init();
    // Ensure internal pulls are disabled on the ADC pin
    gpio_disable_pulls(ADCConfig::ADC_GPIO);
    adc_gpio_init(ADCConfig::ADC_GPIO);
    adc_select_input(ADCConfig::ADC_CHANNEL);

    // Drive-test loop: toggle between ADC read and drive-high test
    const uint drive_pin = ADCConfig::ADC_GPIO; // reuse the configured ADC pin for drive test
    while (true) {
        // --- ADC read ---
        uint16_t raw = adc_read(); // 12-bit value (0..ADC_MAX)
        float v_adc = (float)raw / (float)ADCConfig::ADC_MAX * ADCConfig::ADC_VREF; // volts at ADC pin
        float v_batt = v_adc * ADCConfig::VDIV_RATIO; // approximate battery voltage
        printf("ADC read: raw=%u, adc=%.3f V, batt=%.3f V\n", raw, v_adc, v_batt);
        sleep_ms(500);

        // --- Drive pin high test ---
        // Configure as GPIO output and drive high
        gpio_set_function(drive_pin, GPIO_FUNC_SIO);
        gpio_set_dir(drive_pin, GPIO_OUT);
        gpio_disable_pulls(drive_pin);
        gpio_put(drive_pin, 1);
        sleep_ms(200);
        // Read back the pin level and measure ADC raw for comparison
        int lvl = gpio_get(drive_pin);
        // If ADC is still enabled, re-init to read raw value
        adc_init();
        adc_select_input(ADCConfig::ADC_CHANNEL);
        uint16_t raw_after = adc_read();
        float v_adc_after = (float)raw_after / (float)ADCConfig::ADC_MAX * ADCConfig::ADC_VREF;
        printf("Drive test: pin_level=%d, adc_after_drive=%.3f V (raw=%u)\n", lvl, v_adc_after, raw_after);

        // Restore pin to ADC use via adc_gpio_init (sets correct function)
        adc_gpio_init(drive_pin);
        sleep_ms(500);
    }

    return 0;
}
