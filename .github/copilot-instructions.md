# Copilot Instructions for Airsoft Display Project

## Project Overview

This is a Raspberry Pi Pico (RP2040) embedded C++ project that implements a shot counter and battery monitor for Airsoft guns. The system uses a custom SH1107 OLED display driver and dual-core architecture for real-time data acquisition and display rendering.

**Key Technologies:**
- **Platform:** Raspberry Pi Pico (RP2040 dual-core ARM Cortex-M0+)
- **Language:** C++17
- **Build System:** CMake with Pico SDK
- **Display:** SH1107 128x128 OLED (custom driver, SPI interface)
- **Hardware:** ADC sampling, DMA, hardware timers, dual-core synchronization

## Hardware Architecture

### Pin Configuration

| Component | Signal | Pico Pin | GPIO | Notes |
|-----------|--------|----------|------|-------|
| **SH1107 Display** | SCK (Clock) | Pin 19 | GP14 | SPI1 Clock |
| | MOSI (Data) | Pin 20 | GP15 | SPI1 MOSI |
| | DC (Data/Command) | Pin 27 | GP21 | Command/Data select |
| | CS (Chip Select) | Pin 17 | GP13 | SPI Chip Select |
| | RST (Reset) | Pin 26 | GP20 | Display Reset |
| | VCC | 3V3 | - | 3.3V Power |
| | GND | GND | - | Ground |
| **Button** | Terminal 1 | Pin 12 | GP9 | Output (drives low) |
| | Terminal 2 | Pin 14 | GP10 | Input (pull-up) |
| **Battery Monitor** | ADC Input | Pin 31 | GP26 | ADC0 |
| **Status LED** | Onboard LED | Pin 30 | GP25 | Built-in LED |

### Hardware Components

- **Microcontroller:** Raspberry Pi Pico (RP2040)
- **Display:** SH1107 128x128 OLED (SPI mode, 3.3V)
- **Battery:** Airsoft gun battery (11.1V 3S LiPo)
- **Voltage Regulator:** L7805CV (5V output, max 1A, input 7.5V-35V)
- **Voltage Divider:** 10kΩ and 28kΩ resistors (scales down battery voltage to ADC range, max 3.3V)
- **Power:** USB 5V during development; L7805CV voltage regulator (powered from battery balance connector) when deployed. Typical draw ~50mA (Pico + Display)

### Dual-Core Architecture

**Core 0 (Display & UI):**
- Display rendering and UI updates
- SPI communication with SH1107
- User input handling (button) - **Not yet implemented**
- Updates at ~60Hz via hardware timer interrupt
- Reads shared data via mutex

**Core 1 (Data Acquisition):**
- High-frequency ADC sampling (target ≥1kHz with DMA) - **Currently timer-based at 10Hz for development/testing; insufficient for shot detection which requires detecting millisecond-scale motor current spikes**
- Moving average calculation - **Not yet implemented**
- Shot detection (voltage dip monitoring) - **Not yet implemented; requires DMA upgrade to ≥1kHz first**
- Battery voltage processing
- Updates shared data via mutex
- Watchdog management

## RP2040 Hardware Features & Usage

This project leverages the RP2040's specialized hardware for responsive operation, efficient power consumption, and high sample rates.

### Hardware Timers
- **Display Updates:** Hardware timer triggers display refresh at ~60Hz (`add_repeating_timer_ms`)
- **ADC Sampling:** Currently uses hardware timer alarms (`add_alarm_in_us`) for periodic sampling
- **Benefits:** Precise, low-jitter timing; frees CPU for other tasks
- **APIs:** `hardware/timer.h` - `add_repeating_timer_ms()`, `add_alarm_in_us()`, `cancel_alarm()`

### DMA (Direct Memory Access)
- **Planned for ADC:** DMA circular buffer for continuous high-speed ADC sampling (≥1kHz)
  - Timer triggers ADC conversions
  - DMA automatically transfers ADC results to RAM buffer
  - CPU only processes completed batches via DMA interrupts
- **Potential for Display:** Could stream framebuffer to SPI peripheral (future optimization)
- **Benefits:** Offloads CPU, enables true background sampling, no missed samples
- **APIs:** `hardware/dma.h` - `dma_channel_config`, `dma_channel_transfer_*()`, DMA IRQ handlers
- **Current Status:** Not yet implemented; using timer-based approach at 10Hz for development

