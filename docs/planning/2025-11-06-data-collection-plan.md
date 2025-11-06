# Data Collection Plan for 5kHz ADC Sampler

**Date:** 2025-11-06  
**Status:** Planning - Ready for Implementation  
**Purpose:** Collect real-world shot detection data for algorithm development

## Overview

This document outlines a plan to collect up to 10 seconds of raw ADC samples from the 5kHz DMA sampler and save them to a file that can be downloaded from the Pico. This data will be used for developing and validating shot detection algorithms.

## Requirements

1. **Collect 10 seconds of samples** at 5kHz (50,000 samples)
2. **Save to a file** on the Pico's flash storage
3. **Download via USB** for offline analysis
4. **Minimal impact** on existing functionality
5. **User-triggered** data collection (not continuous)

## Feasibility Analysis

### Storage Requirements

```
Sample rate:       5,000 Hz
Duration:          10 seconds
Total samples:     50,000
Bytes per sample:  2 (uint16_t)
Total size:        100,000 bytes (97.66 KB)
```

### Hardware Constraints

| Resource | Available | Required | Feasible? |
|----------|-----------|----------|-----------|
| **RAM** | 264 KB | 97.66 KB (37%) | ✅ Yes |
| **Flash** | ~1.8 MB | 0.095 MB (5%) | ✅ Yes |
| **DMA Sampler** | 5 kHz | 5 kHz | ✅ Yes |

**Conclusion:** 10 seconds of data can easily fit in RAM and flash.

## Implementation Approach

### Option 1: RAM Buffer + Batch Write (Recommended)

**Architecture:**
```
┌─────────────────────────────────────────────────────────┐
│ User triggers collection via button or serial command   │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│ Allocate 100KB RAM buffer (static or malloc)            │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│ DMA sampler fills buffers (512 samples × 98 buffers)    │
│ Copy each buffer to collection buffer                   │
│ Progress: 0% ... 50% ... 100% (display on screen)      │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│ Write entire buffer to flash file in one operation      │
│ File: "/data/capture_XXXXX.bin" (XXXXX = counter)      │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│ Download file via USB serial or USB mass storage        │
└─────────────────────────────────────────────────────────┘
```

**Pros:**
- Simple implementation
- Fast collection (no flash write overhead)
- No risk of missed samples during collection
- Single atomic write to flash

**Cons:**
- Uses ~38% of RAM during collection
- Cannot collect more than 10 seconds without flash streaming

### Option 2: Streaming to Flash (Not Recommended)

**Why not:**
- Flash writes are slow (blocks DMA processing)
- Risk of buffer overflow during write operations
- More complex state management
- Not needed for 10-second requirement

## File System Integration

### LittleFS for Pico SDK

We need to integrate LittleFS library into the project:

**Repository:** https://github.com/littlefs-project/littlefs

**Integration Steps:**
1. Add LittleFS as a Git submodule or copy to `lib/littlefs/`
2. Create flash driver adapter for RP2040 hardware flash
3. Add CMake configuration for LittleFS
4. Implement file I/O wrappers

**Flash Partitioning:**
```
┌──────────────────────────────────────┐
│ 0x00000000 - 0x001FFFFF (2 MB)       │
├──────────────────────────────────────┤
│ 0x00000000 - ~0x00030000 (~200 KB)   │  Program binary (.uf2)
│                                      │
├──────────────────────────────────────┤
│ 0x00100000 - 0x001FFFFF (1 MB)       │  LittleFS partition
│                                      │  (data files)
└──────────────────────────────────────┘
```

**Key Functions Needed:**
```cpp
// Initialize file system
bool fs_init();

// Create new data file
bool fs_create_capture_file(const char* filename);

// Write data to file
bool fs_write_samples(const uint16_t* buffer, uint32_t count);

// List files (for debugging)
void fs_list_files();

// Read file for download
bool fs_read_file(const char* filename, uint8_t* buffer, uint32_t size);
```

