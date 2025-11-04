# ADC Investigation Summary — 2025-11-03

## Issue

When the battery voltage divider is connected to the Pico ADC input, the ADC reading is much lower than expected (initially near 0 V, later ~1.3–1.8 V depending on wiring). When the divider is disconnected, the divider measures correctly (open-circuit midpoint ≈ expected value). The problem occurs only when the Pico and divider share the same supply.

## Short reproduction

1. Divider: R_top = 2.7 kΩ, R_bottom = 1.0 kΩ, Vin = 9 V → expected Vmid ≈ 2.4–2.5 V.
2. With divider alone (no Pico connected): Vmid ≈ 2.5 V (OK).
3. Power Pico from 9 V → regulator → 5 V → Pico; connect divider mid to ADC pin.
4. Vmid collapses (varied during debugging): initially ~0 V, later ~0.7 V, and after changes settles to ~1.8–2.1 V depending on pin and wiring.

## What we've tried

- Removed legacy timer-based `ADCSampler` code (clean-up).
- Reviewed `DMAADCSampler::init()` to ensure ADC GPIO pulls are disabled (it already calls `gpio_disable_pulls()` before `adc_gpio_init()`).
- Added `gpio_disable_pulls()` earlier to legacy code path while debugging.
- Moved ADC pin in software from GP26 (ADC0) to GP27 (ADC1) for isolation testing (updated `lib/adc_config.h`).
- Removed some diodes from regulator path as an experiment.
- Performed series-resistor isolation tests (1 kΩ, 10 kΩ) to evaluate where the load is.

## Key measurements (selected)

- Divider alone: Vin = 9 V → Vmid ≈ 2.5 V.
- With Pico powered from same supply and divider connected to ADC (various pins):
  - On GP27: Vmid ≈ 2.1 V, Vpin ≈ 1.3 V.
  - Equivalent load inferred on ADC net: ≈ 1–3 kΩ to ground (explains the ~0.7–1.2 V drop from expected Vmid).
- Unpowered checks: ADC pad measured >1 MΩ to ground (no hard short when unpowered).
- Diode/diode-mode tests under power show diode-like conduction mid→GND ≈ 0.6 V in some cases (indicating a powered clamp/protection element conducts under certain conditions).
- Series resistor (1 kΩ) between mid and ADC: partial recovery of Vmid, confirming the load is on Pico/board side.

## Inferred cause

- A powered-on conductive path on the Pico/board is loading the divider node to ground (or another rail) with an equivalent resistance in the kilo-ohm range when the Pico is powered from the same regulator. This path behaves like a diode/network when powered — i.e., it is not a simple passive short detectable when unpowered.

Possible specific culprits:
- On-board clamp/protection diodes, level-shifter or ESD networks on the connector that become conductive under power.
- Regulator wiring or diodes that create a back-path or clamp to ground when powered.
- A miswired net or pull resistor tied to the ADC net on the board.
- Less likely: MCU pin configured as output/pull-down by firmware (code already calls gpio_disable_pulls); or a damaged MCU pad (unpowered checks argue against this).

## Files changed (for traceability)

- Removed: `lib/adc_sampler.cpp`, `lib/adc_sampler.h` (legacy timer sampler removed).
- Edited: `lib/adc_config.h` (ADC_GPIO changed from 26 → 27, ADC_CHANNEL 0 → 1 for isolation testing).
- No changes made to `lib/dma_adc_sampler.*` beyond code review.

## Recommended next steps

Short-term diagnostics

- Insert a 10 kΩ series resistor between divider mid and ADC pin and measure both sides (pre/post resistor). If Vmid stays ~2.5 V and Vpin remains low, the clamp is on the Pico/board side.
- Power the Pico from a separate supply temporarily — if the problem disappears, it confirms the clamp is power-path dependent (already observed in earlier tests).
- Use the MCU to actively drive the ADC pin high (as a temporary firmware test). If the MCU can drive the pin to 3.3 V, the issue is not a hard short but an external clamp/pull; if it can't, the clamp is very low impedance.

Medium-term fixes

- Inspect and remove/rework any protection diodes or level shifters on the ADC net that are in the DC path of the divider node. Protection networks should be arranged in parallel to rails, not in series with divider return.
- If the offending component is part of the regulator or board connector and cannot be removed, either route the divider to another ADC pin that is free of the load (e.g., GP27 if that tests clean), or buffer the divider midpoint with a unity-gain op-amp before the ADC.
- As a temporary software/hardware workaround, lower the divider impedance (smaller resistor values) so divider source resistance << the observed ~2 kΩ load. This wastes power but reduces measurement error until the root cause is fixed.

## Recommended post-fix tests

- With the suspected clamp removed, verify: Vmid ≈ expected (divider alone), Vmid ≈ expected when connected to ADC, ADC readings match multimeter.
- Confirm that ADC pad measures high impedance to ground when unpowered.
- Re-run series resistor test to ensure any remaining loading is negligible.

---

If you want I can also:
- Create a small test UF2 that prints GPIO/ADC pin function and toggles the pin (to prove whether the MCU can drive the pin high). I can build this and provide the binary for you to flash.
- Draft a one-page annotated schematic suggestion showing which diodes/clamps to remove or re-route.


*Prepared by: debugging session with the airsoft-display repo (branch: fix/adc-zero) — 2025-11-03*
