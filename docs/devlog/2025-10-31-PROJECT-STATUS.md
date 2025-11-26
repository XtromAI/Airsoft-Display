# Airsoft Display - Project Status

**Last Updated:** October 31, 2025  
**Branch:** copilot/implement-sampling-method

## Overview

This project implements a shot counter and battery monitor for an Airsoft gun using a Raspberry Pi Pico (RP2040) with a custom SH1107 OLED display driver. The system uses dual cores for parallel data acquisition and display rendering.

## Current Implementation Status

### ✅ Completed Features

#### Display System
- **Custom SH1107 Driver** (`lib/sh1107-pico/`) - Built from scratch instead of using U8g2
  - Full graphics primitives: pixels, lines, rectangles, circles, triangles
  - Hardware SPI communication via SPI1 (GP14=SCK, GP15=MOSI)
  - 128x128 resolution with full framebuffer (2KB RAM)
  - Display rotation and inversion support
  
- **Font Rendering**
  - Custom 8x8 bitmap font (`font8x8.cpp`)
  - Custom 16x16 bitmap font (`font16x16.cpp`)
  - Configurable character spacing
  - String rendering with automatic positioning

- **Demo Capabilities**
  - Wave animation demo (`wave_demo.cpp`)
  - Spinning triangle demo (`sh1107_demo.cpp`)

#### System Architecture
- **Dual-Core Operation**
  - Core 0: Display and UI rendering (~60Hz update rate)
  - Core 1: Data acquisition and processing
  - Mutex-protected shared data structure

- **Hardware Integration**
  - DMA-based ADC sampling pipeline (`dma_adc_sampler.cpp`) paced by hardware alarms at 5 kHz
  - Voltage filtering chain (`voltage_filter.cpp`) combining 5-tap median and 1st-order low-pass stages
  - Watchdog timer for system recovery
  - Status LED feedback (onboard LED on GP25)

#### Pin Configuration
| Component | Signal | Pin | GPIO |
|-----------|--------|-----|------|
| Display (SH1107) | SCK | Pin 19 | GP14 |
| | MOSI | Pin 20 | GP15 |
| | DC | Pin 27 | GP21 |
| | CS | Pin 17 | GP13 |
| | RST | Pin 26 | GP20 |
| Battery Monitor | ADC | Pin 31 | GP26 (ADC0) |
| Status LED | LED | Pin 30 | GP25 |

### 🚧 In Progress / Partially Complete

- Shot detection algorithm using voltage dip detection windows
- Button input handling for shot counter reset (GP9/GP10)
- UX polish for display (voltage history, shot counter presentation)

### ❌ Not Yet Implemented

#### Core Functionality (High Priority)
1. **Shot Detection Algorithm**
   - Moving average calculation
   - Voltage dip detection
   - Configurable threshold
   - Shot counter increment logic
   - Reference: Design Guide Step 3

2. **Button Input**
   - Hardware: GP9 (output), GP10 (input with pull-up)
   - Short press: No action
   - Long press (3s): Reset shot counter
   - Reference: Design Guide Step 5

3. **Voltage Analytics & Alerts**
  - Add trend graph or min/max presentation
  - Integrate voltage alerts as thresholds are defined

#### Display Optimizations (Lower Priority)
From `docs/sh1107-update-plan.md`:
1. **PIO-based Display Transfer** - Use PIO for SPI to offload CPU
2. **DMA Display Streaming** - Stream framebuffer via DMA
3. **Dirty Region Tracking** - Only update changed areas
4. **Double Buffering** - Eliminate tearing in animations

## File Structure

```
src/
  main.cpp                 # Main application, Core 0 & Core 1 logic
lib/
  adc_sampler.cpp/.h       # Legacy timer-based ADC sampling (reference only)
  dma_adc_sampler.cpp/.h   # Active DMA-based ADC sampling pipeline
  voltage_filter.cpp/.h    # Voltage filtering pipeline
  sh1107-pico/src/
    sh1107_driver.cpp/.h   # Custom SH1107 display driver
    font8x8.cpp/.h         # 8x8 bitmap font
    font16x16.cpp/.h       # 16x16 bitmap font
    sh1107_demo.cpp/.h     # Demo animations
    wave_demo.cpp/.h       # Wave animation
docs/planning/
  2025-01-15-design-guide.md          # Original design specification
  2025-08-06-sh1107-update-plan.md    # Display optimization roadmap
  2025-10-31-PROJECT-STATUS.md        # This file
```

## Key Deviations from Original Design

1. **Display Library:** Built custom SH1107 driver instead of using U8g2
   - **Why:** U8g2 integration failed (only produced display noise/static)
   - **Solution:** Custom driver based on MicroPython reference code
   - **Outcome:** Working display, better control, optimized for specific use case
   - **History:** See `docs/archive/` for original U8g2 research and troubleshooting attempts

2. **Sampling Rate:** Upgraded from 10 Hz timer sampling to 5 kHz DMA sampling
  - **Why:** Required for reliable shot detection
  - **Approach:** Dual-buffer DMA with watchdog-safe hardware alarm triggers

## Next Steps (Recommended Priority Order)

### Immediate (Core Functionality)
1. Implement shot detection logic
2. Add button input handling
3. Integrate shot counter UI elements

### Short Term (Polish)
4. Expand voltage analytics/graphing
5. Run calibration passes on live hardware
6. Define watchdog/health indicators for release builds

### Long Term (Optimization)
7. Display optimizations (PIO, DMA streaming, dirty regions)
8. Power optimization (sleep modes, display idle states)
9. User configuration (serial/OTA thresholds)

## Build & Flash

```bash
# Compile (use task or manual)
ninja -C build

# Flash (if using debugger)
# Task: "Flash"

# Monitor (USB CDC serial + on-screen diagnostics)
```

## Notes

- Serial output enabled over USB CDC (`pico_enable_stdio_usb` = 1)
- Debugging uses on-screen metrics and periodic USB logs
- Watchdog enabled with 2-second timeout
- Display updates at ~60Hz via timer interrupt
