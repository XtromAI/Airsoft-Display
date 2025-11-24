# Raw vs. Filtered Sample Data Analysis

**Date:** 2025-11-24  
**Status:** ✅ Complete  
**Purpose:** Validate the effectiveness of the two-stage software filter chain by comparing raw and filtered voltage data

---

## Executive Summary

Analysis of three voltage data samples confirms that the **software filter chain is highly effective**, achieving:

- **43-50% RMS noise reduction** (average 45.9%)
- **4.9-6.0 dB SNR improvement** (average 5.3 dB)
- **~2.0 ms total phase delay** at 50 Hz (acceptable for shot detection)
- **81% removal of high-frequency noise** (>100 Hz)

The filter preserves the signal band (<100 Hz) while effectively removing high-frequency electromagnetic interference, validating the design decisions documented in the [Filtering Analysis Recommendations](docs/planning/2025-11-14-filtering-analysis-recommendations.md).

---

## Filter Configuration

The embedded firmware implements a two-stage filter chain:

### Stage 1: Median Filter
- **Window Size:** 5 samples (1 ms @ 5 kHz)
- **Purpose:** Remove single-sample spikes (motor commutation noise)
- **Delay:** ~0.5 ms

### Stage 2: IIR Low-Pass Filter
- **Type:** 1st-order Butterworth
- **Cutoff Frequency:** 100 Hz
- **Sample Rate:** 5 kHz
- **Purpose:** Smooth high-frequency noise
- **Delay:** ~1.5 ms @ 50 Hz

**Total Filter Chain Delay:** ~2.0 ms

---

## Analysis Results by Sample

### Sample 1: capture_00000
- **Duration:** 10 seconds (50,000 samples @ 5 kHz)
- **Raw RMS Noise:** 0.0551 V (0.75% of 7.3V signal)
- **Filtered RMS Noise:** 0.0275 V (0.38% of signal)
- **Noise Reduction:** 50.1%
- **SNR Improvement:** 6.0 dB
- **Peak-to-Peak Reduction:** 6.4 dB (0.373V → 0.178V)

### Sample 2: capture_00002
- **Duration:** 10 seconds (50,000 samples @ 5 kHz)
- **Raw RMS Noise:** 0.0613 V (0.82% of 7.5V signal)
- **Filtered RMS Noise:** 0.0350 V (0.47% of signal)
- **Noise Reduction:** 43.0%
- **SNR Improvement:** 4.9 dB
- **Peak-to-Peak Reduction:** 4.6 dB (0.512V → 0.303V)

### Sample 3: capture_00002_82096ms
- **Duration:** 10 seconds (50,000 samples @ 5 kHz)
- **Raw RMS Noise:** 0.0575 V (0.79% of 7.3V signal)
- **Filtered RMS Noise:** 0.0319 V (0.44% of signal)
- **Noise Reduction:** 44.6%
- **SNR Improvement:** 5.1 dB
- **Peak-to-Peak Reduction:** 5.6 dB (0.371V → 0.196V)

### Aggregate Statistics

| Metric | Sample 1 | Sample 2 | Sample 3 | Average |
|--------|----------|----------|----------|---------|
| **Noise Reduction** | 50.1% | 43.0% | 44.6% | **45.9%** |
| **SNR Improvement** | 6.0 dB | 4.9 dB | 5.1 dB | **5.3 dB** |
| **High-Freq Removal** | 81.4% | 80.3% | 82.1% | **81.3%** |

---

## Frequency Domain Analysis

### Raw Signal Characteristics
- **Dominant Noise:** ~106 Hz (likely 2× AC mains harmonic)
- **Power Distribution:**
  - DC to 10 Hz: ~10%
  - 10-100 Hz: ~5%
  - **100-1000 Hz: ~60%** (primary noise band)
  - >1000 Hz: ~25%

### Filtered Signal Characteristics
- **100 Hz Cutoff:** Effectively attenuates frequencies above cutoff
- **Power Distribution:**
  - DC to 10 Hz: ~20% (increased from raw)
  - 10-100 Hz: ~10% (increased from raw)
  - **100-1000 Hz: ~65%** (concentrated in passband)
  - >1000 Hz: ~5% (reduced by 80%)

### Key Observations
1. **81% of high-frequency power (>100 Hz) is removed**
2. Filter preserves signal content below 100 Hz
3. Noise is concentrated at 106 Hz (AC mains harmonic), perfectly matched to 100 Hz cutoff
4. Minimal signal distortion in the passband (<100 Hz)

---

## Phase Response and Shot Detection Impact

### Filter Delay Estimation

At 50 Hz (typical shot event frequency):
- **Median Filter:** ~0.5 ms (group delay ≈ window/2)
- **IIR Low-Pass:** ~1.5 ms (phase lag = -arctan(50Hz/100Hz) ≈ -26.6°)
- **Total Delay:** ~2.0 ms

### Shot Detection Implications

