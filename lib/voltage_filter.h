#ifndef VOLTAGE_FILTER_H
#define VOLTAGE_FILTER_H

#include <stdint.h>
#include "adc_config.h"

// ==================================================
// MedianFilter Class
// Removes single-sample spikes (motor commutation noise)
// ==================================================

class MedianFilter {
public:
    MedianFilter();
    
    // Process a new sample and return filtered value
    float process(uint16_t raw_adc);
    
    // Reset filter state
    void reset();
    
private:
    static constexpr uint32_t WINDOW_SIZE = FilterConfig::MEDIAN_WINDOW;
    float buffer[WINDOW_SIZE];
    uint32_t index;
    
    // Simple insertion sort for small arrays (fast for size=5)
    void insertion_sort(float* arr, uint32_t size);
};

// ==================================================
// LowPassFilter Class
// IIR Butterworth filter for smoothing noise
// ==================================================

class LowPassFilter {
public:
    LowPassFilter();
    
    // Process a new sample and return filtered value
    float process(float input);
    
    // Reset filter state
    void reset();
    
private:
    float x_prev;  // Previous input
    float y_prev;  // Previous output
    
    // Coefficients for 100 Hz cutoff @ 5 kHz sample rate
    static constexpr float A0 = FilterConfig::LPF_A0;
    static constexpr float A1 = FilterConfig::LPF_A1;
    static constexpr float B1 = FilterConfig::LPF_B1;
};

// ==================================================
// VoltageFilter Class
// Two-stage filtering: Median → Low-pass
// ==================================================

class VoltageFilter {
public:
    VoltageFilter();
    
    // Process raw ADC sample through complete filter chain
    float process(uint16_t raw_adc);
    
    // Reset all filter states
    void reset();
    
private:
    MedianFilter median;
    LowPassFilter lpf;
};

#endif // VOLTAGE_FILTER_H
