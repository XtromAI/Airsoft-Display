#!/usr/bin/env python3
"""
Analyze frequency components in captured data
Identifies low-frequency oscillations and noise characteristics
"""

import sys
import csv
import numpy as np
from pathlib import Path
import matplotlib.pyplot as plt
from scipy import signal, fft

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

def analyze_frequency_content(data, sample_rate, label="Data"):
    """Perform FFT and identify dominant frequencies"""
    # Remove DC component
    data_ac = data - np.mean(data)
    
    # Compute FFT
    n = len(data_ac)
    fft_vals = np.fft.rfft(data_ac)
    fft_freq = np.fft.rfftfreq(n, 1.0/sample_rate)
    fft_mag = np.abs(fft_vals)
    
    # Compute power spectral density
    psd = (fft_mag ** 2) / n
    
    # Find peaks
    # Only look at frequencies > 0.1 Hz to exclude DC drift
    valid_idx = fft_freq > 0.1
    valid_freq = fft_freq[valid_idx]
    valid_psd = psd[valid_idx]
    
    # Find top 10 peaks
    peak_indices = signal.find_peaks(valid_psd, height=np.max(valid_psd)*0.01)[0]
    peak_freqs = valid_freq[peak_indices]
    peak_powers = valid_psd[peak_indices]
    
    # Sort by power
    sorted_indices = np.argsort(peak_powers)[::-1]
    top_peaks = [(peak_freqs[i], peak_powers[i]) for i in sorted_indices[:10]]
    
    print(f"\n{'='*60}")
    print(f"{label} - Frequency Analysis")
    print(f"{'='*60}")
    print(f"Sample rate: {sample_rate} Hz")
    print(f"Duration: {len(data)/sample_rate:.3f} seconds")
    print(f"Mean: {np.mean(data):.2f}")
    print(f"Std Dev: {np.std(data):.2f}")
    print(f"Peak-to-Peak: {np.max(data) - np.min(data):.2f}")
    
    print(f"\nTop 10 Frequency Components:")
    print(f"{'Frequency (Hz)':<20} {'Power':<15} {'Period (ms)':<15}")
    print("-" * 50)
    for freq, power in top_peaks:
        period_ms = 1000.0 / freq if freq > 0 else float('inf')
        print(f"{freq:<20.3f} {power:<15.2e} {period_ms:<15.1f}")
    
    # Calculate total power in different frequency bands
    bands = [
        ("DC-1 Hz", 0, 1),
        ("1-10 Hz", 1, 10),
        ("10-50 Hz", 10, 50),
        ("50-100 Hz", 50, 100),
        ("100-500 Hz", 100, 500),
        ("500-1000 Hz", 500, 1000),
        (">1000 Hz", 1000, sample_rate/2)
    ]
    
    print(f"\nPower Distribution by Frequency Band:")
    print(f"{'Band':<20} {'Power':<15} {'% of Total':<15}")
    print("-" * 50)
    
    total_power = np.sum(psd)
    for band_name, f_low, f_high in bands:
        band_idx = (fft_freq >= f_low) & (fft_freq < f_high)
        band_power = np.sum(psd[band_idx])
        band_pct = 100.0 * band_power / total_power
        print(f"{band_name:<20} {band_power:<15.2e} {band_pct:<15.1f}%")
    
    return fft_freq, psd, top_peaks

def plot_frequency_analysis(raw_freq, raw_psd, filt_freq, filt_psd, output_file):
    """Create frequency analysis plots"""
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    
    # Raw data - full spectrum
    ax = axes[0, 0]
    ax.semilogy(raw_freq, raw_psd)
    ax.set_xlabel('Frequency (Hz)')
    ax.set_ylabel('Power Spectral Density')
    ax.set_title('Raw Data - Full Spectrum')
    ax.grid(True, alpha=0.3)
    ax.set_xlim([0, 2500])
    
    # Raw data - low frequency zoom
    ax = axes[0, 1]
    ax.semilogy(raw_freq, raw_psd)
    ax.set_xlabel('Frequency (Hz)')
    ax.set_ylabel('Power Spectral Density')
    ax.set_title('Raw Data - Low Frequency Detail (0-50 Hz)')
    ax.grid(True, alpha=0.3)
    ax.set_xlim([0, 50])
    
    # Filtered data - full spectrum
    ax = axes[1, 0]
    ax.semilogy(filt_freq, filt_psd)
    ax.set_xlabel('Frequency (Hz)')
    ax.set_ylabel('Power Spectral Density')
    ax.set_title('Filtered Data - Full Spectrum')
    ax.grid(True, alpha=0.3)
    ax.set_xlim([0, 2500])
    
    # Filtered data - low frequency zoom
    ax = axes[1, 1]
    ax.semilogy(filt_freq, filt_psd)
    ax.set_xlabel('Frequency (Hz)')
    ax.set_ylabel('Power Spectral Density')
    ax.set_title('Filtered Data - Low Frequency Detail (0-50 Hz)')
    ax.grid(True, alpha=0.3)
    ax.set_xlim([0, 50])
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    print(f"\nFrequency analysis plot saved to: {output_file}")
    
