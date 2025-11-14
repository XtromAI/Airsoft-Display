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
    
    # Version 1: 24 bytes header, Version 2: 32 bytes header
    if len(data) < 24:
        print("ERROR: File too small (missing header)")
        return None
    
    # Read first fields to determine version
    magic, version = struct.unpack('<II', data[:8])
    
    if magic != 0x41444353:
        print("ERROR: Invalid magic number")
        return None
    
    # Parse header based on version
    if version == 1:
        header_size = 24
        header = struct.unpack('<IIIIII', data[:header_size])
        magic, version, sample_rate, sample_count, timestamp, checksum = header
        has_filtered = 0
        checksum_filt = 0
    elif version == 2:
        header_size = 32
        if len(data) < header_size:
            print("ERROR: File too small for version 2 header")
            return None
        header = struct.unpack('<IIIIIIII', data[:header_size])
        magic, version, sample_rate, sample_count, timestamp, checksum, has_filtered, checksum_filt = header
    else:
        print(f"ERROR: Unsupported version {version}")
        return None
    
    print("="*60)
    print(f"Capture File: {filename}")
    print("="*60)
    print(f"Magic:        0x{magic:08X} ✓")
    print(f"Version:      {version}")
    print(f"Sample Rate:  {sample_rate} Hz")
    print(f"Sample Count: {sample_count}")
    print(f"Timestamp:    {timestamp} ms ({timestamp/1000:.1f} seconds)")
    print(f"Checksum:     0x{checksum:08X}")
    if version >= 2:
        print(f"Has Filtered: {'Yes' if has_filtered else 'No'}")
        if has_filtered:
            print(f"Filt Checksum: 0x{checksum_filt:08X}")
    
    # Parse raw samples
    raw_data_offset = header_size
    raw_data_size = sample_count * 2
    
    if len(data) < raw_data_offset + raw_data_size:
        print(f"WARNING: File truncated (missing raw data)")
        sample_count = (len(data) - raw_data_offset) // 2
        raw_data_size = sample_count * 2
    
    raw_samples = struct.unpack(f'<{sample_count}H', data[raw_data_offset:raw_data_offset + raw_data_size])
    
    # Verify raw checksum
    import zlib
    calculated_crc = zlib.crc32(data[raw_data_offset:raw_data_offset + raw_data_size]) & 0xFFFFFFFF
    
    if calculated_crc == checksum:
        print(f"Raw Checksum:  ✓ Valid")
    else:
        print(f"Raw Checksum:  ✗ Mismatch (calculated: 0x{calculated_crc:08X})")
    
    # Parse filtered samples if present
    filtered_samples = None
    if version >= 2 and has_filtered:
        filt_data_offset = raw_data_offset + raw_data_size
        filt_data_size = sample_count * 2
        
        if len(data) >= filt_data_offset + filt_data_size:
            filtered_samples = struct.unpack(f'<{sample_count}H', 
                                            data[filt_data_offset:filt_data_offset + filt_data_size])
            
            # Verify filtered checksum
            calculated_filt_crc = zlib.crc32(data[filt_data_offset:filt_data_offset + filt_data_size]) & 0xFFFFFFFF
            
            if calculated_filt_crc == checksum_filt:
                print(f"Filt Checksum: ✓ Valid")
            else:
                print(f"Filt Checksum: ✗ Mismatch (calculated: 0x{calculated_filt_crc:08X})")
        else:
            print(f"WARNING: File truncated (missing filtered data)")
    
    return {
        'header': {
            'magic': magic,
            'version': version,
            'sample_rate': sample_rate,
            'sample_count': sample_count,
            'timestamp': timestamp,
            'checksum': checksum,
            'has_filtered': has_filtered,
            'checksum_filt': checksum_filt
        },
        'raw_samples': np.array(raw_samples, dtype=np.uint16),
        'filtered_samples': np.array(filtered_samples, dtype=np.uint16) if filtered_samples else None
    }


