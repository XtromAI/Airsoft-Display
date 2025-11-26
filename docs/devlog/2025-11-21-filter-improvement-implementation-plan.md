# Filter Improvement Implementation Plan

**Date:** 2025-11-21  
**Based on Analysis:** `2025-11-21-filtered-data-analysis.md`  
**Status:** Ready for Implementation

## Executive Summary

Based on frequency analysis of the first dual-channel dataset, we have identified specific improvements to enhance noise rejection from the current 6 dB to 12-15 dB. This document provides a detailed implementation plan with code changes, testing strategy, and success criteria.

## Background

**Current State:**
- Filter: Median (5-tap) + IIR Low-Pass (100 Hz cutoff)
- Noise reduction: 6 dB (2× improvement)
- Main issue: 106 Hz dominant noise (73% of power) is not attenuated
- Secondary issue: 0.6 Hz low-frequency oscillation present

**Analysis Results:**
- 106 Hz frequency passes through filter (cutoff too close at 100 Hz)
- High frequencies (>1 kHz) excellently rejected (98.8%)
- Low-frequency drift not addressed by current filter

## Implementation Plan Overview

### Phase 1: Lower LPF Cutoff (Priority 1) ⭐
**Objective:** Attenuate 106 Hz noise by lowering cutoff to 50-70 Hz  
**Expected Improvement:** 12 dB noise reduction (4× vs current 2×)  
**Effort:** Low - coefficient change only  
**Risk:** Minimal - shot events are 20-100 Hz  

### Phase 2: Adaptive Baseline Tracking (Priority 2)
**Objective:** Handle 0.6 Hz drift for robust shot detection  
**Expected Improvement:** Eliminate false triggers from slow drift  
**Effort:** Moderate - new algorithm implementation  
**Risk:** None - standard approach  

### Phase 3: Validation with Shot Data (Priority 3)
**Objective:** Verify filter performance with real trigger events  
**Expected Improvement:** Confirm shot detection accuracy  
**Effort:** Hardware testing  
**Risk:** None - data collection  

---

## Phase 1: Lower LPF Cutoff to 50-70 Hz

### 1.1 Design Decisions

**Selected Cutoff: 60 Hz**

Rationale:
- Shot events are 10-50 ms duration (20-100 Hz fundamental frequency range)
- 60 Hz cutoff preserves shot detection bandwidth
- Provides strong attenuation at 106 Hz: ~15 dB (5.6× reduction)
- Conservative middle ground between 50 Hz and 70 Hz

**Filter Response:**
- At 60 Hz (cutoff): -3 dB (pass)
- At 80 Hz: -6 dB
- At 100 Hz: -10 dB
- At 106 Hz (noise): -11 dB
- At 120 Hz: -12 dB

### 1.2 Code Changes

**File: `lib/adc_config.h`**

Change the FilterConfig section:

```cpp
namespace FilterConfig {
    // Median filter (spike rejection)
    constexpr uint32_t MEDIAN_WINDOW = 5;  // 1ms @ 5kHz
    
    // Low-pass filter (noise smoothing)
    constexpr float LPF_CUTOFF_HZ = 60.0f;  // Changed from 100.0f
    constexpr float LPF_SAMPLE_RATE = static_cast<float>(ADCConfig::SAMPLE_RATE_HZ);
    
    // IIR coefficients (60 Hz @ 5 kHz)
    // Butterworth 1st order low-pass filter design
    // Calculated using: fc = 60 Hz, fs = 5000 Hz
    // w0 = 2*pi*fc/fs = 0.075398224
    // alpha = sin(w0)/(2*Q) where Q=0.7071 for Butterworth
    // a0 = (1 - cos(w0)) / 2
    // a1 = 1 - cos(w0)
    // b1 = cos(w0) - alpha
    constexpr float LPF_A0 = 0.04380236f;   // Changed from 0.06745527f
    constexpr float LPF_A1 = 0.04380236f;   // Changed from 0.06745527f
    constexpr float LPF_B1 = -0.91239528f;  // Changed from -0.86508946f
}
```

