
# Raspberry Pi Pico Shot Counter & Battery Monitor

## Project Overview

This project implements a shot counter and battery monitor for an Airsoft gun using a Raspberry Pi Pico. The core functionality relies on high-precision, high-frequency voltage sampling to detect a voltage dip, which signifies a shot being fired. This plan outlines a robust software architecture using the Pico's dual cores, DMA, and hardware timers to achieve precise and reliable performance.

## Hardware Components

- **Microcontroller:** Raspberry Pi Pico (RP2040)
- **Display:** SH1107 128x128 OLED display (SPI interface)
  - Reference: 1.3-inch SH1107 OLED Display on Amazon
  - *Note: Please confirm the screen size and SPI connection details yourself. The plan assumes a 128x128 resolution and an SPI interface.*
- **Battery:** Airsoft gun battery (7.4V or 11.1V LiPo)
- **Voltage Regulator:** DC-DC converter or LDO to power the Pico from the Airsoft battery, stepping the voltage down to the Pico's operating range (e.g., 3.3V or 5V via VBUS).
  - **Specific Model:** STMicroelectronics L7805CV
  - **Specifications:** Fixed 5V output, max 1.5A, input 7V–35V.
  - **Input Voltage:** Powered by two cells of the 11.1V battery via the balance port connector (nominal 7.4V).
  - *Note: Due to the voltage drop (e.g., from 7.4V to 5V), the regulator will dissipate a small amount of heat. A heatsink is still recommended to ensure stable operation and prevent thermal shutdown, especially under load.*
- **Voltage Divider:** Two series resistors to scale battery voltage down to Pico's ADC range (max 3.3V).
- **Wiring:** Standard breadboard/PCB, jumper wires, etc.

## Hardware Schematic

*(Add your schematic here for the voltage divider and voltage regulator)*

## Software Architecture

The application will be split across the Pico's two CPU cores to ensure real-time performance and prevent display updates from interfering with critical sampling.

### Core 0 (Data Acquisition & Processing)

- **Primary Task:** High-frequency, hardware-assisted battery voltage sampling.
- **Logic:**
  - Initialize ADC, hardware timer, and DMA channel.
  - Timer triggers ADC conversions at a consistent, high frequency (>1000Hz). *[Configurable in code]*
  - DMA transfers ADC results into a circular buffer in RAM.
  - Process DMA completion interrupts to calculate a moving average.
  - Maintain a history buffer of recent moving averages.
  - Detect voltage dips for shot detection.
  - Manage shot counter and debouncing.
  - Share data with Core 1 via a thread-safe structure.
  - Kick the watchdog timer periodically.

### Core 1 (Display & UI)

- **Primary Task:** Rendering the user interface on the SH1107 display.
- **Logic:**
  - Initialize SPI and display.
  - Handle all display drawing, including text and graphs, using a custom 16×16 font.
  - Periodically read latest data from shared structure.
  - Render information to the screen.
  - Handle user input (e.g., buttons).

## Display Layout

The 128×128 pixel OLED display will be used to show key information in a clear, easy-to-read layout. The primary display will consist of:

- **Shot Count:** Large, prominent display of the current shot count using the custom 16×16 pixel font, centered on the screen.
- **Battery Voltage:** Current battery voltage at the bottom of the screen, possibly with a small battery icon.
- **Trend Graph (Optional):** Small graph showing the recent voltage trend to help visualize shot detection.

## Implementation Steps

This is a step-by-step plan for building the code.

### Step 1: Project Setup and Boilerplate

1. Set up a new C++ project for the Raspberry Pi Pico using the pico-sdk.
2. Configure `CMakeLists.txt` with the necessary libraries: `pico_stdlib`, `hardware_adc`, `hardware_dma`, `hardware_timer`, `hardware_spi`, `pico_multicore`, `pico_sync`.
3. Include a display library. We will use the U8g2 library, which is a universal graphics library for embedded systems with support for the SH1107 display. It can be integrated into your project.
4. Create `main.cpp` for Core 1 logic and a `core0_main.cpp` file or function for the Core 0 logic.