### PIO (Programmable I/O)
- **Potential Use:** Custom SPI protocol for display via PIO state machines
- **Benefits:** Offloads SPI signaling to dedicated hardware, frees CPU, precise timing control
- **APIs:** `hardware/pio.h` - `pio_sm_*()` functions, PIO assembly programs
- **Current Status:** Not implemented; using hardware SPI (SPI1) successfully
- **Future:** Optional optimization if CPU becomes bottleneck (see `docs/archive/sh1107-update-plan.md`)

### Dual-Core Processing
- **Core 0:** UI and display rendering (non-critical timing)
- **Core 1:** Time-critical data acquisition and shot detection
- **Synchronization:** `mutex_t` from `pico_sync` for thread-safe shared data access
- **Benefits:** True parallel processing; display rendering never blocks ADC sampling
- **APIs:** `pico/multicore.h` - `multicore_launch_core1()`, `pico/sync.h` - `mutex_t`, `mutex_enter_blocking()`

### Watchdog Timer
- **Purpose:** Automatic system recovery from crashes or hangs
- **Configuration:** 2-second timeout, enabled in `main()`, both cores must kick periodically
- **Implementation:** `watchdog_update()` called in main loops of both cores
- **Benefits:** Ensures system reliability; auto-reboot on failure
- **APIs:** `hardware/watchdog.h` - `watchdog_enable()`, `watchdog_update()`, `watchdog_caused_reboot()`

### ADC (Analog-to-Digital Converter)
- **Input:** GP26 (ADC0) connected to battery voltage divider
- **Configuration:** 12-bit resolution, free-running mode (`adc_run(true)`)
- **Sampling:** Currently timer-based; planned DMA upgrade for ≥1kHz continuous sampling
- **Benefits:** Built-in ADC with low noise, sufficient resolution for voltage monitoring
- **APIs:** `hardware/adc.h` - `adc_init()`, `adc_gpio_init()`, `adc_select_input()`, `adc_read()`

### SPI (Serial Peripheral Interface)
- **Instance:** SPI1 for SH1107 display
- **Pins:** GP14 (SCK), GP15 (MOSI), GP13 (CS), GP21 (DC), GP20 (RST)
- **Configuration:** Hardware SPI via `gpio_set_function(pin, GPIO_FUNC_SPI)`
- **Benefits:** Hardware-accelerated serial communication, efficient display updates
- **APIs:** `hardware/spi.h` - `spi_init()`, `spi_write_blocking()`

### Power Efficiency Considerations
- **Sleep States:** Use `tight_loop_contents()` when waiting (lower power than busy loops)
- **Display Contrast:** Configurable (`display.setContrast()`); affects power consumption
- **Future Optimizations:** 
  - Display sleep mode when idle
  - CPU frequency scaling if needed
  - PIO/DMA to reduce CPU wake time

### Memory Architecture
- **Framebuffer:** 2KB RAM for 128x128 monochrome display (organized as 128 columns × 16 pages, matching SH1107 memory layout)
- **DMA Buffers:** Circular buffers in SRAM for ADC samples
- **Stack Allocation:** Prefer stack over heap for embedded performance
- **Shared Data:** Minimal volatile structure protected by mutex

## Development Environment

### Build System

- **Do not** run the VS Code task "Run Project"
- **Always compile** the project after code changes to verify no errors
- Use CMake with Ninja generator
- Build command: `ninja -C build` or use VS Code CMake tasks

### Terminal

- Use **Windows PowerShell** commands in the terminal
- Cross-platform bash commands are acceptable when clearly portable

### Debugging

- **Serial output available:** USB serial is enabled (`pico_enable_stdio_usb` = 1 in CMakeLists.txt)
- **Display debugging:** On-screen debugging also available and useful for deployed systems
- Status LED (GP25) available for simple state indication

## Code Style and Conventions

### Header Comments

When adding section headers in code, use this format:

```cpp
// ==================================================
// Constructor & Destructor
// ==================================================
```

### Naming Conventions

