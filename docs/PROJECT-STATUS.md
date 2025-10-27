# Airsoft Display - Project Status

**Last Updated:** October 26, 2025  
**Branch:** feature/sampler

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
  - Temperature monitoring (`temperature.cpp`) with Fahrenheit/Celsius support
  - ADC sampling framework (`adc_sampler.cpp`) using hardware timers
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

#### ADC Sampling
- **Current State:** Timer-based sampling at 10Hz (testing rate)
- **Next Steps:** Implement DMA-based high-frequency sampling (≥1kHz)
- **Files:** `lib/adc_sampler.cpp`, `src/main.cpp`

### ❌ Not Yet Implemented

#### Core Functionality (High Priority)
1. **DMA-based ADC Sampling**
   - Replace timer callbacks with DMA circular buffer
   - Target: ≥1kHz sampling rate
   - Reference: Design Guide Step 2-3

2. **Shot Detection Algorithm**
   - Moving average calculation
   - Voltage dip detection
   - Configurable threshold
   - Shot counter increment logic
   - Reference: Design Guide Step 3

3. **Button Input**
   - Hardware: GP9 (output), GP10 (input with pull-up)
   - Short press: No action
   - Long press (3s): Reset shot counter
   - Reference: Design Guide Step 5

4. **Voltage Conversion & Display**
   - Convert ADC to actual battery voltage (accounting for voltage divider)
   - Display formatted voltage on screen
   - Optional: Voltage trend graph

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
  adc_sampler.cpp/.h       # Timer-based ADC sampling (needs DMA upgrade)
  temperature.cpp/.h       # Temperature sensor wrapper
  sh1107-pico/src/
    sh1107_driver.cpp/.h   # Custom SH1107 display driver
    font8x8.cpp/.h         # 8x8 bitmap font
    font16x16.cpp/.h       # 16x16 bitmap font
    sh1107_demo.cpp/.h     # Demo animations
    wave_demo.cpp/.h       # Wave animation
docs/
  design-guide.md          # Original design specification
  sh1107-update-plan.md    # Display optimization roadmap
  PROJECT-STATUS.md        # This file
```

## Key Deviations from Original Design

1. **Display Library:** Built custom SH1107 driver instead of using U8g2
   - **Why:** U8g2 integration failed (only produced display noise/static)
   - **Solution:** Custom driver based on MicroPython reference code
   - **Outcome:** Working display, better control, optimized for specific use case
   - **History:** See `docs/archive/` for original U8g2 research and troubleshooting attempts

2. **Sampling Rate:** Currently at 10Hz instead of ≥1kHz
   - **Why:** Testing/development phase
   - **Next:** Implement DMA-based sampling for high frequency

## Next Steps (Recommended Priority Order)

### Immediate (Core Functionality)
1. **Implement DMA-based ADC sampling** - Critical for shot detection
2. **Add moving average calculation** - Foundation for shot detection
3. **Implement shot detection logic** - Core feature
4. **Add button input handling** - User interaction

### Short Term (Polish)
5. **Voltage display formatting** - User-facing feature
6. **Shot counter display** - User-facing feature
7. **Testing & calibration** - Verify shot detection accuracy

### Long Term (Optimization)
8. **Display optimizations** - PIO, DMA, dirty regions (if needed)
9. **Power optimization** - Sleep modes, display off when idle
10. **User configuration** - Serial commands for threshold adjustment

## Build & Flash

```bash
# Compile (use task or manual)
ninja -C build

# Flash (if using debugger)
# Task: "Flash"

# Monitor (debugging via display, no serial output currently)
```

## Notes

- Serial output currently disabled (`pico_enable_stdio_usb` = 0)
- Debugging uses on-screen display per copilot-instructions.md
- Watchdog enabled with 2-second timeout
- Display updates at ~60Hz via timer interrupt