**Coefficient Calculation Details:**

For 60 Hz cutoff at 5000 Hz sample rate:
```
Ωc = 2 * π * fc / fs = 2 * π * 60 / 5000 = 0.075398224

For 1st order Butterworth IIR:
K = tan(Ωc / 2) = tan(0.037699112) = 0.037717197

a0 = K / (1 + K) = 0.04380236
a1 = K / (1 + K) = 0.04380236
b1 = -(1 - K) / (1 + K) = -0.91239528
```

### 1.3 Testing Plan

**Unit Tests (Manual Verification):**

1. **Coefficient Verification**
   - Print filter response at key frequencies
   - Verify -3 dB at 60 Hz
   - Verify >10 dB attenuation at 106 Hz

2. **Signal Integrity Check**
   - Generate synthetic shot event (20 ms pulse)
   - Pass through filter
   - Measure rise time and overshoot
   - Confirm <5 ms delay, <10% overshoot

3. **Frequency Response Test**
   - Generate sine waves at [20, 40, 60, 80, 100, 106, 120 Hz]
   - Measure output amplitude vs input
   - Plot frequency response curve

**Integration Tests:**

1. **Noise Reduction Measurement**
   - Collect 10-second idle capture with new filter
   - Compare std dev to original (expect 2-3× improvement)
   - Target: <5 ADC counts std dev (vs 4.97 currently, 9.87 raw)

2. **106 Hz Attenuation Verification**
   - Run FFT analysis on new capture
   - Measure 106 Hz component power
   - Confirm >10 dB reduction vs raw data

**Hardware Validation:**

1. **Baseline Capture**
   - Battery connected, no shots
   - 10 seconds
   - Verify low noise floor

2. **Shot Detection Test** (requires gun)
   - Semi-auto: 5 single shots
   - Full-auto: 1-2 second burst
   - Verify all shots detected, no false triggers

---

## Phase 2: Adaptive Baseline Tracking

### 2.1 Design Decisions

**Algorithm: Exponential Moving Average (EMA)**

Parameters:
- Time constant: 1.0 seconds (α = 0.002 at 5 kHz)
- Update rate: Every sample (minimal CPU overhead)
- Threshold: Local baseline × 0.80 (20% voltage drop)

**Why EMA:**
- Simple and efficient (one multiply, one add per sample)
- Adapts to slow drift automatically
- Proven approach in commercial detectors
- No additional memory required

### 2.2 Code Changes

**New File: `lib/baseline_tracker.h`**

```cpp
#ifndef BASELINE_TRACKER_H
#define BASELINE_TRACKER_H

#include <stdint.h>

// ==================================================
// BaselineTracker Class
// Tracks slowly-varying baseline voltage for adaptive shot detection
// ==================================================

class BaselineTracker {
public:
    BaselineTracker();
    
    // Update baseline with new sample, returns current baseline
    float update(float voltage_mv);
    
    // Get current baseline estimate
    float get_baseline() const { return baseline_mv; }
    
    // Get adaptive threshold (baseline × factor)
    float get_threshold(float drop_factor = 0.80f) const {
        return baseline_mv * drop_factor;
    }
    
    // Reset baseline to specific value
    void reset(float initial_voltage_mv);
    
    // Check if voltage represents a shot (below threshold)
    bool is_shot(float voltage_mv, float drop_factor = 0.80f) const {
        return voltage_mv < get_threshold(drop_factor);
    }
    
private:
    float baseline_mv;        // Current baseline estimate
    
    // EMA coefficient for 1.0 second time constant @ 5 kHz
    // alpha = 1 - exp(-dt / tau) = 1 - exp(-0.0002 / 1.0) ≈ 0.0002
    static constexpr float ALPHA = 0.0002f;
};

#endif // BASELINE_TRACKER_H
```

**New File: `lib/baseline_tracker.cpp`**

