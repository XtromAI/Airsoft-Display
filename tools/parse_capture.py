#!/usr/bin/env python3
"""
Parse and visualize captured ADC data from Pico

Usage:
    python parse_capture.py <capture_file.bin>

Creates:
    - CSV file with parsed data
    - PNG plot of voltage over time
"""

import struct
import sys
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path


def parse_capture(filename):
    """Parse binary capture file"""
    with open(filename, 'rb') as f:
        data = f.read()
    
    if len(data) < 24:
        print("ERROR: File too small (missing header)")
        return None
    
    # Parse header
    header = struct.unpack('<IIIIII', data[:24])
    magic, version, sample_rate, sample_count, timestamp, checksum = header
    
    print("="*60)
    print(f"Capture File: {filename}")
    print("="*60)
    print(f"Magic:        0x{magic:08X} {'✓' if magic == 0x41444353 else '✗ INVALID'}")
    print(f"Version:      {version}")
    print(f"Sample Rate:  {sample_rate} Hz")
    print(f"Sample Count: {sample_count}")
    print(f"Timestamp:    {timestamp} ms ({timestamp/1000:.1f} seconds)")
    print(f"Checksum:     0x{checksum:08X}")
    
    if magic != 0x41444353:
        print("ERROR: Invalid magic number")
        return None
    
    # Parse samples
    sample_data = data[24:]
    expected_bytes = sample_count * 2
    actual_bytes = len(sample_data)
    
    if actual_bytes < expected_bytes:
        print(f"WARNING: File truncated ({actual_bytes} < {expected_bytes} bytes)")
        sample_count = actual_bytes // 2
    
    samples = struct.unpack(f'<{sample_count}H', sample_data[:sample_count*2])
    
    # Verify checksum
    import zlib
    calculated_crc = zlib.crc32(sample_data[:sample_count*2]) & 0xFFFFFFFF
    
    if calculated_crc == checksum:
        print(f"Checksum:     ✓ Valid")
    else:
        print(f"Checksum:     ✗ Mismatch (calculated: 0x{calculated_crc:08X})")
    
    return {
        'header': {
            'magic': magic,
            'version': version,
            'sample_rate': sample_rate,
            'sample_count': sample_count,
            'timestamp': timestamp,
            'checksum': checksum
        },
        'samples': np.array(samples, dtype=np.uint16)
    }


def analyze_samples(data):
    """Analyze and print statistics"""
    samples = data['samples']
    sample_rate = data['header']['sample_rate']
    
    # Convert to voltage
    # 12-bit ADC, 3.3V reference, 3.8x voltage divider
    voltage = (samples / 4095.0) * 3.3 * 3.8
    
    # Time axis
    duration_sec = len(samples) / sample_rate
    time_ms = np.arange(len(samples)) * (1000.0 / sample_rate)
    
    print("="*60)
    print("Sample Statistics")
    print("="*60)
    print(f"Duration:     {duration_sec:.3f} seconds")
    print(f"Samples:      {len(samples)}")
    print(f"\nADC Raw Values:")
    print(f"  Min:        {np.min(samples)}")
    print(f"  Max:        {np.max(samples)}")
    print(f"  Mean:       {np.mean(samples):.1f}")
    print(f"  Std Dev:    {np.std(samples):.1f}")
    print(f"\nVoltage (scaled to battery):")
    print(f"  Min:        {np.min(voltage):.2f} V")
    print(f"  Max:        {np.max(voltage):.2f} V")
    print(f"  Mean:       {np.mean(voltage):.2f} V")
    print(f"  Std Dev:    {np.std(voltage):.2f} V")
    
    # Detect potential shots (simple threshold)
    baseline = np.median(voltage)
    threshold = baseline * 0.8  # 20% drop
    below_threshold = voltage < threshold
    
    # Count transitions from above to below threshold
    transitions = np.diff(below_threshold.astype(int))
    shot_count = np.sum(transitions > 0)
    
    print(f"\nShot Detection (simple threshold):")
    print(f"  Baseline:   {baseline:.2f} V")
    print(f"  Threshold:  {threshold:.2f} V (80% of baseline)")
    print(f"  Shots:      {shot_count}")
    
    return time_ms, voltage, baseline, threshold


def save_csv(filename, data, time_ms, voltage):
    """Save data to CSV file"""
    csv_file = filename.with_suffix('.csv')
    
    with open(csv_file, 'w') as f:
        f.write("time_ms,adc_raw,voltage_v\n")
        for t, adc, v in zip(time_ms, data['samples'], voltage):
            f.write(f"{t:.3f},{adc},{v:.3f}\n")
    
    print(f"\nSaved CSV: {csv_file}")


def plot_data(filename, time_ms, voltage, baseline, threshold):
    """Create visualization plot"""
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    
    # Plot 1: Full time series
    ax1.plot(time_ms / 1000.0, voltage, linewidth=0.5)
    ax1.axhline(baseline, color='g', linestyle='--', label=f'Baseline ({baseline:.2f}V)')
    ax1.axhline(threshold, color='r', linestyle='--', label=f'Threshold ({threshold:.2f}V)')
    ax1.set_xlabel('Time (seconds)')
    ax1.set_ylabel('Battery Voltage (V)')
    ax1.set_title(f'ADC Capture - {filename.name}')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: First 500ms zoomed in
    zoom_samples = min(2500, len(voltage))  # 500ms at 5kHz
    ax2.plot(time_ms[:zoom_samples], voltage[:zoom_samples], linewidth=1)
    ax2.axhline(baseline, color='g', linestyle='--', label=f'Baseline')
    ax2.axhline(threshold, color='r', linestyle='--', label=f'Threshold')
    ax2.set_xlabel('Time (ms)')
    ax2.set_ylabel('Battery Voltage (V)')
    ax2.set_title('Zoomed View - First 500ms')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    png_file = filename.with_suffix('.png')
    plt.savefig(png_file, dpi=150)
    print(f"Saved plot: {png_file}")
    
    plt.show()


def main():
    if len(sys.argv) < 2:
        print("Usage: python parse_capture.py <capture_file.bin>")
        print("\nExample:")
        print("  python parse_capture.py capture_00001.bin")
        sys.exit(1)
    
    filename = Path(sys.argv[1])
    
    if not filename.exists():
        print(f"ERROR: File not found: {filename}")
        sys.exit(1)
    
    # Parse file
    data = parse_capture(filename)
    if data is None:
        sys.exit(1)
    
    # Analyze
    time_ms, voltage, baseline, threshold = analyze_samples(data)
    
    # Save CSV
    save_csv(filename, data, time_ms, voltage)
    
    # Plot
    plot_data(filename, time_ms, voltage, baseline, threshold)


if __name__ == '__main__':
    main()
