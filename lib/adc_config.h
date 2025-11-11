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
    constexpr uint32_t ADC_GPIO = 27;  // GP27 = ADC1 (moved from GP26)
    constexpr uint32_t ADC_CHANNEL = 1;  // ADC1
    constexpr uint32_t ADC_BITS = 12;
    constexpr uint32_t ADC_MAX = (1 << ADC_BITS) - 1;  // 4095
    constexpr float ADC_VREF = 3.3f;
    
    // ADC calibration factor
    // Hardware: Diode → TL072 buffer → 100Ω series resistor → GP27 (ADC1)
    // Measured: Actual pin 2.3V, voltage after diode 10.1V
    // Current reading with 1.218 cal: 2.5V at pin, 10.7V displayed
    // Adjustment needed: 2.3/2.5 = 0.92, so new cal: 1.218 × 0.92 = 1.12
    // Verification: ADC_reading × 1.12 × 4.39 should equal 10.1V
    constexpr float ADC_CALIBRATION = 1.12f;
    
    // Voltage divider (11.1V LiPo → 3.3V max on ADC)
    // Using 3.3kΩ + 1kΩ (nominal values)
    // Note: Diode in series drops ~1.1V before divider (11.2V → 10.1V)
    // Measured ratio: 10.1V after diode → 2.3V at ADC pin = 4.39:1
    constexpr float VDIV_R1 = 3300.0f;  // 3.3kΩ to battery (nominal)
    constexpr float VDIV_R2 = 1000.0f;  // 1kΩ to ground (nominal)
    constexpr float VDIV_RATIO = 4.39f;  // Measured actual ratio (after diode drop)
    
    // Diode voltage drop compensation (to display pre-diode battery voltage)
    // Measured: 11.2V battery → 10.1V after diode = 1.1V drop
    constexpr float DIODE_DROP_MV = 1100.0f;  // Add back to show true battery voltage
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