### Step 2: Core 0 - ADC, DMA, and Timer Configuration

**core0_main function:**

1. Initialize the ADC, setting it to the correct pin for the battery voltage divider.
2. Initialize a large volatile `uint16_t` circular buffer in RAM for ADC samples (e.g., 1024 samples).
3. Configure a hardware timer to fire at the desired frequency (e.g., 1000Hz) and trigger an ADC conversion.
4. Set up a DMA channel:
    - Configure DMA to transfer data from the ADC FIFO (`ADC_DATA_REG`) to the circular buffer.
    - Enable DMA in circular mode (`dma_channel_config_set_ring`).
    - Set the trigger source to the ADC.
    - Set up a DMA interrupt to fire when a batch of samples is complete.

### Step 3: Core 0 - Data Processing and Detection Logic

**Global Data Structures:**

1. Create a struct to hold shared data (e.g., `current_voltage_mv`, `shot_count`, `moving_average_mv`).
2. Use a `pico_mutex_t` to protect this shared data from simultaneous access by both cores.
3. Initialize a volatile float circular buffer for the moving average history.

**DMA Interrupt Handler:**

1. Write the `dma_irq_handler` function.
2. Inside the handler, signal to the main Core 0 loop that a new batch of samples is ready.

**Core 0 Main Loop:**

1. In a continuous `while(true)` loop:
    - Wait for the signal from the DMA interrupt handler.
    - Acquire the mutex.
    - Read the latest N samples from the ADC circular buffer.
    - Calculate the continuous moving average.
    - Add this new moving average value to the `moving_average_history_buffer`.
    - Perform the shot detection logic:
        - Find the maximum value in the `moving_average_history_buffer`.
        - Compare the current moving average to this maximum. If the drop is greater than a defined threshold, increment the `shot_count`.
    - Update the shared data structure.
    - Release the mutex.
    - Periodically "kick" the watchdog timer to prevent a reset.
    - Add a small sleep to avoid busy-waiting.

### Step 4: Core 1 - Display and UI

**main function:**

1. Launch Core 0's logic using `multicore_launch_core1(core0_main)`.
2. Initialize the SPI bus and the SH1107 display driver.
3. Initialize a separate hardware timer to fire at a steady, slower rate (e.g., 60Hz) to trigger display updates. This provides a consistent refresh rate and limits mutex contention.
4. In a `while(true)` loop:
    - Wait for a flag or event from the display update timer's interrupt handler.
    - Acquire the mutex.
    - Read the latest data from the shared data structure.
    - Release the mutex.
    - Clear the display buffer.
    - Draw the UI:
        - Display the `shot_count` prominently, centered using the custom 16×16 font.
        - Display the `current_voltage_mv`.
        - Optionally, draw a small trend graph using the `moving_average_history_buffer`.
    - Update the display.

### Step 5: Implement User Interface Logic

- The single physical button will be used to reset the shot counter.
    - A short press will have no effect.
    - A long press (3 seconds) will reset the shot counter to zero. This will require a timer or a counter to track the duration of the button press.

### Step 6: Implement Custom Font and Graphics

This step involves developing or sourcing a custom 16×16 pixel font (monochrome bitmap) and implementing the necessary rendering routines to draw characters and strings to the display using the U8g2 library.

### Step 7: Final Touches

- **Voltage Divider Calculation:** Determine the correct resistor values for the voltage divider based on your battery's maximum voltage and the Pico's ADC range.
- **Calibration:** Write a function to calibrate the ADC, mapping the raw digital values to a millivolt reading.
- **Tuning:** Adjust the moving average window size and the shot detection threshold to match the characteristics of your Airsoft gun's motor and battery.
- **Watchdog:** Implement the watchdog timer initialization and "kick" functions to ensure system stability.

## Required Libraries

- **pico-sdk** (for all hardware peripherals)
- **U8g2 library** (for the SH1107 display driver and graphics)