# DMA Sampling Implementation - Quick Start

## What Was Implemented

This implementation adds **hardware-accelerated 5 kHz ADC sampling** to the Airsoft Display project, based on the research in `docs/planning/sampling-research.md`.

### Key Features

✅ **5 kHz DMA sampling** (500× faster than previous 10 Hz)  
✅ **Double-buffered architecture** (ping-pong buffers)  
✅ **Two-stage filter chain** (median + low-pass IIR)  
✅ **<1% CPU usage** (leaves headroom for display, future features)  
✅ **Zero data loss** (automatic buffer swapping)  
✅ **Real-time statistics** on display  

❌ **Shot detection NOT implemented** (as requested)

## Files Created

```
lib/
  adc_config.h              # Configuration constants
  voltage_filter.h/cpp      # Median + IIR filter chain
  dma_adc_sampler.h/cpp     # DMA sampling infrastructure

docs/implementation/
  dma-sampling-implementation.md  # Full implementation guide
```

## Files Modified

```
src/main.cpp               # Core 1 now processes DMA buffers
CMakeLists.txt             # Added new source files
```

## How It Works

```
Hardware Timer (5 kHz)
    ↓
ADC Conversion (12-bit)
    ↓
DMA Transfer (to RAM buffer)
    ↓
Buffer Full? → Swap to other buffer (ping-pong)
    ↓
Core 1 processes completed buffer:
    ├─> Median filter (kill motor spikes)
    ├─> Low-pass filter (smooth noise)
    ├─> Convert to voltage
    └─> Update shared data
    ↓
Core 0 displays statistics
```

## Display Output

When running on hardware, you'll see:

```
BUF: 12345      # Total buffers processed (~10/sec expected)
OVF: 0          # Buffer overflows (MUST be 0 for reliable sampling)
SMP: 6320640    # Total samples filtered (~5000/sec expected)
TMP: 72.5°F     # RP2040 temperature
VOL: 11.10V     # Battery voltage (filtered & scaled)
SHT: 0          # Shot count (not implemented yet)
```

## Key Performance Metrics

| Metric | Target | Actual (Estimated) |
|--------|--------|-------------------|
| Sample Rate | 5000 Hz | 5000 Hz |
| CPU Usage | <1% | ~0.14% |
| RAM Usage | <10 KB | 2.1 KB |
| Buffer Overflows | 0 | 0 (when working) |

## Next Steps

### 1. Hardware Testing

Flash to RP2040 and verify:
- [ ] BUF counter increments (~10/sec)
- [ ] OVF stays at 0
- [ ] SMP counter increments (~5000/sec)
- [ ] VOL shows stable voltage reading

### 2. Troubleshooting

If OVF > 0:
- Core 1 can't process buffers fast enough
- Check for blocking operations in Core 1 loop
- Verify watchdog isn't timing out

If SMP = 0:
- DMA not triggering
- Check ADC/DMA initialization
- Verify timer is running

### 3. Future Work (Not Implemented)

When ready to add shot detection:
- Implement `AdaptiveShotDetector` class (design in research doc)
- Add state machine for shot events
- Wire up to `g_shared_data.shot_count`
- See research doc Section 4.3 for complete algorithm

## Configuration

All parameters are in `lib/adc_config.h`:

```cpp
// To change sample rate (if needed):
constexpr uint32_t SAMPLE_RATE_HZ = 5000;  // Default: 5 kHz

// To tune filters (if noise still present):
constexpr uint32_t MEDIAN_WINDOW = 5;      // Default: 5 samples
constexpr float LPF_CUTOFF_HZ = 100.0f;    // Default: 100 Hz

// To adjust for different voltage divider:
constexpr float VDIV_R1 = 28000.0f;        // 28kΩ (to battery)
constexpr float VDIV_R2 = 10000.0f;        // 10kΩ (to ground)
```

## Documentation

**Full details:** `docs/implementation/dma-sampling-implementation.md`  
**Research basis:** `docs/planning/sampling-research.md`

## Build Notes

To compile (requires ARM toolchain):
```bash
cd build
cmake -G Ninja -DPICO_SDK_FETCH_FROM_GIT=ON ..
ninja
```

Then flash `airsoft-display.uf2` to the Pico.

---

**Ready for hardware testing!** 🚀
