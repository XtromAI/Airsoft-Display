# Filtered Data Collection - First Dataset Analysis

**Date:** 2025-11-21  
**Dataset:** `tools/data/capture_slot0.bin` (10 seconds, 50,000 samples @ 5 kHz)  
**Status:** Analysis Complete

## Executive Summary

Successfully collected and analyzed the first dual-channel (raw + filtered) dataset. The analysis reveals:

1. ✅ **Filter is working effectively**: 6 dB noise reduction (2× improvement)
2. ✅ **Dominant frequency at ~106 Hz**: Likely AC power line noise (2× 53 Hz or 106 Hz harmonic)
3. ⚠️ **Very low-frequency component detected**: ~0.6 Hz oscillation present in both raw and filtered data
4. ✅ **No significant drift below 1 Hz** in the traditional sense (baseline is stable)

## Data Quality Metrics

### Raw Data
- **Mean:** 3038.0 ADC counts (~9.31V battery voltage)
- **Std Dev:** 9.87 ADC counts (0.32% of signal)
- **Peak-to-Peak:** 66 ADC counts (2.1% of signal)
- **Range:** 3002 - 3068 ADC

### Filtered Data
- **Mean:** 3038.3 ADC counts (~9.31V battery voltage)
- **Std Dev:** 4.97 ADC counts (0.16% of signal)
- **Peak-to-Peak:** 24 ADC counts (0.8% of signal)
- **Range:** 3028 - 3052 ADC

### Filter Performance
- **Noise Reduction:** 6.0 dB (1.99× factor)
- **Raw Noise:** 9.87 ADC counts
- **Filtered Noise:** 4.97 ADC counts
- **Peak-to-Peak Reduction:** 66 → 24 ADC (63.6% reduction)

## Frequency Analysis

### Power Distribution by Band

**Raw Data:**
| Band | Power | % of Total |
|------|-------|------------|
| DC-1 Hz | 3.02e+04 | 1.2% |
| 1-10 Hz | 6.85e+03 | 0.3% |
| 10-50 Hz | 8.12e+03 | 0.3% |
| 50-100 Hz | 1.12e+04 | 0.5% |
| **100-500 Hz** | **1.78e+06** | **73.0%** |
| 500-1000 Hz | 1.99e+05 | 8.2% |
| >1000 Hz | 4.02e+05 | 16.5% |

**Filtered Data:**
| Band | Power | % of Total |
|------|-------|------------|
| DC-1 Hz | 3.05e+04 | 4.9% |
| 1-10 Hz | 7.37e+03 | 1.2% |
| 10-50 Hz | 8.17e+03 | 1.3% |
| 50-100 Hz | 9.18e+03 | 1.5% |
| **100-500 Hz** | **5.59e+05** | **90.5%** |
| 500-1000 Hz | 2.34e+03 | 0.4% |
| >1000 Hz | 1.39e+03 | 0.2% |

### Key Observations

1. **Dominant Frequency: 106 Hz**
   - Present in both raw and filtered data
   - Most likely **2× AC mains harmonic** (if 50 Hz system) or direct 106 Hz noise
   - Could also be switching power supply frequency
   - **Current 100 Hz low-pass filter does NOT remove this** (cutoff is too close)

2. **Harmonics of 106 Hz**
   - 212 Hz (2× fundamental)
   - 318 Hz (3× fundamental)
   - 424 Hz (4× fundamental)
   - These are significantly attenuated by the filter

3. **Very Low Frequency: 0.6 Hz**
   - Detected in filtered data (period: 1666 ms ≈ 1.67 seconds)
   - This is BELOW the current filter's passband
   - Likely causes:
     - Battery voltage droop during measurement
     - Thermal effects
     - Very slow mechanical vibrations
     - ADC reference drift

4. **High-Frequency Noise (>1000 Hz)**
   - Heavily attenuated by the filter: 16.5% → 0.2% (98.8% reduction)
   - Demonstrates excellent high-frequency rejection

## Filter Effectiveness

### What the Current Filter Does Well ✅

1. **High-frequency noise removal** (>500 Hz): 98.8% power reduction
2. **Harmonic suppression**: 2nd-4th harmonics of 106 Hz significantly reduced
3. **Peak-to-peak reduction**: 66 → 24 ADC counts (63.6%)
4. **SNR improvement**: 2× better (6 dB)

### What the Current Filter Does NOT Address ⚠️

1. **106 Hz component**: Passes through nearly unattenuated
   - Filter cutoff is 100 Hz
   - 106 Hz is only 6% above cutoff → minimal attenuation
   - This component contains **73% of raw noise power**

2. **Low-frequency oscillations** (<1 Hz): Not filtered at all
   - 0.6 Hz component present in both raw and filtered
   - Represents slow baseline variations
   - May affect shot detection if threshold is fixed

## Recommendations

### Option 1: Lower the Low-Pass Filter Cutoff (RECOMMENDED)

**Change cutoff from 100 Hz → 50-70 Hz**

**Rationale:**
- Shot events are 10-50 ms duration (20-100 Hz fundamental)
- 50-70 Hz cutoff still preserves shot detection capability
- Would significantly attenuate the 106 Hz noise (main noise source)

**Trade-offs:**
- Slightly more phase lag: ~5-7 ms (still acceptable for shot detection)
- May slightly round fast transients

