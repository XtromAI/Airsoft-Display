// Dual-channel ADC test
// Purpose: read two ADC channels (GP26=ADC0 and GP27=ADC1) and print both values so
// you can compare DIV_MID (pre-buffer) and OP_OUT (post-buffer).

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "adc_config.h"

// Wiring suggestion:
// - Connect DIV_MID (pre-buffer test point) to GP26 (ADC0)
// - Connect OP_OUT (buffer output) to GP27 (ADC1) — this should match ADCConfig::ADC_GPIO

static constexpr uint PRE_ADC_GPIO = 26;   // GP26, ADC0 — pre-buffer (DIV_MID)
static constexpr uint PRE_ADC_CHANNEL = 0;

static constexpr uint POST_ADC_GPIO = ADCConfig::ADC_GPIO; // GP27 (configured in adc_config.h)
static constexpr uint POST_ADC_CHANNEL = ADCConfig::ADC_CHANNEL;

int main() {
    stdio_init_all();
    sleep_ms(500);
    printf("Dual ADC buffer test starting...\n");

    adc_init();

    // Prepare both ADC GPIO pins (disable pulls and set ADC function)
    gpio_disable_pulls(PRE_ADC_GPIO);
    adc_gpio_init(PRE_ADC_GPIO);
    gpio_disable_pulls(POST_ADC_GPIO);
    adc_gpio_init(POST_ADC_GPIO);

    while (true) {
        // Read pre-buffer (ADC0)
        adc_select_input(PRE_ADC_CHANNEL);
        uint16_t raw_pre = adc_read();
        float v_pre = (float)raw_pre / (float)ADCConfig::ADC_MAX * ADCConfig::ADC_VREF;

        // Read post-buffer (ADC1)
        adc_select_input(POST_ADC_CHANNEL);
        uint16_t raw_post = adc_read();
        float v_post = (float)raw_post / (float)ADCConfig::ADC_MAX * ADCConfig::ADC_VREF;

        // If DIV_MID is pre-buffer and connected to ADC0 via divider, estimate battery
        float batt_est = v_pre * ADCConfig::VDIV_RATIO;

        printf("PRE: raw=%4u  V=%.3f V | POST: raw=%4u  V=%.3f V | batt_est=%.3f V\n",
               raw_pre, v_pre, raw_post, v_post, batt_est);

        sleep_ms(300);
    }

    return 0;
}
