# DMA Sampling & Voltage Display Retrospective

**Date:** 2025-10-31  
**Branch:** `copilot/implement-sampling-method`

## Highlights

- Brought the DMA-based ADC sampler online at 5 kHz using a hardware alarm trigger.  
- Hardened the ADC pipeline: ensured conversions only fire when `ADC_CS_READY` is set, drained stale FIFO data, and confirmed continuous buffer turnover without overflow.  
- Instrumented the data path with serial diagnostics (loop frequency, DMA counters, ADC register snapshots) and on-screen statistics for live visibility.  
- Replaced the legacy timer sampler and removed the temperature subsystem to avoid resource contention with the ADC.  
- Stabilized voltage reporting by preserving the latest averaged value between DMA batches and formatting measurements directly in volts on the SH1107 display.

## Key Changes

| Area | Description |
|------|-------------|
| `lib/dma_adc_sampler.cpp` | Added hardware alarm scheduling, ADC readiness checks, FIFO maintenance, DMA busy helpers, and richer instrumentation. |
| `src/main.cpp` | Split responsibilities across cores, surfaced DMA stats on-screen, refined voltage averaging/display logic, and removed temperature usage. |
| `lib/voltage_filter.*` | Median + IIR low-pass filter chain processes every DMA sample prior to voltage conversion. |
| `CMakeLists.txt` | Pruned the temperature module from the build. |
| `lib/temperature.*` | Retired (pending deletion) after conflicts with shared ADC usage. |

## Observations

- With the ADC input floating, the filtered voltage stabilizes around 0.906–0.907 V; introducing a firm ground or 3.3 V reference causes the expected quick step response.
- DMA counters (`BUF`, `IRQ`, `SMP`, `TMR`) advance ~10 buffers per second, aligning with 512-sample buffers at 5 kHz.  No overflows observed after watchdog and FIFO fixes.
- The watchdog remains active (2 s timeout) and is serviced correctly on both cores.
- Debug serial output remains invaluable for spotting FIFO stalls or DMA contention—retain these prints during ongoing development.

## Follow-Up Ideas

1. Replace the stubbed `lib/temperature.*` files with calibrated environment sensors if future requirements demand it.  
2. Add min/max voltage logging per buffer to visualize ripple when the sampler is connected to live hardware.  
3. Explore DMA-assisted display transfers once battery monitoring and shot detection are feature-complete.

## Verification Checklist

- `Compile Project` task succeeds after all modifications.  
- Serial logs show monotonic increases in DMA counters and a steady `avg=906.xmV` while the pin floats.  
- OLED displays live buffer/IRQ/TMR statistics alongside the voltage readout without blanking or tearing.  
- Watchdog resets no longer occur under sustained sampling.
