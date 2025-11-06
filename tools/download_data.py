#!/usr/bin/env python3
"""
Download captured ADC data from Pico via USB serial

Usage:
    python download_data.py <port> <slot_number> [output_file]

Examples:
    python download_data.py /dev/ttyACM0 0
    python download_data.py COM3 1 capture_001.bin
"""

import serial
import struct
import sys
import time
from pathlib import Path


def list_captures(ser):
    """List all captures stored on the Pico"""
    ser.write(b'LIST\n')
    time.sleep(0.1)
    
    print("Captures stored on Pico:")
    while ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        print(f"  {line}")


def download_capture(ser, slot, output_file=None):
    """Download a specific capture slot"""
    if output_file is None:
        output_file = f"capture_{slot:05d}.bin"
    
    print(f"Requesting download of slot {slot}...")
    ser.write(f'DOWNLOAD {slot}\n'.encode())
    
    # Wait for START message
    start_found = False
    timeout = time.time() + 5.0  # 5 second timeout
    
    while time.time() < timeout:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            print(f"Pico: {line}")
            
            if line.startswith('START'):
                parts = line.split()
                if len(parts) >= 2:
                    size = int(parts[1])
                    start_found = True
                    break
            elif 'ERROR' in line or 'Invalid' in line:
                print("Error from Pico, aborting")
                return False
    
    if not start_found:
        print("ERROR: Did not receive START message")
        return False
    
    print(f"Receiving {size} bytes...")
    
    # Read binary data
    data = bytearray()
    bytes_received = 0
    
    while bytes_received < size:
        chunk = ser.read(min(1024, size - bytes_received))
        if not chunk:
            print(f"ERROR: Connection interrupted at {bytes_received}/{size} bytes")
            return False
        data.extend(chunk)
        bytes_received += len(chunk)
        
        # Progress indicator
        if bytes_received % 10000 == 0 or bytes_received == size:
            print(f"  {bytes_received}/{size} bytes ({100*bytes_received//size}%)")
    
    # Wait for END message
    end_found = False
    timeout = time.time() + 2.0
    
    while time.time() < timeout:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            print(f"Pico: {line}")
            if line == 'END':
                end_found = True
                break
    
    if not end_found:
        print("WARNING: Did not receive END message")
    
    # Save to file
    with open(output_file, 'wb') as f:
        f.write(data)
    
    print(f"Saved {len(data)} bytes to {output_file}")
    
    # Parse and verify header
    if len(data) >= 24:
        header = struct.unpack('<IIIIII', data[:24])
        magic, version, sample_rate, sample_count, timestamp, checksum = header
        
        print(f"\nCapture Info:")
        print(f"  Magic:        0x{magic:08X} {'(valid)' if magic == 0x41444353 else '(INVALID!)'}")
        print(f"  Version:      {version}")
        print(f"  Sample Rate:  {sample_rate} Hz")
        print(f"  Sample Count: {sample_count}")
        print(f"  Timestamp:    {timestamp} ms")
        print(f"  Checksum:     0x{checksum:08X}")
        
        # Parse samples
        sample_data = data[24:]
        expected_samples = sample_count
        actual_samples = len(sample_data) // 2
        
        if actual_samples == expected_samples:
            print(f"  Samples:      {actual_samples} (matches header)")
        else:
            print(f"  Samples:      {actual_samples} (MISMATCH: expected {expected_samples})")
        
        # Quick stats on samples
        if actual_samples > 0:
            samples = struct.unpack(f'<{actual_samples}H', sample_data[:actual_samples*2])
            print(f"\nSample Statistics:")
            print(f"  Min ADC:      {min(samples)}")
            print(f"  Max ADC:      {max(samples)}")
            print(f"  Mean ADC:     {sum(samples)//len(samples)}")
            
            # Convert to voltage (assuming 12-bit ADC, 3.3V ref, 3.8x divider)
            def adc_to_volts(adc):
                return (adc / 4095.0) * 3.3 * 3.8
            
            print(f"  Min Voltage:  {adc_to_volts(min(samples)):.2f} V")
            print(f"  Max Voltage:  {adc_to_volts(max(samples)):.2f} V")
            print(f"  Mean Voltage: {adc_to_volts(sum(samples)//len(samples)):.2f} V")
    
    return True


def delete_capture(ser, slot):
    """Delete a specific capture slot"""
    print(f"Deleting slot {slot}...")
    ser.write(f'DELETE {slot}\n'.encode())
    time.sleep(0.5)
    
    while ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        print(f"Pico: {line}")


def main():
    if len(sys.argv) < 2:
        print("Usage: python download_data.py <port> [command] [args...]")
        print("\nCommands:")
        print("  list                    - List all captures")
        print("  download <slot> [file]  - Download capture to file")
        print("  delete <slot>           - Delete a capture")
        print("\nExamples:")
        print("  python download_data.py /dev/ttyACM0 list")
        print("  python download_data.py COM3 download 0")
        print("  python download_data.py /dev/ttyACM0 download 1 my_capture.bin")
        print("  python download_data.py COM3 delete 0")
        sys.exit(1)
    
    port = sys.argv[1]
    command = sys.argv[2] if len(sys.argv) > 2 else 'list'
    
    # Open serial port
    try:
        print(f"Opening {port}...")
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(0.5)  # Wait for connection to stabilize
        
        # Clear any pending data
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        print("Connected")
        
    except serial.SerialException as e:
        print(f"ERROR: Could not open {port}: {e}")
        sys.exit(1)
    
    try:
        if command == 'list':
            list_captures(ser)
        
        elif command == 'download':
            if len(sys.argv) < 4:
                print("ERROR: download requires slot number")
                sys.exit(1)
            
            slot = int(sys.argv[3])
            output_file = sys.argv[4] if len(sys.argv) > 4 else None
            
            success = download_capture(ser, slot, output_file)
            sys.exit(0 if success else 1)
        
        elif command == 'delete':
            if len(sys.argv) < 4:
                print("ERROR: delete requires slot number")
                sys.exit(1)
            
            slot = int(sys.argv[3])
            delete_capture(ser, slot)
        
        else:
            print(f"ERROR: Unknown command '{command}'")
            sys.exit(1)
    
    finally:
        ser.close()
        print("Disconnected")


if __name__ == '__main__':
    main()
