#include "temperature.h"
#include <cstdio>
#include <string>
#include "pico/stdlib.h"
#include "hardware/adc.h"

std::string get_formatted_temperature() {
    // Initialize ADC if not already done
    static bool adc_initialized = false;
    if (!adc_initialized) {
        adc_init();
        adc_set_temp_sensor_enabled(true);
        adc_select_input(4); // Channel 4 is the onboard temp sensor
        adc_initialized = true;
    }

    // Read raw value from ADC
    uint16_t raw = adc_read();
    // Convert raw value to voltage
    const float conversion_factor = 3.3f / (1 << 12); // 12-bit ADC
    float voltage = raw * conversion_factor;
    // Convert voltage to temperature (RP2040 datasheet formula)
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;

    // Format as string
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.1f C", temperature);
    return std::string(buffer);
}
