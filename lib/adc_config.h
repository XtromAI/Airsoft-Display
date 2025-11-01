#ifndef ADC_CONFIG_H
#define ADC_CONFIG_H

#include <stdint.h>

// ==================================================
// ADC Configuration Constants
// Based on sampling-research.md
// ==================================================

namespace ADCConfig {
    // Sampling
    constexpr uint32_t SAMPLE_RATE_HZ = 5000;
    constexpr uint32_t SAMPLE_PERIOD_US = 1'000'000 / SAMPLE_RATE_HZ;  // 200µs
    
    // Buffers (ping-pong)
    constexpr uint32_t BUFFER_SIZE = 512;  // Must be power of 2
    constexpr uint32_t BUFFER_TIME_MS = (BUFFER_SIZE * 1000) / SAMPLE_RATE_HZ;  // 102ms
    
    // ADC hardware
    constexpr uint32_t ADC_GPIO = 26;  // GP26 = ADC0
    constexpr uint32_t ADC_CHANNEL = 0;  // ADC0
    constexpr uint32_t ADC_BITS = 12;
    constexpr uint32_t ADC_MAX = (1 << ADC_BITS) - 1;  // 4095
    constexpr float ADC_VREF = 3.3f;
    
    // Voltage divider (11.1V LiPo → 3.3V max on ADC)
    // Using 28kΩ + 10kΩ (actual hardware configuration)
    constexpr float VDIV_R1 = 28000.0f;  // 28kΩ to battery
    constexpr float VDIV_R2 = 10000.0f;  // 10kΩ to ground
    constexpr float VDIV_RATIO = (VDIV_R1 + VDIV_R2) / VDIV_R2;  // 3.8
}

// ==================================================
// Filter Configuration Constants
// ==================================================

namespace FilterConfig {
    // Median filter (spike rejection)
    constexpr uint32_t MEDIAN_WINDOW = 5;  // 1ms @ 5kHz
    
    // Low-pass filter (noise smoothing)
    constexpr float LPF_CUTOFF_HZ = 100.0f;
    constexpr float LPF_SAMPLE_RATE = static_cast<float>(ADCConfig::SAMPLE_RATE_HZ);
    
    // IIR coefficients (100 Hz @ 5 kHz)
    // Calculated using Butterworth 1st order low-pass filter design
    // http://www.micromodeler.com/dsp/
    constexpr float LPF_A0 = 0.06745527f;
    constexpr float LPF_A1 = 0.06745527f;
    constexpr float LPF_B1 = -0.86508946f;
}

#endif // ADC_CONFIG_H
