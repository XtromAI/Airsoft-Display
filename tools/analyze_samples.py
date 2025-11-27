#!/usr/bin/env python3
"""
Analyze voltage data samples collected from the Pico ADC.
Performs statistical analysis and frequency domain analysis to identify noise characteristics.

---
Project Context:
    This script is a core analysis tool for the Airsoft Display project (RP2040/Pico ADC data).
    It provides statistical analysis, FFT spectrum, autocorrelation, and visualization plots for CSV data captured from the device.
    Used for noise characterization, filter validation, and system debugging.

Devlog References:
    - See docs/devlog/2025-11-14-filtering-analysis-summary.md for usage and rationale
    - Referenced in filter design and recommendations (see also 2025-11-14-filtering-analysis-recommendations.md)
    - Example usage:
            python tools/analyze_samples.py tools/data/capture_*.csv

This script is maintained as part of the standard analysis toolkit for reproducible research and filter validation.
---
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy import signal, stats
import sys
import os

def load_csv(filename):
    """Load CSV data with time, raw ADC, and voltage columns."""
    data = np.loadtxt(filename, delimiter=',', skiprows=1)
    time_ms = data[:, 0]
    adc_raw = data[:, 1]
    voltage_v = data[:, 2]
    return time_ms, adc_raw, voltage_v

def analyze_statistics(data, name):
    """Compute basic statistics for a dataset."""
    print(f"\n=== {name} Statistics ===")
    print(f"Mean:     {np.mean(data):.3f}")
    print(f"Median:   {np.median(data):.3f}")
    print(f"Std Dev:  {np.std(data):.3f}")
    print(f"Min:      {np.min(data):.3f}")
    print(f"Max:      {np.max(data):.3f}")
    print(f"Range:    {np.max(data) - np.min(data):.3f}")
    print(f"P-P:      {np.ptp(data):.3f}")
    
    # Calculate RMS noise (deviation from mean)
    rms_noise = np.sqrt(np.mean((data - np.mean(data))**2))
    print(f"RMS Noise: {rms_noise:.3f}")
    
    return {
        'mean': np.mean(data),
        'median': np.median(data),
        'std': np.std(data),
        'min': np.min(data),
        'max': np.max(data),
        'range': np.max(data) - np.min(data),
        'rms_noise': rms_noise
    }

def detect_outliers(data, threshold_std=3):
    """Detect outliers using standard deviation method."""
    mean = np.mean(data)
    std = np.std(data)
    outliers = np.abs(data - mean) > threshold_std * std
    return outliers, np.sum(outliers)

def analyze_frequency_domain(time_ms, data, sample_name):
    """Perform FFT analysis to identify frequency components."""
    # Calculate sample rate
    dt = np.mean(np.diff(time_ms)) / 1000.0  # Convert ms to seconds
    fs = 1.0 / dt  # Sample rate in Hz
    
    print(f"\n=== Frequency Domain Analysis ({sample_name}) ===")
    print(f"Sample Rate: {fs:.1f} Hz")
    print(f"Nyquist Freq: {fs/2:.1f} Hz")
    
    # Remove DC component (mean) for better AC analysis
    data_ac = data - np.mean(data)
    
    # Compute FFT
    N = len(data_ac)
    fft = np.fft.rfft(data_ac)
    freqs = np.fft.rfftfreq(N, dt)
    magnitude = np.abs(fft) * 2 / N  # Normalize
    power = magnitude ** 2
    
    # Find dominant frequencies (excluding DC)
    threshold = np.max(magnitude) * 0.1  # 10% of max
    dominant_indices = np.where(magnitude > threshold)[0]
    
    if len(dominant_indices) > 0:
        print(f"\nDominant Frequencies (>10% of peak):")
        for idx in dominant_indices[:10]:  # Top 10
            if freqs[idx] > 0:  # Skip DC
                print(f"  {freqs[idx]:.2f} Hz: magnitude={magnitude[idx]:.4f}")
    
    # Calculate power in different frequency bands
    bands = {
        'DC (0 Hz)': (0, 0.1),
        'Very Low (0.1-1 Hz)': (0.1, 1),
        'Low (1-10 Hz)': (1, 10),
        'Mid (10-100 Hz)': (10, 100),
        'High (100-1000 Hz)': (100, 1000),
        'Very High (>1000 Hz)': (1000, fs/2)
    }
    
    print(f"\nPower Distribution by Frequency Band:")
    total_power = np.sum(power)
    for band_name, (f_low, f_high) in bands.items():
        mask = (freqs >= f_low) & (freqs < f_high)
        band_power = np.sum(power[mask])
        percentage = (band_power / total_power) * 100 if total_power > 0 else 0
        print(f"  {band_name:25s}: {percentage:5.2f}%")
    
    return freqs, magnitude, power, fs

def analyze_autocorrelation(data, max_lag=100):
    """Analyze autocorrelation to detect periodic patterns."""
    # Normalize
    data_normalized = (data - np.mean(data)) / np.std(data)
    
    # Calculate autocorrelation
    autocorr = np.correlate(data_normalized, data_normalized, mode='full')
    autocorr = autocorr[len(autocorr)//2:]  # Keep only positive lags
    autocorr = autocorr / autocorr[0]  # Normalize to 1 at lag 0
    
    # Find peaks (potential periodic components)
    peaks, _ = signal.find_peaks(autocorr[:max_lag], height=0.1)
    
    return autocorr[:max_lag], peaks

def plot_analysis(samples_data):
    """Create comprehensive analysis plots."""
    n_samples = len(samples_data)
    
    # Create figure with subplots
    fig = plt.figure(figsize=(16, 12))
    
    for i, (name, time_ms, adc_raw, voltage_v, freqs, magnitude, power, fs, autocorr, peaks) in enumerate(samples_data):
        # Time domain plot
        ax1 = plt.subplot(n_samples, 4, i*4 + 1)
        ax1.plot(time_ms, voltage_v, linewidth=0.5)
        ax1.set_xlabel('Time (ms)')
        ax1.set_ylabel('Voltage (V)')
        ax1.set_title(f'{name}\nTime Domain')
        ax1.grid(True, alpha=0.3)
        
        # Histogram
        ax2 = plt.subplot(n_samples, 4, i*4 + 2)
        ax2.hist(voltage_v, bins=50, edgecolor='black', alpha=0.7)
        ax2.set_xlabel('Voltage (V)')
        ax2.set_ylabel('Count')
        ax2.set_title('Distribution')
        ax2.grid(True, alpha=0.3)
        
        # Frequency spectrum (log scale)
        ax3 = plt.subplot(n_samples, 4, i*4 + 3)
        ax3.semilogy(freqs, magnitude, linewidth=0.5)
        ax3.set_xlabel('Frequency (Hz)')
        ax3.set_ylabel('Magnitude')
        ax3.set_title('Frequency Spectrum')
        ax3.grid(True, alpha=0.3)
        ax3.set_xlim([0, fs/2])
        
        # Autocorrelation
        ax4 = plt.subplot(n_samples, 4, i*4 + 4)
        ax4.plot(autocorr, linewidth=0.5)
        if len(peaks) > 0:
            ax4.plot(peaks, autocorr[peaks], 'ro', markersize=3)
        ax4.set_xlabel('Lag (samples)')
        ax4.set_ylabel('Autocorrelation')
        ax4.set_title('Autocorrelation')
        ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Save figure
    output_path = os.path.join(os.path.dirname(__file__), 'data', 'analysis_results.png')
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"\nPlots saved to: {output_path}")
    
    plt.show()

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_samples.py <csv_file1> [csv_file2] [csv_file3] ...")
        sys.exit(1)
    
    samples_data = []
    
    for csv_file in sys.argv[1:]:
        if not os.path.exists(csv_file):
            print(f"Error: File not found: {csv_file}")
            continue
        
        print(f"\n{'='*60}")
        print(f"Analyzing: {csv_file}")
        print('='*60)
        
        # Load data
        time_ms, adc_raw, voltage_v = load_csv(csv_file)
        sample_name = os.path.basename(csv_file).replace('.csv', '')
        
        print(f"\nDataset Info:")
        print(f"  Samples: {len(time_ms)}")
        print(f"  Duration: {time_ms[-1] - time_ms[0]:.1f} ms")
        
        # Statistical analysis
        voltage_stats = analyze_statistics(voltage_v, "Voltage (V)")
        adc_stats = analyze_statistics(adc_raw, "ADC Raw")
        
        # Outlier detection
        outliers, num_outliers = detect_outliers(voltage_v, threshold_std=3)
        print(f"\nOutliers (>3σ): {num_outliers} ({num_outliers/len(voltage_v)*100:.2f}%)")
        
        # Frequency domain analysis
        freqs, magnitude, power, fs = analyze_frequency_domain(time_ms, voltage_v, sample_name)
        
        # Autocorrelation analysis
        print(f"\n=== Autocorrelation Analysis ===")
        autocorr, peaks = analyze_autocorrelation(voltage_v)
        if len(peaks) > 0:
            print(f"Periodic peaks found at lags: {peaks[:5]}")
            print(f"Corresponding to frequencies: {[f'{fs/(p+1):.2f} Hz' for p in peaks[:5]]}")
        else:
            print("No significant periodic components detected")
        
        # Store data for plotting
        samples_data.append((
            sample_name, time_ms, adc_raw, voltage_v, 
            freqs, magnitude, power, fs, autocorr, peaks
        ))
    
    # Generate comparison summary
    if len(samples_data) > 1:
        print(f"\n{'='*60}")
        print("COMPARISON SUMMARY")
        print('='*60)
        
        print("\nVoltage Statistics Comparison:")
        print(f"{'Sample':<30} {'Mean':>8} {'Std':>8} {'Range':>8} {'RMS Noise':>10}")
        print("-" * 70)
        
        for name, time_ms, adc_raw, voltage_v, *_ in samples_data:
            mean = np.mean(voltage_v)
            std = np.std(voltage_v)
            range_v = np.max(voltage_v) - np.min(voltage_v)
            rms = np.sqrt(np.mean((voltage_v - mean)**2))
            print(f"{name:<30} {mean:8.3f} {std:8.3f} {range_v:8.3f} {rms:10.6f}")
    
    # Filtering recommendations
    print(f"\n{'='*60}")
    print("FILTERING RECOMMENDATIONS")
    print('='*60)
    
    # Analyze first sample in detail for recommendations
    if samples_data:
        name, time_ms, adc_raw, voltage_v, freqs, magnitude, power, fs, autocorr, peaks = samples_data[0]
        
        # Calculate power in different bands
        total_power = np.sum(power)
        high_freq_power = np.sum(power[(freqs >= 100) & (freqs < fs/2)])
        high_freq_percentage = (high_freq_power / total_power) * 100
        
        print("\nBased on the frequency analysis:")
        print(f"  - Sample Rate: {fs:.1f} Hz")
        print(f"  - Nyquist Frequency: {fs/2:.1f} Hz")
        print(f"  - High Frequency Power (>100 Hz): {high_freq_percentage:.2f}%")
        
        std_dev = np.std(voltage_v)
        print(f"\nNoise Characteristics:")
        print(f"  - Standard Deviation: {std_dev:.3f} V")
        print(f"  - Peak-to-Peak: {np.ptp(voltage_v):.3f} V")
        
        # Recommend cutoff frequencies
        print("\nRecommended Filter Cutoff Frequencies:")
        
        # Find frequency where cumulative power reaches 95%
        cumulative_power = np.cumsum(power)
        idx_95 = np.where(cumulative_power >= 0.95 * total_power)[0]
        if len(idx_95) > 0:
            freq_95 = freqs[idx_95[0]]
            print(f"  - 95% of signal power below: {freq_95:.1f} Hz")
        
        # Find frequency where cumulative power reaches 99%
        idx_99 = np.where(cumulative_power >= 0.99 * total_power)[0]
        if len(idx_99) > 0:
            freq_99 = freqs[idx_99[0]]
            print(f"  - 99% of signal power below: {freq_99:.1f} Hz")
        
        # For shot detection, we need to preserve transients
        print("\nFor shot detection (motor current spikes):")
        print("  - Expected shot duration: 10-50 ms")
        print("  - Minimum frequency to preserve: ~20 Hz (50ms events)")
        print("  - Recommended low-pass cutoff: 50-100 Hz")
        print("  - This preserves transients while removing high-frequency noise")
        
    # Create plots
    plot_analysis(samples_data)

if __name__ == '__main__':
    main()