def analyze_samples(data):
    """Analyze and print statistics"""
    raw_samples = data['raw_samples']
    filtered_samples = data.get('filtered_samples')
    sample_rate = data['header']['sample_rate']
    
    # Convert raw samples to voltage
    # 12-bit ADC, 3.3V reference, 3.8x voltage divider
    raw_voltage = (raw_samples / 4095.0) * 3.3 * 3.8
    
    # Convert filtered samples to voltage if present
    filtered_voltage = None
    if filtered_samples is not None:
        filtered_voltage = (filtered_samples / 4095.0) * 3.3 * 3.8
    
    # Time axis
    duration_sec = len(raw_samples) / sample_rate
    time_ms = np.arange(len(raw_samples)) * (1000.0 / sample_rate)
    
    print("="*60)
    print("Sample Statistics")
    print("="*60)
    print(f"Duration:     {duration_sec:.3f} seconds")
    print(f"Samples:      {len(raw_samples)}")
    print(f"\nRAW ADC Values:")
    print(f"  Min:        {np.min(raw_samples)}")
    print(f"  Max:        {np.max(raw_samples)}")
    print(f"  Mean:       {np.mean(raw_samples):.1f}")
    print(f"  Std Dev:    {np.std(raw_samples):.1f}")
    print(f"\nRAW Voltage (scaled to battery):")
    print(f"  Min:        {np.min(raw_voltage):.2f} V")
    print(f"  Max:        {np.max(raw_voltage):.2f} V")
    print(f"  Mean:       {np.mean(raw_voltage):.2f} V")
    print(f"  Std Dev:    {np.std(raw_voltage):.2f} V")
    
    if filtered_voltage is not None:
        print(f"\nFILTERED ADC Values:")
        print(f"  Min:        {np.min(filtered_samples)}")
        print(f"  Max:        {np.max(filtered_samples)}")
        print(f"  Mean:       {np.mean(filtered_samples):.1f}")
        print(f"  Std Dev:    {np.std(filtered_samples):.1f}")
        print(f"\nFILTERED Voltage (scaled to battery):")
        print(f"  Min:        {np.min(filtered_voltage):.2f} V")
        print(f"  Max:        {np.max(filtered_voltage):.2f} V")
        print(f"  Mean:       {np.mean(filtered_voltage):.2f} V")
        print(f"  Std Dev:    {np.std(filtered_voltage):.2f} V")
    
    # Detect potential shots (simple threshold) - use filtered data if available
    voltage_for_detection = filtered_voltage if filtered_voltage is not None else raw_voltage
    baseline = np.median(voltage_for_detection)
    threshold = baseline * 0.8  # 20% drop
    below_threshold = voltage_for_detection < threshold
    
    # Count transitions from above to below threshold
    transitions = np.diff(below_threshold.astype(int))
    shot_count = np.sum(transitions > 0)
    
    print(f"\nShot Detection (simple threshold on {'filtered' if filtered_voltage is not None else 'raw'} data):")
    print(f"  Baseline:   {baseline:.2f} V")
    print(f"  Threshold:  {threshold:.2f} V (80% of baseline)")
    print(f"  Shots:      {shot_count}")
    
    return time_ms, raw_voltage, filtered_voltage, baseline, threshold


def save_csv(filename, data, time_ms, raw_voltage, filtered_voltage):
    """Save data to CSV file"""
    csv_file = filename.with_suffix('.csv')
    
    with open(csv_file, 'w') as f:
        if filtered_voltage is not None:
            f.write("time_ms,adc_raw,voltage_raw_v,adc_filtered,voltage_filtered_v\n")
            for t, adc_raw, v_raw, adc_filt, v_filt in zip(
                time_ms, data['raw_samples'], raw_voltage, 
                data['filtered_samples'], filtered_voltage):
                f.write(f"{t:.3f},{adc_raw},{v_raw:.3f},{adc_filt},{v_filt:.3f}\n")
        else:
            f.write("time_ms,adc_raw,voltage_v\n")
            for t, adc, v in zip(time_ms, data['raw_samples'], raw_voltage):
                f.write(f"{t:.3f},{adc},{v:.3f}\n")
    
    print(f"\nSaved CSV: {csv_file}")


def plot_data(filename, time_ms, raw_voltage, filtered_voltage, baseline, threshold):
    """Create visualization plot"""
    if filtered_voltage is not None:
        fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 12))
    else:
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    
    # Plot 1: Full time series - raw data
    ax1.plot(time_ms / 1000.0, raw_voltage, linewidth=0.5, label='Raw', alpha=0.7)
    if filtered_voltage is not None:
        ax1.plot(time_ms / 1000.0, filtered_voltage, linewidth=0.5, label='Filtered', alpha=0.9)
    ax1.axhline(baseline, color='g', linestyle='--', label=f'Baseline ({baseline:.2f}V)')
    ax1.axhline(threshold, color='r', linestyle='--', label=f'Threshold ({threshold:.2f}V)')
    ax1.set_xlabel('Time (seconds)')
    ax1.set_ylabel('Battery Voltage (V)')
    ax1.set_title(f'ADC Capture - {filename.name}')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: First 500ms zoomed in
    zoom_samples = min(2500, len(raw_voltage))  # 500ms at 5kHz
    ax2.plot(time_ms[:zoom_samples], raw_voltage[:zoom_samples], linewidth=1, label='Raw', alpha=0.7)
    if filtered_voltage is not None:
        ax2.plot(time_ms[:zoom_samples], filtered_voltage[:zoom_samples], linewidth=1, label='Filtered', alpha=0.9)
    ax2.axhline(baseline, color='g', linestyle='--', label=f'Baseline')
    ax2.axhline(threshold, color='r', linestyle='--', label=f'Threshold')
    ax2.set_xlabel('Time (ms)')
    ax2.set_ylabel('Battery Voltage (V)')
    ax2.set_title('Zoomed View - First 500ms')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Plot 3: Comparison of raw vs filtered (if filtered data present)
    if filtered_voltage is not None:
        # Show a smaller window to see filtering effect clearly
        detail_samples = min(250, len(raw_voltage))  # 50ms at 5kHz
        ax3.plot(time_ms[:detail_samples], raw_voltage[:detail_samples], linewidth=1, 
                label='Raw', alpha=0.7, marker='o', markersize=2)
        ax3.plot(time_ms[:detail_samples], filtered_voltage[:detail_samples], linewidth=1.5, 
                label='Filtered', alpha=0.9)
        ax3.set_xlabel('Time (ms)')
        ax3.set_ylabel('Battery Voltage (V)')
        ax3.set_title('Detail View - First 50ms (showing filter effect)')
        ax3.legend()
        ax3.grid(True, alpha=0.3)
    
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
    time_ms, raw_voltage, filtered_voltage, baseline, threshold = analyze_samples(data)
    
    # Save CSV
    save_csv(filename, data, time_ms, raw_voltage, filtered_voltage)
    
    # Plot
    plot_data(filename, time_ms, raw_voltage, filtered_voltage, baseline, threshold)


if __name__ == '__main__':
    main()
