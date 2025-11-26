# Voltage Data Analysis Results

This directory contains voltage data samples collected from the Pico ADC and their analysis results.

## Data Samples

- **capture_00000.csv** - 10 seconds @ 5kHz, 50,000 samples
- **capture_00002.csv** - 10 seconds @ 5kHz, 50,000 samples  
- **capture_00002_82096ms.csv** - 10 seconds @ 5kHz, 50,000 samples (taken 82s after power-on)

All samples represent stable 11.1V LiPo battery voltage with NO shots fired.

## Analysis Results

### Key Findings

- **Sample Rate:** 5000 Hz (Nyquist: 2500 Hz)
- **RMS Noise:** 55-61 mV (0.75-0.82% of signal)
- **Peak-to-Peak Noise:** 370-512 mV (5-7% of signal)
- **Noise Power Distribution:**
  - **88.2% of noise power is above 100 Hz** ✅
  - High frequency (100-1000 Hz): 61.1%
  - Very high frequency (>1000 Hz): 27.1%
  - Low frequency (<100 Hz): 11.8%

### Dominant Frequencies

- **~106 Hz** - Primary interference peak (likely 2× AC mains harmonic)
- **~212 Hz** - Second harmonic
- **~50-53 Hz** - Fundamental component

### Visualization

See **analysis_results.png** for comprehensive plots showing:
- Time domain voltage traces
- Voltage distribution histograms
- Frequency spectrum (log scale)
- Autocorrelation analysis

## How to Analyze

Run the analysis tool:

```bash
# Create and activate virtual environment
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r ../../requirements.txt

# Run analysis
python ../analyze_samples.py capture_00000.csv capture_00002.csv capture_00002_82096ms.csv
```

## Filtering Recommendations

Based on this analysis, the current software filtering approach is optimal:

✅ **Current Configuration (RECOMMENDED):**
- Median filter (5-sample window) for spike rejection
- IIR Butterworth low-pass (100 Hz cutoff) for noise smoothing
- Removes 88% of noise power
- Preserves transient response for shot detection (10-50 ms events)

❌ **Hardware RC filtering NOT recommended:**
- Adds ~6-10 ms phase lag
- May impact shot detection timing
- Difficult to tune after manufacturing
- Current software approach is sufficient

See **../../docs/devlog/2025-11-14-filtering-analysis-recommendations.md** for detailed analysis and trade-offs.
