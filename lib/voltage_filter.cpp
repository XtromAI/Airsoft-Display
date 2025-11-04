#include "voltage_filter.h"
#include <string.h>

// ==================================================
// MedianFilter Implementation
// ==================================================

MedianFilter::MedianFilter() : index(0) {
    reset();
}

void MedianFilter::reset() {
    for (uint32_t i = 0; i < WINDOW_SIZE; ++i) {
        buffer[i] = 0.0f;
    }
    index = 0;
}

float MedianFilter::process(uint16_t raw_adc) {
    // Add new sample to circular buffer
    buffer[index] = static_cast<float>(raw_adc);
    index = (index + 1) % WINDOW_SIZE;
    
    // Copy buffer for sorting (don't modify original)
    float sorted[WINDOW_SIZE];
    memcpy(sorted, buffer, WINDOW_SIZE * sizeof(float));
    
    // Sort array
    insertion_sort(sorted, WINDOW_SIZE);
    
    // Return median (middle value)
    return sorted[WINDOW_SIZE / 2];
}

void MedianFilter::insertion_sort(float* arr, uint32_t size) {
    for (uint32_t i = 1; i < size; ++i) {
        float key = arr[i];
        int32_t j = i - 1;
        
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// ==================================================
// LowPassFilter Implementation
// ==================================================

LowPassFilter::LowPassFilter() : x_prev(0.0f), y_prev(0.0f) {
}

void LowPassFilter::reset() {
    x_prev = 0.0f;
    y_prev = 0.0f;
}

float LowPassFilter::process(float input) {
    // IIR filter equation; B1 is stored negative so subtract to apply +|B1| feedback
    float output = A0 * input + A1 * x_prev - B1 * y_prev;
    
    // Update state
    x_prev = input;
    y_prev = output;
    
    return output;
}

// ==================================================
// VoltageFilter Implementation
// ==================================================

VoltageFilter::VoltageFilter() {
}

void VoltageFilter::reset() {
    median.reset();
    lpf.reset();
}

float VoltageFilter::process(uint16_t raw_adc) {
    // Stage 1: Remove spikes with median filter
    float despiked = median.process(raw_adc);
    
    // Stage 2: Smooth noise with low-pass filter
    float smoothed = lpf.process(despiked);
    
    return smoothed;
}