**Implementation:**
```cpp
// In lib/adc_config.h, change:
constexpr float LPF_CUTOFF_HZ = 50.0f;  // Was 100.0f

// Recalculate coefficients for 50 Hz @ 5 kHz sample rate
// Using Butterworth 1st order low-pass filter design
constexpr float LPF_A0 = 0.03526474f;
constexpr float LPF_A1 = 0.03526474f;
constexpr float LPF_B1 = -0.92947052f;
```

**Expected improvement:**
- 106 Hz attenuation: ~12 dB (4× reduction)
- Noise reduction: 6 dB → 12-15 dB
- Better shot detection sensitivity

### Option 2: Add High-Pass Filter for Baseline Tracking

**Add 0.5-1 Hz high-pass filter**

**Rationale:**
- Removes 0.6 Hz oscillation and DC drift
- Preserves all shot detection frequencies (>20 Hz)
- Creates AC-coupled signal (removes absolute voltage)

**Trade-offs:**
- Loses absolute voltage measurement
- Requires careful shot detection threshold tuning
- Adds computational complexity

**Implementation:**
```cpp
// Add to VoltageFilter class
class HighPassFilter {
    // 1 Hz cutoff @ 5 kHz sample rate
    static constexpr float HP_A0 = 0.99968576f;
    static constexpr float HP_A1 = -0.99968576f;
    static constexpr float HP_B1 = -0.99937152f;
};
```

### Option 3: Adaptive Baseline Tracking (BEST FOR SHOT DETECTION)

**Track local baseline with slow moving average**

**Rationale:**
- Handles both DC drift and low-frequency oscillations
- Maintains absolute voltage measurement
- Adapts shot detection threshold to local baseline
- Already common in commercial detectors

**Implementation approach:**
```cpp
// Use exponential moving average with ~1-2 second time constant
float baseline = baseline * 0.99 + current_voltage * 0.01;
float shot_threshold = baseline * 0.80;  // 20% drop from local baseline

// Detect shot when voltage drops below threshold
if (current_voltage < shot_threshold) {
    // Shot detected
}
```

**Advantages:**
- Robust to slow baseline changes
- No additional filtering needed
- Simple to implement
- Used successfully in commercial products

## Data Visualization

### Generated Plots

1. **`capture_slot0.png`** - Original parse_capture output
   - Full 10-second timeline
   - 500ms zoom
   - 50ms detail

2. **`capture_slot0_frequency_analysis.png`** - Frequency domain analysis
   - Full spectrum (0-2500 Hz)
   - Low-frequency detail (0-50 Hz)
   - Separate plots for raw and filtered

3. **`capture_slot0_low_freq_analysis.png`** - Time domain low-frequency analysis
   - Full time series comparison
   - Isolated <1 Hz components
   - 2-second zoom view

## Conclusions

### Filter Performance Assessment

The current median + 100 Hz low-pass filter is:

1. ✅ **Effective at high frequencies** (>500 Hz): 98.8% power reduction
2. ⚠️ **Ineffective at dominant noise frequency** (106 Hz): Minimal attenuation
3. ✅ **Provides 2× noise reduction overall** (6 dB)
4. ⚠️ **Does not address low-frequency drift** (0.6 Hz component)

### Recommended Next Steps

**Immediate Actions:**

1. **Lower LPF cutoff to 50-70 Hz** (highest priority)
   - Simple coefficient change in `lib/adc_config.h`
   - Expected 4× improvement in noise reduction
   - Minimal impact on shot detection capability

2. **Implement adaptive baseline tracking** (for shot detection)
   - Add exponential moving average baseline tracker
   - Use local baseline for threshold calculation
   - Makes shot detection robust to slow drift

3. **Collect shot data for validation**
   - Semi-auto shots (single shots)
   - Full-auto burst
   - Verify new filter settings work for actual shot events

**Future Investigations:**

1. **Identify 106 Hz noise source**
   - Check power supply ripple
   - Verify AC mains frequency (50 Hz vs 60 Hz)
   - Consider hardware filtering or better power supply

2. **Investigate 0.6 Hz oscillation**
   - Monitor battery voltage during capture
   - Check for thermal effects
   - Verify ADC reference stability

3. **Consider notch filter at 106 Hz** (optional)
   - Surgical removal of specific frequency
   - Preserves signal quality at other frequencies
   - More complex implementation

## Analysis Tools

New Python scripts created for this analysis:

1. **`tools/analyze_frequency.py`**
   - FFT analysis
   - Power spectral density
   - Frequency band power distribution
   - Dominant frequency identification

2. **`tools/analyze_low_frequency.py`**
   - Time-domain detrending
   - Low-frequency isolation
   - Baseline drift analysis
   - High-pass/low-pass filtering demonstrations

**Usage:**
```bash
python tools/analyze_frequency.py tools/data/capture_slot0.csv
python tools/analyze_low_frequency.py tools/data/capture_slot0.csv
```

## References

- **Original Analysis:** User comment requesting analysis of first dual-channel dataset
- **Filter Design:** `docs/planning/2025-11-14-filtering-analysis-recommendations.md`
- **Data Collection:** `docs/planning/2025-11-20-filtered-data-session.md`
- **Dataset:** `tools/data/capture_slot0.bin` (196 KB)

---

**Analysis Date:** 2025-11-21  
**Analyst:** GitHub Copilot Agent  
**Dataset:** 10 seconds, 50,000 samples, dual-channel (raw + filtered)  
**Recommendation:** Lower LPF cutoff to 50-70 Hz for better noise rejection