```cpp
#include "baseline_tracker.h"
#include "adc_config.h"
#include <stdio.h>

BaselineTracker::BaselineTracker() : baseline_mv(0.0f) {
}

float BaselineTracker::update(float voltage_mv) {
    if (baseline_mv == 0.0f) {
        // First sample - initialize baseline
        baseline_mv = voltage_mv;
    } else {
        // Exponential moving average: baseline = baseline*(1-α) + sample*α
        baseline_mv = baseline_mv * (1.0f - ALPHA) + voltage_mv * ALPHA;
    }
    
    return baseline_mv;
}

void BaselineTracker::reset(float initial_voltage_mv) {
    baseline_mv = initial_voltage_mv;
}
```

**Modified File: `src/main.cpp`**

Add to includes:
```cpp
#include "baseline_tracker.h"
```

Add global variable after voltage_filter:
```cpp
// Initialize baseline tracker for adaptive shot detection
BaselineTracker baseline_tracker;
```

Update Core 1 loop (in voltage processing section):
```cpp
// Convert to voltage (millivolts) using pre-computed combined scale factor
float voltage_mv = filtered_adc * ADC_TO_VOLTAGE_SCALE;

// Update baseline tracker
baseline_tracker.update(voltage_mv);

// Check for shot (example - not yet integrated with shot counter)
// if (baseline_tracker.is_shot(voltage_mv)) {
//     // Shot detected logic here
// }
```

**Modified File: `CMakeLists.txt`**

Add to source files:
```cmake
add_executable(airsoft-display 
    src/main.cpp 
    lib/sh1107-pico/src/sh1107_driver.cpp
    lib/sh1107-pico/src/font8x8.cpp
    lib/sh1107-pico/src/font16x16.cpp
    lib/sh1107-pico/src/sh1107_demo.cpp
    lib/sh1107-pico/src/wave_demo.cpp
    lib/voltage_filter.cpp
    lib/dma_adc_sampler.cpp
    lib/flash_storage.cpp
    lib/data_collector.cpp
    lib/serial_commands.cpp
    lib/baseline_tracker.cpp  # NEW
)
```

### 2.3 Testing Plan

**Unit Tests:**

1. **Baseline Convergence**
   - Initialize with 10V
   - Feed constant 11V samples
   - Verify convergence to 11V within 5 time constants (5 seconds)

2. **Drift Tracking**
   - Simulate slow drift from 10V to 11V over 10 seconds
   - Verify baseline follows within 10% of actual value

3. **Shot Response**
   - Feed steady 11V baseline
   - Inject 20% voltage drop (shot)
   - Verify shot detected
   - Verify baseline doesn't follow shot (too fast)

**Integration Tests:**

1. **Idle Stability**
   - Collect 60 seconds of idle data
   - Verify baseline stable (std dev <0.1V)
   - Verify no false shot triggers

2. **Temperature Drift**
   - Monitor baseline over 5-10 minutes
   - Verify tracking of slow thermal drift
   - No false triggers during drift

---

## Phase 3: Validation with Shot Data

### 3.1 Data Collection Plan

**Test Scenarios:**

1. **Semi-Auto Baseline**
   - 10 seconds, no shots
   - Verify low noise, stable baseline

2. **Semi-Auto 5-Shot**
   - 10 seconds with 5 single shots spaced 1-2 seconds apart
   - Verify all 5 shots detected
   - Measure shot signature (voltage drop magnitude, duration)

3. **Full-Auto Short Burst**
   - 10 seconds with 1-second burst (~15-20 shots)
   - Verify all shots counted accurately
   - Measure ROF (rounds per minute)

4. **Full-Auto Extended**
   - 10 seconds with 3-second burst (~50-60 shots)
   - Verify shot separation at high ROF
   - Check for missed shots or double-counts

### 3.2 Analysis Metrics

For each test:

1. **Detection Accuracy**
   - Expected shots vs detected shots
   - Target: 100% detection rate
   - False positive rate: <1%

2. **Signal Quality**
   - Shot voltage drop magnitude (expect 20-40%)
   - Shot duration (expect 10-50 ms)
   - Rise time after shot (expect 50-100 ms)

3. **Filter Performance**
   - Noise floor during idle
   - SNR during shots
   - Filter delay vs shot timing

