

#pragma once




// Temperature sensor interface for reading and formatting temperature values
class Temperature {
public:
	Temperature();
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
	void init_adc();
	static char formatted_buffer[8];
	static_assert(sizeof(formatted_buffer) >= 8, "formatted_buffer must be at least 8 bytes for temperature string");
};

// Inline implementations for efficiency
inline float Temperature::get_raw_temperature() {
	uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	if ((now_ms - last_update_ms) < 1000) {
		return cached_raw_temperature;
	}
	uint16_t raw = adc_read();
			float voltage = raw * kConversionFactor;
			float temperature = kBaseTemp - (voltage - kVAtBase) / kTempSlope;
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