- **Classes:** PascalCase (e.g., `SH1107_Display`, `Temperature`)
- **Functions/Methods:** camelCase (e.g., `drawChar`, `setPixel`)
- **Variables:** snake_case for locals and members (e.g., `cs_pin`, `current_voltage_mv`)
- **Constants:** UPPER_SNAKE_CASE for class constants and compile-time constants (e.g., `CONVERSION_FACTOR`, `PIN_SPI_SCK`, `DEFAULT_WIDTH`)
  - Note: Some legacy code uses kPascalCase (e.g., `kConversionFactor` in Temperature class); prefer UPPER_SNAKE_CASE for new code
- **Macros:** UPPER_SNAKE_CASE (e.g., `SH1107_DISPLAYON`)

### Code Organization

- Inline simple getters/setters in headers for performance
- Use `pragma once` for header guards
- Place hardware-specific constants with macros or `constexpr`
- Prefer `constexpr` over macros for type-safe constants
- Keep platform-specific code isolated and documented

### Comments

- Document hardware interfaces and timing-critical code
- Explain "why" not "what" for non-obvious logic
- Use inline comments sparingly; prefer self-documenting code
- Add TODO comments with context when deferring work

## File Structure

```
src/
  main.cpp                    # Main application, dual-core entry points
lib/
  adc_sampler.cpp/.h          # ADC sampling framework (timer/DMA-based)
  temperature.cpp/.h          # Temperature sensor wrapper
  sh1107-pico/src/
    sh1107_driver.cpp/.h      # Custom SH1107 display driver
    font8x8.cpp/.h            # 8x8 bitmap font
    font16x16.cpp/.h          # 16x16 bitmap font
    bitmap_font.h             # Font interface
    sh1107_demo.cpp/.h        # Demo animations
    wave_demo.cpp/.h          # Wave animation
docs/devlog/
  2025-01-15-design-guide.md             # Original design specification
  2025-10-31-PROJECT-STATUS.md           # Current implementation status
  *.md                                   # Additional documentation
CMakeLists.txt                # Build configuration
```

## Key Libraries and APIs

### Pico SDK Libraries

- `pico_stdlib` - Core functionality
- `hardware_spi` - SPI communication for display
- `hardware_dma` - DMA transfers
- `hardware_adc` - ADC sampling
- `hardware_timer` - Hardware timers
- `pico_multicore` - Dual-core support
- `pico_sync` - Mutex and synchronization primitives
- `hardware_watchdog` - Watchdog timer

### Custom Libraries

- **SH1107 Driver** (`lib/sh1107-pico/`) - Custom display driver with:
  - Graphics primitives: pixels, lines, rectangles, circles, triangles
  - Hardware SPI via SPI1
  - 128x128 framebuffer (2KB RAM)
  - Custom 8x8 and 16x16 bitmap fonts
  
- **ADC Sampler** (`lib/adc_sampler.*`) - ADC acquisition framework
- **Temperature** (`lib/temperature.*`) - RP2040 temperature sensor wrapper

## Display Driver Notes

- **Do not use U8g2** - Project uses custom SH1107 driver due to compatibility issues
- Display updates should be efficient; full framebuffer is 2KB
- Use `display.display()` to push framebuffer to screen
- Font rendering supports configurable character spacing
- Coordinate system: (0,0) at top-left, (127,127) at bottom-right

## Shared Data and Synchronization

- Use `mutex_t` for thread-safe access to shared data between cores
- Core 0 reads from shared structure
- Core 1 writes to shared structure
- Always acquire mutex before accessing shared data
- Keep critical sections short to avoid blocking

### Example Pattern

```cpp
mutex_t g_data_mutex;
volatile shared_data_t g_shared_data = {0};

// Core 1 (writer)
mutex_enter_blocking(&g_data_mutex);
g_shared_data.shot_count++;
g_shared_data.data_updated = true;
mutex_exit(&g_data_mutex);

// Core 0 (reader)
mutex_enter_blocking(&g_data_mutex);
local_data = g_shared_data;
g_shared_data.data_updated = false;
mutex_exit(&g_data_mutex);
```

## Performance Considerations

