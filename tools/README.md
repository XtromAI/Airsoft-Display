# Data Collection Tools

This directory contains Python scripts for downloading and analyzing ADC capture data from the Pico.

## Prerequisites

```bash
pip install pyserial numpy matplotlib
```

## Tools

### download_data.py

Download captured ADC data from the Pico via USB serial.

**Usage:**
```bash
# List all captures on the Pico
python download_data.py /dev/ttyACM0 list

# Download a specific capture
python download_data.py /dev/ttyACM0 download 0
python download_data.py COM3 download 1 my_capture.bin

# Delete a capture
python download_data.py /dev/ttyACM0 delete 0
```

**On Windows:** Use `COM3`, `COM4`, etc. instead of `/dev/ttyACM0`

**On Mac:** Use `/dev/tty.usbmodem*` (find with `ls /dev/tty.usb*`)

### parse_capture.py

Parse and visualize downloaded capture files.

**Usage:**
```bash
python parse_capture.py capture_00001.bin
```

**Output:**
- `capture_00001.csv` - CSV file with parsed data
- `capture_00001.png` - Plot of voltage over time
- Console output with statistics and shot detection

## Workflow Example

1. **Trigger collection on Pico** (via serial command or button)
   ```
   User sends: COLLECT 10
   Pico responds: Collecting... 100%
   Pico responds: Saved to slot 0
   ```

2. **List captures**
   ```bash
   python download_data.py /dev/ttyACM0 list
   ```
   Output:
   ```
   Slot 0: 50000 samples, 100024 bytes, timestamp: 123456
   ```

3. **Download capture**
   ```bash
   python download_data.py /dev/ttyACM0 download 0
   ```
   Output:
   ```
   Downloading slot 0...
   Receiving 100024 bytes...
   100%
   Saved to capture_00000.bin
   
   Capture Info:
     Magic:        0x41444353 (valid)
     Version:      1
     Sample Rate:  5000 Hz
     Sample Count: 50000
     Timestamp:    123456 ms
     Checksum:     0x12345678
   
   Sample Statistics:
     Min ADC:      1500
     Max ADC:      2500
     Mean ADC:     2000
     Min Voltage:  7.25 V
     Max Voltage:  12.10 V
     Mean Voltage: 9.68 V
   ```

4. **Analyze and visualize**
   ```bash
   python parse_capture.py capture_00000.bin
   ```
   Output:
   - CSV file with all samples
   - PNG plot showing voltage over time
   - Shot detection analysis

## Serial Protocol

The Pico responds to the following commands over USB serial (115200 baud):

### LIST
List all stored captures.

**Request:**
```
LIST\n
```

**Response:**
```
Slot 0: 50000 samples, timestamp: 123456
Slot 1: 50000 samples, timestamp: 234567
```

### DOWNLOAD <slot>
Download a specific capture slot.

**Request:**
```
DOWNLOAD 0\n
```

**Response:**
```
START 100024\n
<binary data: 100024 bytes>
END\n
```

### DELETE <slot>
Delete a specific capture slot.

**Request:**
```
DELETE 0\n
```

**Response:**
```
OK\n
```

### COLLECT <duration_seconds>
Start data collection (implemented in main.cpp).

**Request:**
```
COLLECT 10\n
```

**Response:**
```
Starting 10 second collection...
Progress: 25%
Progress: 50%
Progress: 75%
Progress: 100%
Saved to slot 0
```

## File Format

Binary format with header + samples:

```
Offset | Size | Field          | Value
-------|------|----------------|---------------------------
0x00   | 4    | Magic          | 0x41444353 ("ADCS")
0x04   | 4    | Version        | 0x00000001
0x08   | 4    | Sample Rate    | 5000 (Hz)
0x0C   | 4    | Sample Count   | 50000
0x10   | 4    | Timestamp      | Uptime in ms
0x14   | 4    | Checksum       | CRC32 of sample data
0x18   | ...  | Samples        | uint16_t[50000] (100KB)
```

Total size: 24 bytes header + 100,000 bytes samples = 100,024 bytes

## Troubleshooting

**"Could not open port":**
- Check that the Pico is connected
- Verify port name (use `ls /dev/tty*` on Linux/Mac)
- Close any other programs using the serial port (Arduino IDE, minicom, etc.)
- On Linux, you may need: `sudo usermod -a -G dialout $USER` (then logout/login)

**"Did not receive START message":**
- The Pico may be busy or the slot is invalid
- Try `LIST` command first to verify connection
- Check serial output from Pico for error messages

**"Checksum mismatch":**
- Data corruption during transmission
- Try downloading again
- Check USB cable quality

**No module named 'serial':**
```bash
pip install pyserial  # Note: pyserial, not serial
```

**No module named 'matplotlib':**
```bash
pip install matplotlib numpy
```
