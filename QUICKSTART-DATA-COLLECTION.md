# Quick Start Guide - Data Collection

**Purpose:** Collect real-world ADC data from your airsoft gun for shot detection algorithm development.

## What You Get

- Collect 10 seconds of raw ADC samples at 5kHz (50,000 samples)
- Save to flash memory (survives power cycle)
- Download via USB to your PC
- Analyze with Python scripts (plots, statistics, shot detection)

## Hardware Requirements

- Raspberry Pi Pico with airsoft-display firmware
- Airsoft gun with battery voltage divider
- USB cable to PC

## Quick Setup (5 minutes)

### 1. Install Python Tools

```bash
pip install pyserial numpy matplotlib
```

### 2. Flash Updated Firmware

The implementation is ready but needs integration into main.cpp. See `docs/planning/data-collection-integration-example.cpp` for the code to add.

Key changes needed in main.cpp:
```cpp
// Add includes
#include "flash_storage.h"
#include "data_collector.h"

// Add globals
static DataCollector g_data_collector;

// In core1_main(), initialize flash
FlashStorage::init();

// In core1_main() loop, feed DMA buffers
if (g_data_collector.is_collecting()) {
    g_data_collector.process_buffer(buffer, size);
}

// Add serial command handler (see example file)
```

### 3. Connect and Test

```bash
# Find your serial port
# Linux: /dev/ttyACM0
# Mac: /dev/tty.usbmodem*
# Windows: COM3, COM4, etc.

# Test connection
python tools/download_data.py /dev/ttyACM0 list
```

## Collecting Data

### Via Serial Terminal

Connect with any serial terminal (115200 baud):

```
> HELP
Available commands:
  COLLECT <seconds>  - Collect data for N seconds (1-60)
  LIST               - List stored captures
  DOWNLOAD <slot>    - Download a capture
  DELETE <slot>      - Delete a capture
  HELP               - Show this help

> COLLECT 10
Starting 10 second collection...
Progress: 100%
Saved to slot 0
```

### Via Python Script

```bash
# Start 10-second collection
echo "COLLECT 10" | python -c "import serial, sys, time; s=serial.Serial('/dev/ttyACM0', 115200); s.write(b'COLLECT 10\n'); time.sleep(12); print(s.read_all().decode())"
```

## Downloading Data

```bash
# List captures
python tools/download_data.py /dev/ttyACM0 list

# Download slot 0
python tools/download_data.py /dev/ttyACM0 download 0

# Output: capture_00000.bin
```

## Analyzing Data

```bash
python tools/parse_capture.py capture_00000.bin
```

**Creates:**
- `capture_00000.csv` - All samples in CSV format
- `capture_00000.png` - Voltage plot
- Console output with statistics

**Example output:**
```
Capture File: capture_00000.bin
Magic:        0x41444353 ✓
Sample Rate:  5000 Hz
Sample Count: 50000
Duration:     10.000 seconds

Sample Statistics:
  Min Voltage:  7.25 V
  Max Voltage:  12.10 V
  Mean Voltage: 11.05 V

Shot Detection:
  Baseline:   11.05 V
  Threshold:  8.84 V (80% of baseline)
  Shots:      12
```

## Typical Workflow

### 1. Baseline Collection (No Shots)
```bash
> COLLECT 10
# Let gun sit idle for 10 seconds
```
Download and analyze to see baseline voltage and noise.

### 2. Semi-Auto Collection
```bash
> COLLECT 10
# Fire 3-5 single shots during collection
```
Download and analyze to see shot signatures.

### 3. Full-Auto Collection
```bash
> COLLECT 10
# Fire a full-auto burst during collection
```
Download and analyze to see rapid shots and ROF.

## Troubleshooting

**"Could not open port":**
- Unplug and replug USB
- Close other serial programs (Arduino IDE, etc.)
- Linux: `sudo usermod -a -G dialout $USER` (logout/login)

**"Checksum mismatch":**
- Download again (possible USB noise)
- Try different USB cable

**"No captures stored":**
- Collection may have failed
- Check Pico serial output for errors
- Verify integration code is correct

## File Management

```bash
# List all captures
python tools/download_data.py /dev/ttyACM0 list

# Delete old capture
python tools/download_data.py /dev/ttyACM0 delete 0

# Delete all (via serial)
> DELETE 0
> DELETE 1
...
```

**Storage:** Pico can store up to 10 captures (100KB each, 1MB total).

## Next Steps

1. **Collect baseline data** - Idle gun, no shots
2. **Collect semi-auto data** - Single shots
3. **Collect full-auto data** - Rapid fire
4. **Analyze patterns** - Identify voltage drop characteristics
5. **Tune thresholds** - Determine optimal detection parameters
6. **Implement detector** - Build shot detection algorithm

## Files Reference

- **Plan:** `docs/planning/2025-11-06-data-collection-plan.md`
- **Summary:** `docs/planning/2025-11-06-data-collection-implementation-summary.md`
- **Integration:** `docs/planning/data-collection-integration-example.cpp`
- **Tools:** `tools/README.md`

## Support

All code is documented in the header files:
- `lib/flash_storage.h` - Flash storage API
- `lib/data_collector.h` - Collection state machine

Serial commands are case-sensitive. Type `HELP` for command list.

---

**Ready to collect real data for shot detection development!**