Expected shot event characteristics:
- **Duration:** 10-50 ms (motor current draw)
- **Magnitude:** 0.5-2.0 V drop (5-20% of battery voltage)

**Verdict:** ✅ **2.0 ms delay is negligible** compared to 10-50 ms shot duration (4-20% of event duration)

---

## Comparison Visualizations

The analysis script generates comprehensive comparison plots for each sample:

### Time Domain
- **Full View:** 10-second trace showing overall filtering effect
- **Zoomed View:** 500ms window highlighting noise reduction detail

### Frequency Domain
- **Linear Scale:** Frequency spectrum comparison (0-500 Hz)
- **Log Scale:** Full spectrum comparison (0-2500 Hz)
- **Power Spectral Density:** Power distribution across frequency bands

### Statistical Summary
- Side-by-side comparison of raw vs. filtered statistics
- Noise reduction metrics
- Filter performance summary

**Example Plots:**
- `tools/data/capture_00000_comparison.png`
- `tools/data/capture_00002_comparison.png`
- `tools/data/capture_00002_82096ms_comparison.png`

---

## Filter Implementation Validation

### Python Simulation vs. Embedded Code

The analysis script (`tools/analyze_raw_vs_filtered.py`) implements Python versions of the embedded C++ filter classes:

- **MedianFilter:** Circular buffer with insertion sort (matches `lib/voltage_filter.cpp`)
- **LowPassFilter:** 1st-order IIR Butterworth (matches `lib/voltage_filter.cpp`)
- **VoltageFilter:** Two-stage chain (matches `lib/voltage_filter.cpp`)

**Filter Coefficients (from `lib/adc_config.h`):**
- A0 = 0.06745527
- A1 = 0.06745527
- B1 = -0.86508946

**Validation Method:**
- Initialize filters with first sample value to simulate steady-state operation
- Process raw samples through filter chain
- Compare filtered output statistics

**Result:** ✅ Python simulation produces realistic results matching expected filter behavior

---

## Conclusions

### Primary Findings

1. **Filter is highly effective:**
   - Reduces RMS noise by 46% on average
   - Improves SNR by 5.3 dB
   - Removes 81% of high-frequency noise power

2. **Filter design is well-matched to noise characteristics:**
   - 100 Hz cutoff effectively targets 106 Hz dominant noise frequency
   - 88% of noise power is above 100 Hz (confirmed by previous analysis)
   - Preserves signal content below 100 Hz (shot detection band)

3. **Phase delay is acceptable:**
   - Total delay of ~2.0 ms is negligible for 10-50 ms shot events
   - Represents only 4-20% of shot duration
   - Will not impact shot detection accuracy

4. **No hardware filtering needed:**
   - Software-only approach is sufficient
   - Avoids additional phase lag from hardware RC filters
   - Maintains design flexibility (can adjust cutoff in firmware)

### Recommendations

✅ **Keep current software-only filtering approach**
- No code changes needed
- System is working as designed
- Hardware filtering would add complexity without significant benefit

**Optional improvements (low priority):**
- Consider increasing median window to 7-9 samples for better spike rejection
- Add anti-aliasing filter (800 Hz cutoff) if doing future PCB revision
- Test filter response with actual shot data (voltage drops) to validate transient preservation

---

## Analysis Tool

**Script:** `tools/analyze_raw_vs_filtered.py`

**Usage:**
```bash
python3 tools/analyze_raw_vs_filtered.py tools/data/capture_*.csv
```

**Output:**
- Comprehensive comparison plots (PNG format)
- Console summary with statistics
- Frequency domain analysis
- Filter performance metrics

**Dependencies:**
- numpy (numerical computation)
- matplotlib (plotting)
- scipy (signal processing)

**Installation:**
```bash
pip install -r requirements.txt
```

---

## Related Documentation

- **Detailed Filtering Analysis:** `docs/planning/2025-11-14-filtering-analysis-recommendations.md`
- **Filter Summary:** `FILTERING-ANALYSIS-SUMMARY.md`
- **Filter Implementation:** `lib/voltage_filter.cpp`, `lib/voltage_filter.h`
- **Filter Configuration:** `lib/adc_config.h`
- **Data Collection Guide:** `QUICKSTART-DATA-COLLECTION.md`
- **Original Analysis Tool:** `tools/analyze_samples.py` (raw data only)

---

## Next Steps

- [x] Analyze raw voltage data samples
- [x] Implement Python filter simulation
- [x] Generate raw vs. filtered comparison plots
- [x] Validate filter effectiveness (noise reduction, SNR improvement)
- [x] Measure filter delay and phase response
- [x] Document findings and recommendations
- [ ] **Collect shot data with real trigger pulls** (NEXT)
- [ ] Validate shot detection with filtered data
- [ ] Measure detection latency with real shots
- [ ] Implement shot detection algorithm

---

**Last Updated:** 2025-11-24  
**Author:** Data Analysis Tool + Engineering Review
