# Data Collection Implementation Summary

**Date:** 2025-11-06  
**Status:** Implementation Complete - Ready for Hardware Integration  

## What Was Implemented

This implementation provides a complete system for collecting 10 seconds of raw ADC samples at 5kHz and downloading them via USB serial for offline analysis.

### Files Created

```
docs/planning/
  2025-11-06-data-collection-plan.md          # Comprehensive design document
  data-collection-integration-example.cpp     # Integration code examples

lib/
  flash_storage.h                              # Flash storage abstraction
  flash_storage.cpp                            # Implementation with CRC32
  data_collector.h                             # Data collection state machine
  data_collector.cpp                           # Collection logic

tools/
  download_data.py                             # PC-side download script
  parse_capture.py                             # Visualization and analysis
  README.md                                    # Tools documentation
```

### Files Modified

```
CMakeLists.txt                                 # Added new source files and hardware_flash library
```

## Key Features

✅ **Storage:** 10 seconds at 5kHz = 97.66 KB (fits in RAM and flash)  
✅ **File System:** Direct flash access with header + CRC32 checksum  
✅ **Capacity:** Store up to 10 captures (1MB flash partition)  
✅ **Download:** USB serial protocol with binary transfer  
✅ **Verification:** CRC32 checksums for data integrity  
✅ **Analysis:** Python tools for parsing and visualization  

## Architecture

### Data Collection Flow

```
User: COLLECT 10
  ↓
Allocate 100KB RAM buffer
  ↓
DMA fills buffers → Copy to collection buffer
  ↓
Progress: 25% ... 50% ... 75% ... 100%
  ↓
Write to flash (slot 0-9)
  ↓
Free RAM buffer
  ↓
Ready for download
```

### Download Flow

```
User: python download_data.py /dev/ttyACM0 download 0
  ↓
PC → Pico: "DOWNLOAD 0\n"
  ↓
Pico → PC: "START 100024\n"
  ↓
Pico → PC: <binary data: 24 byte header + 100KB samples>
  ↓
Pico → PC: "END\n"
  ↓
PC saves to file
  ↓
python parse_capture.py capture_00000.bin
  ↓
Generate CSV + plot + statistics
```

## File Format

Binary format with structured header:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | Magic | 0x41444353 ("ADCS") |
| 0x04 | 4 | Version | 1 |
| 0x08 | 4 | Sample Rate | 5000 Hz |
| 0x0C | 4 | Sample Count | 50,000 |
| 0x10 | 4 | Timestamp | Uptime in ms |
| 0x14 | 4 | Checksum | CRC32 of samples |
| 0x18 | 100KB | Samples | uint16_t[50000] |

**Total:** 100,024 bytes

## Serial Protocol

### Commands

- `COLLECT <seconds>` - Start collection (1-60 seconds)
- `LIST` - List stored captures
- `DOWNLOAD <slot>` - Download capture (0-9)
- `DELETE <slot>` - Delete capture
- `HELP` - Show help

### Example Session

```
> COLLECT 10
Starting 10 second collection...
Collection started
...
Progress: 100%
Saved to slot 0

> LIST
Stored captures:
Slot 0: 50000 samples, timestamp: 123456 ms

> DOWNLOAD 0
START 100024
<binary data>
END
```

## Integration Steps

To integrate into main.cpp:

1. **Add includes:**
   ```cpp
   #include "flash_storage.h"
   #include "data_collector.h"
   ```

2. **Add global objects:**
   ```cpp
   static DataCollector g_data_collector;
   ```

3. **Initialize flash in core1_main():**
   ```cpp
   FlashStorage::init();
   ```

4. **Feed DMA buffers to collector:**
   ```cpp
   if (g_data_collector.is_collecting()) {
       g_data_collector.process_buffer(buffer, size);
   }
   ```

5. **Add serial command handler:**
   ```cpp
   check_serial_input();  // Call in main loop
   ```

See `docs/planning/data-collection-integration-example.cpp` for complete code.

## Memory Usage

### During Collection