def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_frequency.py <capture.csv>")
        sys.exit(1)
    
    csv_file = Path(sys.argv[1])
    if not csv_file.exists():
        print(f"ERROR: File not found: {csv_file}")
        sys.exit(1)
    
    print(f"Loading data from: {csv_file}")
    time_ms, raw_adc, filtered_adc = load_csv(csv_file)
    
    # Determine sample rate from time data
    time_diff = np.diff(time_ms)
    sample_period_ms = np.mean(time_diff)
    sample_rate = 1000.0 / sample_period_ms
    
    print(f"Detected sample rate: {sample_rate:.1f} Hz")
    
    # Analyze raw data
    raw_freq, raw_psd, raw_peaks = analyze_frequency_content(raw_adc, sample_rate, "RAW DATA")
    
    # Analyze filtered data
    filt_freq, filt_psd, filt_peaks = analyze_frequency_content(filtered_adc, sample_rate, "FILTERED DATA")
    
    # Compare noise reduction
    raw_noise = np.std(raw_adc)
    filt_noise = np.std(filtered_adc)
    noise_reduction_db = 20 * np.log10(raw_noise / filt_noise)
    
    print(f"\n{'='*60}")
    print(f"FILTER EFFECTIVENESS")
    print(f"{'='*60}")
    print(f"Raw noise (std dev):       {raw_noise:.2f} ADC counts")
    print(f"Filtered noise (std dev):  {filt_noise:.2f} ADC counts")
    print(f"Noise reduction:           {noise_reduction_db:.1f} dB")
    print(f"Noise reduction factor:    {raw_noise/filt_noise:.2f}x")
    
    # Create frequency analysis plot
    output_png = csv_file.with_name(csv_file.stem + '_frequency_analysis.png')
    plot_frequency_analysis(raw_freq, raw_psd, filt_freq, filt_psd, output_png)
    
    # Look for low-frequency oscillations
    print(f"\n{'='*60}")
    print(f"LOW-FREQUENCY OSCILLATION ANALYSIS")
    print(f"{'='*60}")
    
    # Check for components below 10 Hz
    low_freq_components = [(f, p) for f, p in raw_peaks if 0.1 < f < 10]
    if low_freq_components:
        print(f"\nDetected {len(low_freq_components)} significant low-frequency component(s) in RAW data:")
        for freq, power in low_freq_components:
            period_sec = 1.0 / freq
            print(f"  {freq:.3f} Hz (period: {period_sec:.2f} seconds) - Power: {power:.2e}")
        
        # Check if these are still present in filtered data
        low_freq_filt = [(f, p) for f, p in filt_peaks if 0.1 < f < 10]
        if low_freq_filt:
            print(f"\nThese low-frequency components are STILL PRESENT in FILTERED data:")
            for freq, power in low_freq_filt:
                period_sec = 1.0 / freq
                print(f"  {freq:.3f} Hz (period: {period_sec:.2f} seconds) - Power: {power:.2e}")
            
            print(f"\n⚠️  RECOMMENDATION:")
            print(f"   The current 100 Hz low-pass filter does NOT remove these low-frequency")
            print(f"   oscillations. Consider adding a high-pass filter or examining the source")
            print(f"   of these oscillations (battery voltage regulation, power supply noise, etc.)")
        else:
            print(f"\n✓ These components are effectively removed by the filter.")
    else:
        print(f"\nNo significant low-frequency oscillations detected (< 10 Hz).")
    
    print(f"\n{'='*60}")
    print("Analysis complete!")
    print(f"{'='*60}\n")

if __name__ == '__main__':
    main()