### Alternative: Direct Flash Access (Simpler, but less robust)

For a quick implementation, we can write directly to flash without a file system:

**Approach:**
```cpp
#include "hardware/flash.h"
#include "hardware/sync.h"

// Reserve last 1MB of flash for data
#define DATA_FLASH_OFFSET (1 * 1024 * 1024)  // 1MB offset
#define DATA_FLASH_SIZE (1 * 1024 * 1024)     // 1MB size

void write_samples_to_flash(const uint16_t* buffer, uint32_t count) {
    // Flash writes must be aligned and sector-erased
    uint32_t interrupts = save_and_disable_interrupts();
    
    // Erase sectors (4KB each)
    flash_range_erase(DATA_FLASH_OFFSET, count * 2);
    
    // Write data (256 bytes at a time)
    flash_range_program(DATA_FLASH_OFFSET, (const uint8_t*)buffer, count * 2);
    
    restore_interrupts(interrupts);
}

const uint16_t* read_samples_from_flash() {
    // Flash is memory-mapped starting at XIP_BASE
    return (const uint16_t*)(XIP_BASE + DATA_FLASH_OFFSET);
}
```

**Pros:**
- No external dependencies
- Simple implementation
- Direct memory-mapped read

**Cons:**
- No file management
- No wear leveling
- Must erase before write
- Only one "file" at a time

**Recommendation:** Start with direct flash access for MVP, migrate to LittleFS later if needed.

## Data Collection State Machine

```cpp
enum class CollectionState {
    IDLE,           // Not collecting
    PREPARING,      // Allocating buffer
    COLLECTING,     // Actively collecting samples
    WRITING_FLASH,  // Writing to flash
    COMPLETE,       // Ready for download
    ERROR           // Collection failed
};

class DataCollector {
private:
    CollectionState state;
    uint16_t* collection_buffer;  // 50,000 samples
    uint32_t samples_collected;
    uint32_t target_samples;
    uint32_t capture_number;      // Auto-increment for filenames
    
public:
    // Start collection
    bool start_collection(uint32_t duration_ms);
    
    // Called by Core 1 when DMA buffer ready
    void process_buffer(const uint16_t* samples, uint32_t count);
    
    // Write to flash when collection complete
    bool finalize_collection();
    
    // Get current progress (0-100%)
    uint8_t get_progress() const;
    
    // Get current state
    CollectionState get_state() const;
};
```

## User Interface

### Trigger Mechanisms

**Option 1: Button Input**
- Hold button for 3 seconds to start collection
- LED blinks during collection
- Display shows progress bar

**Option 2: Serial Command**
- Send "COLLECT 10" via USB serial
- Response: "Starting 10s collection..."
- Progress updates: "25%... 50%... 75%... 100%"
- Completion: "Saved to capture_00001.bin"

**Option 3: Both**
- Implement both for flexibility
- Serial for development, button for field use

### Display Feedback

```
┌──────────────────────┐
│  DATA COLLECTION     │
│                      │
│  Progress: 65%       │
│  ████████████░░░░    │
│                      │
│  Samples: 32500      │
│  Target:  50000      │
│                      │
│  Press button to     │
│  cancel              │
└──────────────────────┘
```

## Download Mechanism

### Option 1: USB Serial Download (Recommended for MVP)

**Protocol:**
```
User: LIST
Pico: capture_00001.bin 100000
      capture_00002.bin 100000

User: DOWNLOAD capture_00001.bin
Pico: START 100000
      <binary data...>
      END

User: DELETE capture_00001.bin
Pico: OK
```