| Component | Size | Notes |
|-----------|------|-------|
| Collection buffer | 100 KB | Temporary |
| DMA buffers | 2 KB | Always allocated |
| Display framebuffer | 2 KB | Always allocated |
| Filter state | 28 bytes | Always allocated |
| **Total** | **~104 KB** | **39% of RAM** |

### Flash Storage

| Component | Size | Location |
|-----------|------|----------|
| Program binary | ~150 KB | 0x00000000 |
| Data partition | 1 MB | 0x00100000 |
| - Capture slot 0 | 128 KB | Per slot |
| - Capture slot 1-9 | 128 KB each | Up to 10 total |

## Python Tools

### download_data.py

Downloads captures from Pico to PC.

```bash
# List captures
python download_data.py /dev/ttyACM0 list

# Download slot 0
python download_data.py /dev/ttyACM0 download 0

# Delete slot 0
python download_data.py /dev/ttyACM0 delete 0
```

### parse_capture.py

Analyzes and visualizes downloaded data.

```bash
python parse_capture.py capture_00000.bin
```

**Output:**
- CSV file with all samples
- PNG plot (full view + zoomed 500ms)
- Statistics (min/max/mean voltage)
- Simple shot detection

## Testing Checklist

### Software Tests

- [ ] Compile without errors
- [ ] Flash writes succeed
- [ ] Flash reads match writes
- [ ] CRC32 verification works
- [ ] Serial commands parse correctly
- [ ] Binary download completes
- [ ] Python scripts parse files

### Hardware Tests

- [ ] Collect 10 seconds successfully
- [ ] No DMA buffer overflows during collection
- [ ] Data persists after power cycle
- [ ] Download completes without errors
- [ ] Plot shows expected voltage traces
- [ ] Collect semi-auto shots
- [ ] Collect full-auto burst
- [ ] Verify shot signatures visible

## Usage Workflow

### Step 1: Collect Data on Pico

Connect to Pico via serial terminal (115200 baud):

```
> COLLECT 10
Starting 10 second collection...
Progress: 100%
Saved to slot 0
```

### Step 2: Download to PC

```bash
python tools/download_data.py /dev/ttyACM0 download 0
```

### Step 3: Analyze

```bash
python tools/parse_capture.py capture_00000.bin
```

Opens plot window and creates CSV file.

### Step 4: Use Data

The captured data can now be used to:
- Develop shot detection algorithms
- Tune filter parameters
- Validate voltage drop characteristics
- Calculate optimal thresholds
- Test different guns/batteries

## Next Steps

1. **Integrate into main.cpp**
   - Add includes and globals
   - Initialize flash storage
   - Add serial command handler
   - Feed DMA buffers to collector
   - Optional: Add display UI

2. **Test on Hardware**
   - Flash to Pico
   - Verify flash writes
   - Collect test data
   - Download and verify

3. **Collect Real Data**
   - Connect to airsoft gun
   - Collect idle periods (baseline)
   - Collect semi-auto shots
   - Collect full-auto bursts
   - Download and analyze

4. **Develop Shot Detection**
   - Analyze captured voltage traces
   - Identify shot signatures
   - Determine optimal thresholds
   - Implement detection algorithm
   - Validate against collected data

## Success Criteria

Implementation is successful if:

- ✅ Can trigger 10-second collection via serial
- ✅ Collects exactly 50,000 samples at 5kHz
- ✅ No DMA overflows during collection
- ✅ Data written to flash successfully
- ✅ Data persists after power cycle
- ✅ Can download via USB serial
- ✅ Downloaded data has correct format
- ✅ Checksums verify correctly
- ✅ Python tools parse and plot data
- ✅ Shot events visible in captured data

## References

- **Design Doc:** `docs/planning/2025-11-06-data-collection-plan.md`
- **Integration Example:** `docs/planning/data-collection-integration-example.cpp`
- **Tools README:** `tools/README.md`
- **Flash API:** Pico SDK `hardware/flash.h`
- **DMA Sampler:** `lib/dma_adc_sampler.h`

---

**Implementation Status:** Complete and ready for hardware testing.

The system is fully implemented and ready to be integrated into main.cpp. All infrastructure is in place for collecting real-world shot detection data.
