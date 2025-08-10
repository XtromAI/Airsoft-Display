
#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <string>


class Temperature {
public:
	Temperature();
	inline const char* get_formatted_temperature();
	inline float get_raw_temperature();
private:
	static constexpr float conversion_factor = 3.3f / (1 << 12);
	static constexpr float base_temp = 27.0f;
	static constexpr float v_at_base = 0.706f;
	static constexpr float temp_slope = 0.001721f;
	float cached_raw_temperature;
	float last_formatted_raw_temperature;
	uint32_t last_update_ms;
	void init_adc();
	static char formatted_buffer[8];
};

// Inline implementations for efficiency
inline float Temperature::get_raw_temperature() {
	uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	if ((now_ms - last_update_ms) < 1000) {
		return cached_raw_temperature;
	}
	uint16_t raw = adc_read();
	float voltage = raw * conversion_factor;
	// Optionally, replace with fixed-point math if needed for further optimization
	float temperature = base_temp - (voltage - v_at_base) / temp_slope;
	cached_raw_temperature = temperature;
	last_update_ms = now_ms;
	return cached_raw_temperature;
}

inline const char* Temperature::get_formatted_temperature() {
	float temperature = get_raw_temperature();
	if (temperature != last_formatted_raw_temperature) {
		snprintf(formatted_buffer, sizeof(formatted_buffer), "%.1f C", temperature);
		last_formatted_raw_temperature = temperature;
	}
	return formatted_buffer;
}

#endif // TEMPERATURE_H
