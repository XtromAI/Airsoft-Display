

#include "temperature.h"
#include <cstdio>
#include <cmath>
#include <limits>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/time.h"

char Temperature::formatted_buffer[8] = {0};

Temperature::Temperature() : cached_raw_temperature(0.0f), last_formatted_raw_temperature(std::numeric_limits<float>::quiet_NaN()), last_update_ms(0) {
    init_adc();
}

void Temperature::init_adc() {
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Channel 4 is the onboard temp sensor
}