- **Watchdog enabled:** 2-second timeout; must kick periodically
- **Display updates:** ~60Hz via timer interrupt (Core 0)
- **ADC sampling:** Target ≥1kHz with DMA (Core 1)
- **Minimize allocations:** Use static buffers and stack allocation
- **ISR safety:** Keep interrupt handlers short and fast
- **DMA preferred:** For high-frequency operations (ADC, SPI)

## Testing and Validation

- No formal unit test framework currently in project
- Validate changes by:
  1. Compiling without errors
  2. Flashing to Pico hardware
  3. Observing display output for correctness
  4. Monitoring behavior over time (watchdog, stability)
  
- Use display for real-time debugging information
- Status LED can indicate simple state changes

## Common Pitfalls

1. **Forgetting mutex protection** when accessing shared data
2. **Blocking in ISRs** - keep interrupt handlers minimal
3. **Display buffer overflow** - respect 128x128 bounds
4. **Watchdog timeout** - ensure both cores kick watchdog
5. **SPI conflicts** - only Core 0 should access display
6. **ADC channel selection** - remember to select correct channel before reading

## Build Workflow

1. Make code changes
2. Compile: `ninja -C build` or use VS Code task
3. Fix any compilation errors
4. Flash to Pico hardware (use VS Code "Flash" task if available)
5. Verify behavior on display
6. Iterate as needed

## References

- **Pico SDK Documentation:** https://www.raspberrypi.com/documentation/pico-sdk/
- **Hardware Datasheets:** See `docs/hardware/` directory
  - `pico-datasheet.pdf` - RP2040 microcontroller specifications
  - `SH1107Datasheet.pdf` - Display controller reference
  - `getting-started-with-pico.pdf` - Raspberry Pi Pico guide
- **Project Documentation:** See `docs/devlog/` directory
  - `2025-01-15-design-guide.md` - Original design spec
  - `2025-10-31-PROJECT-STATUS.md` - Current implementation status
  - `2025-10-31-RECONNECTION-GUIDE.md` - Quick project overview
  - `docs/display/` - Display-specific documentation
  - `docs/hardware/` - Hardware datasheets and schematics

## Documentation Standards

### Development and Research Documentation

All major changes, research efforts, and design decisions must be documented in the `docs/devlog/` directory following these standards:

**Naming Convention:**
- Use date-prefixed format: `YYYY-MM-DD-description.md`
- Example: `2025-11-02-button-debounce-implementation.md`
- Use descriptive names that clearly indicate the document's purpose

**When to Create Documentation:**
- **Before** starting major feature implementations
- **During** research or investigation of technical approaches
- **After** completing significant milestones (retrospectives)
- When making design decisions that affect future development
- For troubleshooting complex issues that may recur

**Document Structure Guidelines:**
- Start with a brief overview/summary at the top
- Include date and status (e.g., "In Progress", "Complete", "Archived")
- Use clear section headers and markdown formatting
- Include code examples where relevant
- Reference related documentation and source files
- Add "What was NOT implemented" sections when relevant to scope future work

**Special Document Types:**
- `PROJECT-STATUS.md` - Current implementation status (update as major features complete)
- `RECONNECTION-GUIDE.md` - Quick overview for returning to the project after a break
- `README.md` - Directory overview explaining purpose of archived or grouped documents

**Timeline Approach Benefits:**
- Chronological tracking of project evolution
- Easy to locate decisions made at specific points in time
- Preserves context for future troubleshooting
- Creates a clear audit trail for engineering decisions

**Example Documents in this Project:**
- `2025-10-31-dma-sampling-implementation.md` - Implementation guide for DMA feature
- `2025-10-31-dma-sampling-retrospective.md` - Post-implementation review
- `2025-08-05-FONT-DECISION.md` - Design decision documentation
- `2025-01-30-sampling-research.md` - Research findings

## Best Practices

- Keep changes minimal and focused
- Test on hardware frequently during development
- Document hardware-dependent behavior
- Use type-safe constructs (enum class, constexpr) over macros when possible
- Prefer RAII and modern C++ features within embedded constraints
- Keep real-time requirements in mind (no blocking operations in critical paths)
- Always consider power consumption and performance impact
- When using new Pico SDK APIs (e.g., multicore lockout, flash operations), review the official SDK documentation (`pico-sdk/src/...`) before integrating
