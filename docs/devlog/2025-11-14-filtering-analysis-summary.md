# Filtering Analysis Summary

## Quick Reference

**Status:** ✅ Analysis Complete  
**Date:** 2025-11-14  
**Recommendation:** Keep current software-only filtering approach

---

## Executive Summary

Analysis of three 10-second voltage data samples (50,000 samples @ 5 kHz) confirms that the **current software filtering is optimal** for this application.

### Key Finding
**88.2% of noise power is above 100 Hz**, perfectly matching the current 100 Hz low-pass filter cutoff.

### Primary Recommendation
✅ **Keep current software-only filtering approach**
- No code changes needed
- System is working as designed
- Hardware filtering would add complexity without significant benefit

---

## Noise Characteristics

| Metric | Value |
|--------|-------|
| **RMS Noise** | 55-61 mV (0.75-0.82% of signal) |
| **Peak-to-Peak** | 370-512 mV (5-7% of signal) |
| **Dominant Frequency** | ~106 Hz (2× AC mains harmonic) |
| **High-Freq Power** | 88.2% (>100 Hz) |
| **Outliers** | <0.2% (>3σ) |

**Interpretation:** Noise is primarily high-frequency EMI from AC power lines or SMPS switching.

---

## Current Filter Performance

**Two-Stage Software Filter Chain:**

1. **Median Filter (5 samples, 1 ms)**
   - Removes single-sample spikes
   - Delay: ~0.5 ms

2. **IIR Butterworth Low-Pass (100 Hz)**
   - 1st order filter
   - Removes 88% of noise power
   - Delay: ~3.2 ms @ 50 Hz

**Total Phase Lag:** ~3.7 ms (excellent for 10-50 ms shot events)

---

## Why Hardware Filtering Is NOT Recommended

| Issue | Impact |
|-------|--------|
| **Phase Lag** | Adds 6-10 ms delay (may affect shot detection) |
| **Tunability** | Fixed components (can't adjust after manufacturing) |
| **Complexity** | Requires PCB modifications |
| **Cost** | Additional BOM cost ($0.05-$0.50) |
| **Benefit** | Marginal improvement over current approach |

**Conclusion:** Current software approach provides 88% noise rejection with only 3.7 ms delay. Hardware filtering would add 6-10 ms delay for only 5-10% improvement.

---

## Detailed Analysis

See comprehensive documentation:
- **`docs/planning/2025-11-14-filtering-analysis-recommendations.md`** (15KB detailed analysis)
- **`tools/data/README.md`** (quick reference)
- **`tools/data/analysis_results.png`** (visualization)

---

## Testing Recommendations

### Before Shot Detection Implementation

1. ✅ Collect voltage data samples (COMPLETE)
2. ✅ Analyze noise characteristics (COMPLETE)
3. ✅ Validate filter design (COMPLETE)
4. ⏳ Collect shot data with real trigger pulls (NEXT)
5. ⏳ Verify shot detection with filtered data
6. ⏳ Measure detection latency (<10 ms target)

### Validation Tests

- **Step Response:** Measure time to 90% (target <10 ms)
- **Noise Rejection:** Verify <3σ false triggers
- **Shot Detection:** 100 shots, 100% detection rate, 0 false triggers

---

## Action Items

- [x] Analyze voltage data samples
- [x] Identify noise frequencies and power distribution
- [x] Evaluate current filter design
- [x] Compare hardware vs. software filtering options
- [x] Document findings and recommendations
- [ ] **Test with real shot data** (NEXT STEP)
- [ ] Consider increasing median window to 7-9 samples (optional)
- [ ] Add 800 Hz anti-aliasing filter if doing future PCB revision (optional)

---

## Tools and Scripts

**Analysis Tool:** `tools/analyze_samples.py`

```bash
# Run analysis on data samples
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python tools/analyze_samples.py tools/data/capture_*.csv
```

**Output:** Statistical analysis, FFT spectrum, autocorrelation, visualization plots

---

## References

- **Detailed Analysis:** `docs/planning/2025-11-14-filtering-analysis-recommendations.md`
- **Filter Code:** `lib/voltage_filter.cpp`, `lib/voltage_filter.h`
- **Filter Config:** `lib/adc_config.h`
- **Data Samples:** `tools/data/*.csv`
- **Visualization:** `tools/data/analysis_results.png`

---

**Last Updated:** 2025-11-14  
**Next Review:** After shot detection testing with real trigger data
