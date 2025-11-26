# SSD1331 (HiLetgo 0.95") Impact Analysis & Implementation Plan

Summary
- Target module: HiLetgo 0.95" 96×64 RGB OLED module (SSD1331 controller, 7-pin SPI).
- Project context: RP2040 (Raspberry Pi Pico), current display: SH1107 128×128 monochrome (custom driver, 2 KB framebuffer). Copilot instructions (repo) confirm SPI1 usage and display code conventions.
- High-level verdict: Feasible on RP2040 but requires non-trivial firmware and documentation changes (driver, graphics, power, wiring, and UI/layout). I will not change code unless you confirm.

Key impacts
1. Hardware / Electrical
   - Interface: SSD1331 uses SPI (CLK, MOSI, DC, RST, CS, VCC, GND). Likely mappable to existing SPI1 pins but verify physical pin order and connector.
   - Voltage: Module typically 3.3V — compatible with Pico logic. Confirm VCC tolerance on the specific module.
   - Power consumption: RGB OLEDs draw more current than the existing SH1107 monochrome display. Re-evaluate power budget and regulator capacity (project currently lists L7805CV and ~50mA typical; expect higher draw and peak currents).
   - Mechanical: Check module dimensions and connector; may need new connector or mounting changes.

2. Memory & Performance
   - Framebuffer: RGB565 (16-bit) → 96 × 64 × 2 = 12,288 bytes (~12 KB). RP2040 SRAM (~264 KB) easily supports this, but it’s a large increase vs current 2 KB.
   - SPI bandwidth: Full-screen updates send ~12 KB vs ~2 KB previously; affects refresh rate and CPU usage.
   - Strategies if CPU or timing problematic:
     - DMA-backed SPI transfers (recommended).
     - Tiled/partial updates or streaming to avoid large allocations if needed.

3. Software / Driver
   - Driver: Add SSD1331 driver (init, command/data functions, framebuffer handling). Project currently uses a custom sh1107-pico driver — follow that layout and patterns.
   - Graphics abstraction: Introduce/extend a display abstraction layer so high-level UI code can target either SH1107 or SSD1331 with minimal changes.
   - Fonts & UI: Update layouts and fonts for 96×64 resolution; redesign UI elements sized for 128×128.
   - Color support: Decide whether to use the display’s color capability (RGB565) or keep monochrome-style rendering on the RGB panel.

4. Dual-core & Concurrency
   - Keep Core 0 for display. Because color updates are heavier, use DMA and keep mutex critical sections short to avoid blocking Core 1 (ADC / shot detection).
   - Ensure watchdog kicking behavior isn’t affected by longer display transfers.

5. DMA / PIO Opportunities
   - Implement DMA streaming of framebuffer to SPI for non-blocking, efficient display updates.
   - PIO is optional; hardware SPI + DMA should be sufficient and simpler.

6. Power, Reliability & UX
   - Add brightness control and screensaver to reduce power draw and prevent OLED burn-in.
   - Measure battery life impact at typical brightness levels and adjust defaults.

7. Documentation & Tests
   - Update wiring diagrams and hardware docs (docs/hardware/ and .github/copilot-instructions.md).
   - Add new library under lib/ (e.g., lib/ssd1331-pico/) modeled after lib/sh1107-pico/.
   - Add demo/test harness to validate visuals and performance.

Concrete checklist / tasks
- Planning
  - Confirm whether you want full color (RGB565) or monochrome-like UI on the RGB module.
  - Confirm physical/mounting constraints.
- Prototype (recommended)
  - Acquire one SSD1331 module and a Pico dev board.
  - Wire to SPI1 pins used in repo (verify pin mapping).
  - Measure current draw at multiple brightness levels.
- Firmware
  - Design a small display abstraction API (init, drawPixel, fillRect, display, setBrightness).
  - Implement lib/ssd1331-pico/ with:
    - SPI init, reset, init sequence, command/data helpers.
    - Framebuffer storage (RGB565) and a display() method.
    - Optional: DMA-backed SPI transfer.
  - Integrate into UI: adapt layouts, fonts, color constants.
  - Implement brightness control and screensaver to prevent burn-in.
- Testing & Validation
  - Build & flash to Pi Pico, validate rendering and responsiveness.
  - Validate dual-core behavior and watchdog interactions.
  - Measure battery life and thermal behavior.
- Documentation
  - Update wiring table in docs/hardware/ and copilot instructions (note: follow repo policy & do not modify without confirmation).
  - Add README for new driver and usage examples.

Estimated rough effort
- Prototype wiring + basic driver + simple drawing demo: 1–2 days.
- Full UI integration + testing + power measurements: 2–4 days.
- DMA optimization and polish: +1–2 days.

Open questions for you, Xris
1. Do you want full-color (RGB565) support, or prefer to keep a simple monochrome-like UI on the RGB panel to reduce design effort?
2. Should I draft a GitHub issue in the repo that breaks this work into tasks (planning only — I won't create it until you say go)?
3. Prioritize DMA-backed SPI transfers from the start, or begin with a blocking prototype driver and optimize later?
4. Do you want a starter driver code snippet (init + minimal draw) to review before we create files in the repo?

Next step recommendation
- If you want me to draft an implementation issue with concrete tasks and file-placeholders, say “Draft issue” and I’ll prepare the issue content for your review (I will not open the issue until you confirm).
- If you prefer, I can also produce a small prototype driver snippet next.

Notes
- This analysis is grounded in the repository Copilot instructions file (lib/sh1107-pico usage, RP2040 constraints, SPI1 pin assignments, framebuffer assumptions). Where assumptions about the SSD1331 module vary, validate the specific module’s pinout and power ratings.