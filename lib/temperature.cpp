
#include "temperature.h"
#include <limits>
#include "hardware/adc.h"

char Temperature::formatted_buffer[8] = {0};

Temperature::Temperature(TemperatureUnit unit)
    : cached_raw_temperature(0.0f),
      last_formatted_raw_temperature(std::numeric_limits<float>::quiet_NaN()),
      last_update_ms(0),
      unit_(unit)
{
    init_adc();
}

// Initialize ADC hardware for onboard temperature sensor
void Temperature::init_adc() {
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Channel 4 is the onboard temp sensor
}