**Implementation:**
```cpp
void handle_serial_command(const char* cmd) {
    if (strcmp(cmd, "LIST") == 0) {
        list_captures();
    } else if (strncmp(cmd, "DOWNLOAD ", 9) == 0) {
        download_capture(cmd + 9);
    } else if (strncmp(cmd, "DELETE ", 7) == 0) {
        delete_capture(cmd + 7);
    }
}

void download_capture(const char* filename) {
    const uint16_t* data = read_samples_from_flash();
    printf("START 100000\n");
    
    // Send binary data (could also encode as hex or base64)
    fwrite(data, sizeof(uint16_t), 50000, stdout);
    fflush(stdout);
    
    printf("END\n");
}
```

**Receiving on PC:**
```python
import serial
import struct

ser = serial.Serial('/dev/ttyACM0', 115200)

# Request download
ser.write(b'DOWNLOAD capture_00001.bin\n')

# Read until START
while True:
    line = ser.readline().decode().strip()
    if line.startswith('START'):
        size = int(line.split()[1])
        break

# Read binary data
data = ser.read(size)

# Parse uint16_t samples
samples = struct.unpack(f'<{size//2}H', data)

# Save to file
with open('capture_00001.bin', 'wb') as f:
    f.write(data)

# Wait for END
assert ser.readline().decode().strip() == 'END'

print(f'Downloaded {len(samples)} samples')
```

### Option 2: USB Mass Storage (Future Enhancement)

Mount Pico as USB drive, drag-and-drop files. Requires TinyUSB MSC integration.

**Pros:**
- No custom software needed
- User-friendly
- Works on any OS

**Cons:**
- Complex implementation
- Must unmount filesystem during access
- Potential for corruption if improperly ejected

**Recommendation:** Implement serial download first, add MSC later if needed.

## File Format

### Binary Format (Recommended)

**Structure:**
```
Offset | Size | Field          | Description
-------|------|----------------|---------------------------
0x00   | 4    | Magic          | 0x41444353 ("ADCS")
0x04   | 4    | Version        | 0x00000001
0x08   | 4    | Sample Rate    | 5000 (Hz)
0x0C   | 4    | Sample Count   | 50000
0x10   | 4    | Timestamp      | Unix epoch (optional)
0x14   | 4    | Checksum       | CRC32 of data
0x18   | ?    | Samples        | uint16_t[50000]
```

**Total size:** 24 bytes header + 100,000 bytes data = 100,024 bytes

**Parsing:**
```python
import struct

with open('capture.bin', 'rb') as f:
    header = f.read(24)
    magic, version, rate, count, timestamp, checksum = struct.unpack('<IIIIII', header)
    
    assert magic == 0x41444353, "Invalid magic number"
    assert version == 1, "Unsupported version"
    
    data = f.read(count * 2)
    samples = struct.unpack(f'<{count}H', data)
```

### CSV Format (Alternative, for quick inspection)

**Structure:**
```csv
sample_number,adc_raw,voltage_mv,timestamp_us
0,2048,1650.0,0
1,2047,1649.2,200
2,2046,1648.4,400
...
```

**Pros:**
- Human-readable
- Easy to open in Excel/LibreOffice
- Simple to parse

**Cons:**
- Much larger (10× size increase)
- Slower to write
- Not suitable for 50,000 samples (would be 1-2 MB)

**Recommendation:** Use binary format. Provide Python script to convert to CSV if needed.

## Implementation Files

### New Files to Create

```
lib/
  data_collector.h              # Data collection state machine
  data_collector.cpp            # Implementation
  flash_storage.h               # Flash I/O abstraction
  flash_storage.cpp             # Direct flash write/read
  
  littlefs/                     # Optional: LittleFS integration
    lfs.c                       # LittleFS library
    lfs.h
    lfs_util.c
    lfs_util.h
    rp2040_flash_driver.c       # Hardware abstraction

tools/
  download_data.py              # PC-side download script
  parse_capture.py              # Convert binary to CSV/plot
```

### Modified Files

```
src/main.cpp                    # Add collection mode
CMakeLists.txt                  # Link flash/filesystem libraries
```

## Implementation Plan

### Phase 1: Direct Flash Storage (MVP)

