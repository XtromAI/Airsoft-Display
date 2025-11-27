#!/usr/bin/env python3
"""
Detailed low-frequency analysis with detrending and filtering recommendations

---
Project Context:
    This script is a core analysis tool for the Airsoft Display project (RP2040/Pico ADC data).
    It focuses on time-domain detrending, low-frequency isolation, baseline drift analysis, and demonstrates high-pass/low-pass filtering on CSV data from the device.
    Used for diagnosing slow drifts, baseline wander, and validating filter design.

Devlog References:
    - See docs/devlog/2025-11-21-filtered-data-analysis.md for usage and rationale
    - Referenced in filter design and validation workflows (see also 2025-11-14-filtering-analysis-summary.md)
    - Example usage:
            python tools/analyze_low_frequency.py tools/data/capture_slot0.csv

This script is maintained as part of the standard analysis toolkit for reproducible research and filter validation.
---
"""

import sys
import csv
import numpy as np
from pathlib import Path
import matplotlib.pyplot as plt
from scipy import signal

def load_csv(filename):
    """Load CSV data"""
    time_ms = []
    raw_adc = []
    filtered_adc = []
    
    with open(filename, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            time_ms.append(float(row['time_ms']))
            raw_adc.append(float(row['adc_raw']))
            filtered_adc.append(float(row['adc_filtered']))
    
    return np.array(time_ms), np.array(raw_adc), np.array(filtered_adc)

def analyze_low_freq_oscillation(time_ms, data, sample_rate, label="Data"):
    """Analyze low-frequency oscillations with detrending"""
    time_s = time_ms / 1000.0
    
    # Apply different detrending techniques
    data_detrended_linear = signal.detrend(data, type='linear')
    data_detrended_const = signal.detrend(data, type='constant')
    
    # Apply high-pass filter at 0.5 Hz to isolate slow drift
    sos_hp = signal.butter(2, 0.5, btype='high', fs=sample_rate, output='sos')
    data_hp = signal.sosfilt(sos_hp, data)
    
    # Apply low-pass filter at 1 Hz to see very slow components
    sos_lp = signal.butter(2, 1.0, btype='low', fs=sample_rate, output='sos')
    data_lp = signal.sosfilt(sos_lp, data)
    
    print(f"\n{'='*60}")
    print(f"{label} - Low-Frequency Component Analysis")
    print(f"{'='*60}")
    print(f"Original signal:")
    print(f"  Mean:     {np.mean(data):.2f} ADC")
    print(f"  Std Dev:  {np.std(data):.2f} ADC")
    print(f"  Min:      {np.min(data):.0f} ADC")
    print(f"  Max:      {np.max(data):.0f} ADC")
    print(f"  P-P:      {np.max(data) - np.min(data):.0f} ADC")
    
    print(f"\nLinear detrended (removes linear drift):")
    print(f"  Std Dev:  {np.std(data_detrended_linear):.2f} ADC")
    print(f"  P-P:      {np.max(data_detrended_linear) - np.min(data_detrended_linear):.0f} ADC")
    
    print(f"\nHigh-pass filtered (>0.5 Hz, removes slow drift):")
    print(f"  Std Dev:  {np.std(data_hp):.2f} ADC")
    print(f"  P-P:      {np.max(data_hp) - np.min(data_hp):.0f} ADC")
    
    print(f"\nLow-pass filtered (<1 Hz, isolates very slow components):")
    print(f"  Std Dev:  {np.std(data_lp):.2f} ADC")
    print(f"  P-P:      {np.max(data_lp) - np.min(data_lp):.0f} ADC")
    
    # Estimate dominant low-frequency period
    # Look at zero crossings in detrended data
    zero_crossings = np.where(np.diff(np.sign(data_detrended_const)))[0]
    if len(zero_crossings) > 1:
        periods = np.diff(zero_crossings) / sample_rate
        avg_period = np.mean(periods) if len(periods) > 0 else 0
        print(f"\nZero-crossing analysis:")
        print(f"  Number of crossings: {len(zero_crossings)}")
        if avg_period > 0:
            print(f"  Average half-period:  {avg_period:.2f} seconds")
            print(f"  Estimated frequency:  {0.5/avg_period:.3f} Hz")
    
    return data_detrended_linear, data_hp, data_lp

def plot_low_freq_analysis(time_ms, raw, raw_lp, filt, filt_lp, output_file):
    """Create time-domain plots showing low-frequency components"""
    time_s = time_ms / 1000.0
    
    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    
    # Full time series - raw vs filtered
    ax = axes[0]
    ax.plot(time_s, raw, 'b-', alpha=0.5, linewidth=0.5, label='Raw')
    ax.plot(time_s, filt, 'r-', alpha=0.7, linewidth=0.8, label='Filtered')
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('ADC Value')
    ax.set_title('Full Time Series: Raw vs Filtered')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Low-frequency components (< 1 Hz)
    ax = axes[1]
    ax.plot(time_s, raw_lp, 'b-', linewidth=1.5, label='Raw (<1 Hz)')
    ax.plot(time_s, filt_lp, 'r-', linewidth=1.5, label='Filtered (<1 Hz)')
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('ADC Value (Low-Pass <1Hz)')
    ax.set_title('Low-Frequency Components (< 1 Hz) - Isolated')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Zoomed view - first 2 seconds
    ax = axes[2]
    zoom_idx = time_s < 2.0
    ax.plot(time_s[zoom_idx], raw[zoom_idx], 'b-', alpha=0.5, linewidth=1, label='Raw')
    ax.plot(time_s[zoom_idx], filt[zoom_idx], 'r-', alpha=0.7, linewidth=1.5, label='Filtered')
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('ADC Value')
    ax.set_title('Zoomed View: First 2 Seconds')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    print(f"\nLow-frequency analysis plot saved to: {output_file}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_low_frequency.py <capture.csv>")
        sys.exit(1)
    
    csv_file = Path(sys.argv[1])
    if not csv_file.exists():
        print(f"ERROR: File not found: {csv_file}")
        sys.exit(1)
    
    print(f"Loading data from: {csv_file}")
    time_ms, raw_adc, filtered_adc = load_csv(csv_file)
    
    # Determine sample rate
    sample_rate = 5000.0  # Known from system
    
    # Analyze low-frequency content
    raw_detrend, raw_hp, raw_lp = analyze_low_freq_oscillation(
        time_ms, raw_adc, sample_rate, "RAW DATA"
    )
    
    filt_detrend, filt_hp, filt_lp = analyze_low_freq_oscillation(
        time_ms, filtered_adc, sample_rate, "FILTERED DATA"
    )
    
    # Create visualization
    output_png = csv_file.with_name(csv_file.stem + '_low_freq_analysis.png')
    plot_low_freq_analysis(time_ms, raw_adc, raw_lp, filtered_adc, filt_lp, output_png)
    
    # Recommendations
    print(f"\n{'='*60}")
    print(f"RECOMMENDATIONS")
    print(f"{'='*60}")
    
    # Check if low-frequency drift is significant
    raw_lf_std = np.std(raw_lp)
    filt_lf_std = np.std(filt_lp)
    
    print(f"\nLow-frequency drift magnitude:")
    print(f"  Raw:      {raw_lf_std:.2f} ADC counts (std dev of <1Hz component)")
    print(f"  Filtered: {filt_lf_std:.2f} ADC counts (std dev of <1Hz component)")
    
    if raw_lf_std > 2.0:  # If low-freq drift is > 2 ADC counts
        print(f"\n⚠️  DETECTED: Significant low-frequency drift/oscillation")
        print(f"\nPossible causes:")
        print(f"  1. Battery voltage droop during capture")
        print(f"  2. Temperature drift in ADC reference")
        print(f"  3. Slow oscillations in power supply regulation")
        print(f"  4. Environmental/thermal effects")
        
        print(f"\nOptions to address this:")
        print(f"  A) Add high-pass filter at 0.5-1 Hz (removes slow drift)")
        print(f"     - Preserves shot events (10-50 ms duration = 20-100 Hz)")
        print(f"     - Removes slow baseline wander")
        print(f"     - Implementation: Add HPF before or after median filter")
        
        print(f"  B) Use baseline tracking/correction")
        print(f"     - Track slow baseline changes (moving average over 1-2 seconds)")
        print(f"     - Subtract baseline from signal")
        print(f"     - Adapt shot detection threshold to local baseline")
        
        print(f"  C) Investigate and fix root cause")
        print(f"     - Check battery voltage regulation")
        print(f"     - Verify ADC reference stability")
        print(f"     - Improve power supply filtering")
    else:
        print(f"\n✓ Low-frequency drift is minimal and likely acceptable.")
        print(f"  Current 100 Hz low-pass filter should be sufficient for shot detection.")
    
    print(f"\n{'='*60}")
    print("Analysis complete!")
    print(f"{'='*60}\n")

if __name__ == '__main__':
    main()
