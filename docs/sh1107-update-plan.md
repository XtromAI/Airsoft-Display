

# SH1107 Driver Refactor & Enhancement Plan

## 1. PIO for Display Transfer

**Goal:** Offload display signaling to the RP2040’s PIO for precise timing and to free CPU resources.

- Write a PIO program for SPI (or parallel) communication, handling MOSI/SCK (and optionally DC/CS).
- Integrate PIO initialization and data transfer into the C++ driver.
- Replace hardware SPI calls with PIO TX FIFO data streaming.

## 2. DMA for Frame Buffer Streaming

**Goal:** Use RP2040’s DMA to stream the frame buffer directly to the PIO or SPI, minimizing CPU involvement.

- Allocate and configure a DMA channel for display updates.
- Set up DMA to transfer from SRAM (frame buffer) to PIO/SPI peripheral.
- Trigger DMA on update, and handle completion with interrupts or polling.
- Support both full-frame and partial (dirty region) updates.

## 3. Frame Buffering Enhancements

### a. Column-Major, Bit-Packed Buffer

- Restructure the frame buffer to match SH1107’s native column-major, bit-packed format.
- Update graphics routines to use this layout for efficient pixel operations and transfers.

### b. Dirty Region Tracking

- Implement tracking of modified regions (dirty rectangles or tiles).
- Only transfer changed regions during display updates, reducing bus traffic.

### c. Double Buffering

- Allocate a second buffer in SRAM.
- Perform all drawing to the back buffer.
- Swap front/back pointers before triggering display update for tear-free animation.

## 4. Execution Sequence

1. Refactor buffer layout and graphics routines.
2. Add dirty region tracking and partial update logic.
3. Implement double buffering.
4. Write and integrate PIO program for display protocol.
5. Set up and test DMA transfers for display updates.