- [ ] Create `flash_storage.h/cpp` for direct flash access
- [ ] Create `data_collector.h/cpp` state machine
- [ ] Add serial command handler ("COLLECT", "LIST", "DOWNLOAD")
- [ ] Integrate into Core 1 main loop
- [ ] Add display UI for collection progress
- [ ] Test 10-second collection and flash write
- [ ] Create Python download script
- [ ] Verify data integrity (checksums)

**Estimated effort:** 4-6 hours  
**Testing required:** Hardware testing with actual Pico

### Phase 2: User Interface Enhancement

- [ ] Add button trigger for collection
- [ ] Add LED status indicator
- [ ] Improve display feedback (progress bar, animations)
- [ ] Add collection metadata (timestamp, capture number)
- [ ] Add DELETE command to clear old captures

**Estimated effort:** 2-3 hours  
**Testing required:** Hardware testing

### Phase 3: LittleFS Integration (Optional)

- [ ] Integrate LittleFS library
- [ ] Create RP2040 flash driver
- [ ] Port data collector to use LittleFS API
- [ ] Add wear leveling and filesystem checks
- [ ] Support multiple captures with filenames

**Estimated effort:** 6-8 hours  
**Testing required:** Extensive flash wear testing

### Phase 4: USB Mass Storage (Future)

- [ ] Integrate TinyUSB MSC
- [ ] Mount LittleFS partition as USB drive
- [ ] Handle mount/unmount safely
- [ ] Test on Windows/Mac/Linux

**Estimated effort:** 8-12 hours  
**Testing required:** Cross-platform testing

## Testing Strategy

### Unit Tests

1. **Flash Write/Read**
   - Write test pattern to flash
   - Read back and verify
   - Test boundary conditions

2. **Data Collector**
   - Test state transitions
   - Test buffer management
   - Test progress calculation

3. **Serial Protocol**
   - Test command parsing
   - Test download integrity
   - Test error handling

### Integration Tests

1. **10-Second Collection**
   - Trigger collection
   - Verify 50,000 samples collected
   - Verify no buffer overflows
   - Verify flash write successful

2. **Download Verification**
   - Download captured data
   - Verify sample count
   - Verify checksum
   - Plot data to verify shape

3. **Multiple Captures**
   - Collect 3× 10-second captures
   - Verify all saved correctly
   - Verify auto-incrementing filenames
   - Verify flash space management

### Hardware Tests

1. **Shot Detection Data**
   - Connect to live airsoft gun
   - Fire single shots (semi-auto)
   - Collect 10-second burst
   - Verify voltage drops visible in data

2. **Full-Auto Burst**
   - Fire full-auto burst
   - Collect 10-second sample
   - Verify individual shots distinguishable
   - Calculate actual ROF from data

3. **Power Cycle**
   - Collect data
   - Power off Pico
   - Power on and list files
   - Verify data persisted

## Data Analysis Workflow

Once data is collected, the workflow is:

1. **Collect data on Pico**
   ```
   User: COLLECT 10
   Pico: Collecting... 100%
   Pico: Saved to capture_00001.bin
   ```

2. **Download to PC**
   ```bash
   python tools/download_data.py /dev/ttyACM0 capture_00001.bin
   ```

3. **Parse and visualize**
   ```bash
   python tools/parse_capture.py capture_00001.bin
   ```

4. **Analyze in Python**
   ```python
   import numpy as np
   import matplotlib.pyplot as plt
   
   # Load data
   samples = load_capture('capture_00001.bin')
   
   # Convert to voltage
   voltage = (samples / 4095.0) * 3.3 * 3.8
   
   # Plot
   plt.plot(voltage)
   plt.xlabel('Sample (@ 5kHz)')
   plt.ylabel('Battery Voltage (V)')
   plt.title('Shot Detection Data')
   plt.show()
   
   # Detect shots (simple threshold)
   baseline = np.median(voltage)
   threshold = baseline * 0.8  # 20% drop
   shots = voltage < threshold
   
   print(f'Baseline: {baseline:.2f}V')
   print(f'Shots detected: {np.sum(np.diff(shots.astype(int)) > 0)}')
   ```

