# Filtered Data Analysis Summary

**Date:** 2025-11-21  
**Dataset:** 10 seconds, 50,000 samples @ 5 kHz  
**Status:** ✅ Analysis Complete

## Quick Summary

The first dual-channel dataset (raw + filtered) has been analyzed. The current median + 100 Hz low-pass filter provides **6 dB noise reduction**, but can be significantly improved.

## Key Findings

### 1. Dominant Noise: 106 Hz (73% of power)
- **Current filter effectiveness:** Minimal (cutoff at 100 Hz is too close)
- **Source:** Likely AC mains harmonic or switching power supply
- **Solution:** Lower LPF cutoff to 50-70 Hz → expect 12 dB improvement

### 2. Low-Frequency Oscillation: 0.6 Hz
- **Period:** 1.67 seconds
- **Status:** Present in both raw and filtered data
- **Source:** Battery droop, thermal drift, or mechanical vibration
- **Solution:** Adaptive baseline tracking for shot detection

### 3. Filter Performance
- ✅ High-frequency rejection (>1 kHz): 98.8% power reduction
- ✅ Noise reduction: 2× improvement (9.87 → 4.97 ADC std dev)
- ⚠️ Dominant 106 Hz noise: Minimal attenuation
- ⚠️ Low-frequency drift: Not addressed

## Recommendations

### Priority 1: Lower LPF Cutoff to 50-70 Hz ⭐
**Impact:** 4× better noise reduction (12 dB vs current 6 dB)  
**Effort:** Simple coefficient change  
**Risk:** Minimal - shot events are 20-100 Hz

```cpp
// In lib/adc_config.h
constexpr float LPF_CUTOFF_HZ = 50.0f;  // Was 100.0f

// Recalculate Butterworth coefficients for 50 Hz @ 5 kHz
constexpr float LPF_A0 = 0.03526474f;
constexpr float LPF_A1 = 0.03526474f;
constexpr float LPF_B1 = -0.92947052f;
```

### Priority 2: Adaptive Baseline Tracking
**Impact:** Robust shot detection despite 0.6 Hz drift  
**Effort:** Moderate - add baseline tracker to shot detector  
**Risk:** None - standard industry approach

```cpp
// Example implementation
float baseline = baseline * 0.99 + current_voltage * 0.01;  // ~1 sec time constant
float threshold = baseline * 0.80;  // 20% drop
if (current_voltage < threshold) {
    // Shot detected
}
```

### Priority 3: Collect Shot Data
**Impact:** Validate filter settings with real events  
**Effort:** Hardware testing required  
**Risk:** None - data collection only

## Analysis Tools

New Python scripts for frequency analysis:

```bash
# FFT analysis and power distribution
python tools/analyze_frequency.py tools/data/capture_slot0.csv

# Time-domain drift analysis
python tools/analyze_low_frequency.py tools/data/capture_slot0.csv
```

## Visualizations

Generated plots in `tools/data/`:
- `capture_slot0_frequency_analysis.png` - Frequency spectrum (raw vs filtered)
- `capture_slot0_low_freq_analysis.png` - Time-domain low-frequency components
- `analysis_summary.png` - Key findings summary

## Full Documentation

See detailed analysis: `docs/planning/2025-11-21-filtered-data-analysis.md`

## Next Steps

1. ✅ Analyze first dual-channel dataset (COMPLETE)
2. ⏳ Lower LPF cutoff to 50-70 Hz (RECOMMENDED)
3. ⏳ Collect shot data for validation
4. ⏳ Implement adaptive baseline tracking
5. ⏳ Test shot detection with new filter settings

---

**Commit:** c52c793  
**Branch:** copilot/add-filtered-data-collection