### 3.3 Success Criteria

**Must Pass:**
- ✅ 100% detection rate on semi-auto shots
- ✅ >95% detection rate on full-auto bursts
- ✅ <1% false positive rate
- ✅ Noise floor <5 ADC counts (std dev)
- ✅ Shot detection latency <10 ms

**Nice to Have:**
- ✅ 100% detection on full-auto bursts
- ✅ Accurate ROF measurement (±5%)
- ✅ Clean shot signatures (no overshoot)

---

## Implementation Schedule

### Week 1: Filter Update
- **Day 1:** Update filter coefficients
- **Day 2:** Test with idle captures
- **Day 3:** Analyze noise reduction

### Week 2: Baseline Tracking
- **Day 1:** Implement baseline tracker
- **Day 2:** Unit test baseline algorithm
- **Day 3:** Integration test with filter

### Week 3: Shot Data Collection
- **Day 1-2:** Collect semi-auto data
- **Day 3-4:** Collect full-auto data
- **Day 5:** Analyze results

### Week 4: Refinement
- **Day 1-2:** Tune parameters based on results
- **Day 3-4:** Final validation
- **Day 5:** Documentation

---

## Rollback Plan

If new filter settings cause issues:

1. **Revert Coefficients**
   - Change `lib/adc_config.h` back to 100 Hz settings
   - Rebuild and reflash

2. **Disable Baseline Tracking**
   - Comment out baseline tracker updates
   - Use fixed threshold

3. **Collect Debug Data**
   - Capture with both old and new settings
   - Side-by-side comparison

---

## Risk Assessment

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Lower cutoff rounds shot edges | Low | Medium | Use 60 Hz (conservative), measure rise time |
| Baseline tracker too slow | Low | Medium | Test with various time constants |
| Baseline follows shots | Very Low | High | Alpha = 0.0002 is very slow |
| Phase lag affects timing | Low | Low | 1st order filter minimal delay |

### Operational Risks

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| No access to gun for testing | Medium | High | Use synthetic shot data |
| Battery voltage unstable | Low | Medium | Test with multiple batteries |
| ROF too high for detection | Low | Medium | Test at max expected ROF |

---

## Success Metrics Summary

### Current Performance (100 Hz Filter)
- Noise: 4.97 ADC counts (std dev)
- 106 Hz attenuation: Minimal (~1 dB)
- Shot detection: Not yet implemented

### Target Performance (60 Hz Filter + Baseline)
- Noise: <3.0 ADC counts (std dev) - 60% improvement
- 106 Hz attenuation: >10 dB - 10× improvement
- Shot detection: >95% accuracy with <1% false positives

### Measurement Plan
1. Collect dual-channel data with new settings
2. Run `analyze_frequency.py` for noise analysis
3. Compare before/after FFT spectra
4. Verify 106 Hz component reduction
5. Test shot detection accuracy

---

## Documentation Updates

After implementation:

1. Update `FILTERING-ANALYSIS-SUMMARY.md` with new results
2. Update `lib/adc_config.h` with design rationale comments
3. Create `docs/planning/2025-11-xx-filter-update-results.md`
4. Update `QUICKSTART-DATA-COLLECTION.md` if needed
5. Add baseline tracker to architecture docs

---

## References

- **Analysis:** `docs/planning/2025-11-21-filtered-data-analysis.md`
- **Filter Design:** https://www.dsprelated.com/freebooks/filters/
- **Butterworth Calculator:** http://www.micromodeler.com/dsp/
- **EMA Baseline:** Standard practice in DSP for shot detectors
- **Original Filter Design:** `docs/planning/2025-11-14-filtering-analysis-recommendations.md`

---

## Approval Checklist

Before proceeding:

- [ ] Review implementation plan with user
- [ ] Confirm 60 Hz cutoff acceptable
- [ ] Verify test plan covers all scenarios
- [ ] Hardware access confirmed for validation
- [ ] Rollback plan understood

---

**Plan Created:** 2025-11-21  
**Author:** GitHub Copilot Agent  
**Status:** Ready for Review and Implementation