## Memory Management

### RAM Usage During Collection

```
Component                    Size        Notes
─────────────────────────────────────────────────────
Collection buffer            100 KB      Temporary
DMA buffers                  2 KB        Always allocated
Display framebuffer          2 KB        Always allocated
Filter state                 0.03 KB     Always allocated
Stack/heap overhead          ~10 KB      Estimated
─────────────────────────────────────────────────────
Total during collection      ~114 KB     43% of 264 KB RAM
```

**Safety margin:** 150 KB remaining for other operations

### Flash Usage

```
Component                    Size        Location
─────────────────────────────────────────────────────
Program binary               ~150 KB     0x00000000
Data partition (LittleFS)    1 MB        0x00100000
  - capture_00001.bin        100 KB
  - capture_00002.bin        100 KB
  - ... up to 10 captures
─────────────────────────────────────────────────────
Total                        ~350 KB     Of 2 MB flash
```

**Capacity:** Can store ~10 captures before needing cleanup

## Potential Issues and Mitigations

### Issue 1: DMA Buffer Overflow During Collection

**Problem:** Core 1 too busy copying to collection buffer, misses DMA buffers

**Mitigation:**
- Use `memcpy()` for fast buffer copy
- Increase DMA buffer processing priority
- Monitor overflow counter during collection
- Abort collection if overflow detected

### Issue 2: Flash Write Blocking

**Problem:** Flash write takes long time, system appears frozen

**Mitigation:**
- Display "Writing to flash..." message
- Flash write is atomic, happens once at end
- Consider watchdog pause during write

### Issue 3: Flash Wear

**Problem:** Repeated collections wear out flash

**Mitigation:**
- LittleFS provides wear leveling
- Flash rated for 100,000 erase cycles
- Typical use: < 100 collections = minimal wear
- Not a concern for development/testing phase

### Issue 4: Data Corruption

**Problem:** Power loss during flash write

**Mitigation:**
- Add CRC32 checksum to file header
- Verify checksum on read
- Display warning if file corrupted
- User must re-collect data

## Success Criteria

The implementation is successful if:

1. ✅ Can trigger 10-second collection via serial command
2. ✅ Collects exactly 50,000 samples at 5kHz
3. ✅ No DMA buffer overflows during collection
4. ✅ Data written to flash successfully
5. ✅ Data persists after power cycle
6. ✅ Can download via USB serial
7. ✅ Downloaded data has correct format and checksum
8. ✅ Can visualize shot events in captured data
9. ✅ Display shows collection progress
10. ✅ System remains responsive during collection

## Next Steps

1. **Implement MVP (Phase 1)**
   - Focus on getting data collection working
   - Use direct flash access (simplest)
   - Serial download only
   - Minimal UI

2. **Test with Real Gun**
   - Collect semi-auto shots
   - Collect full-auto bursts
   - Collect idle periods (no shots)
   - Verify data quality

3. **Analyze Data**
   - Plot voltage traces
   - Identify shot signatures
   - Calculate optimal thresholds
   - Develop shot detection algorithm

4. **Iterate**
   - Refine collection parameters if needed
   - Add UI improvements
   - Consider LittleFS if multiple captures needed
   - Document findings

## References

- **RP2040 Flash API:** `hardware/flash.h` in Pico SDK
- **LittleFS:** https://github.com/littlefs-project/littlefs
- **LittleFS on RP2040:** https://dominicmaas.co.nz/pages/pico-filesystem-littlefs
- **Pico Flash Tutorial:** https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_flash
- **DMA Sampler:** `lib/dma_adc_sampler.h` (existing implementation)
- **Research:** `docs/planning/2025-01-30-sampling-research.md`

---

**End of Document**
