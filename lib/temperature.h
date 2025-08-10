
#pragma once
#include "pico/time.h" // Added for get_absolute_time()
#include <cstdio>
#include "hardware/adc.h"

// Enum for temperature units
enum class TemperatureUnit {
	Celsius,
	Fahrenheit
};

// Temperature sensor interface for reading and formatting temperature values
class Temperature {
public:
	// Optional unit parameter, defaults to Celsius
	Temperature(TemperatureUnit unit = TemperatureUnit::Celsius);
	// Returns formatted temperature string (e.g., "23.1 C")
	inline const char* get_formatted_temperature();
	// Returns the latest raw temperature value in Celsius
	inline float get_raw_temperature();
private:
	// Conversion constants for temperature calculation
	static constexpr float kConversionFactor = 3.3f / (1 << 12);
	static constexpr float kBaseTemp = 27.0f;
	static constexpr float kVAtBase = 0.706f;
	static constexpr float kTempSlope = 0.001721f;
	float cached_raw_temperature;
	float last_formatted_raw_temperature;
	uint32_t last_update_ms;
	TemperatureUnit unit_; // Store the selected unit
	float calibration_offset_ = 0.0f; // Default calibration offset
	void init_adc();
	static char formatted_buffer[8];
	static_assert(sizeof(formatted_buffer) >= 8, "formatted_buffer must be at least 8 bytes for temperature string");
public:
	// Setter for calibration offset
	void set_calibration_offset(float offset) { calibration_offset_ = offset; }
};

// Inline implementations for efficiency
inline float Temperature::get_raw_temperature() {
	uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	if ((now_ms - last_update_ms) < 1000) {
		return cached_raw_temperature;
	}
	adc_select_input(4); // Ensure correct ADC channel for temperature sensor
	uint16_t raw = adc_read();
	float voltage = raw * kConversionFactor;
	float temperature = kBaseTemp - (voltage - kVAtBase) / kTempSlope;
	temperature += calibration_offset_;
	cached_raw_temperature = temperature;
	last_update_ms = now_ms;
	return cached_raw_temperature;
}

inline float celsius_to_fahrenheit(float celsius) {
	return celsius * 9.0f / 5.0f + 32.0f;
}

inline const char* Temperature::get_formatted_temperature() {
	float temperature = get_raw_temperature();
	float display_temp = temperature;
	const char* unit_str = "C";
	if (unit_ == TemperatureUnit::Fahrenheit) {
		display_temp = celsius_to_fahrenheit(temperature);
		unit_str = "F";
	}
	if (display_temp != last_formatted_raw_temperature) {
			snprintf(formatted_buffer, sizeof(formatted_buffer), "%.1f\x7F%s", display_temp, unit_str);
		last_formatted_raw_temperature = display_temp;
	}
	return formatted_buffer;
}


