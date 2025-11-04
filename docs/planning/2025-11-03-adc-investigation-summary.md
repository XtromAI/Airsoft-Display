# ADC Investigation Summary — 2025-11-03 (updated)

## Issue

When the battery voltage divider is connected to the Pico ADC input, the ADC reading is much lower than expected (initially near 0 V; later instrumented testing showed ~0.03 V on the ADC input while the divider midpoint measured ~2.4 V). When the divider is disconnected, the divider measures correctly (open-circuit midpoint ≈ expected value). The problem occurs only when the Pico and divider share the same supply.

This document aggregates the full debugging sequence, the additional drive-test measurements, and the recommended hardware remediation path (op-amp buffer plan is in-progress).

## Short reproduction

1. Divider used during testing: R_top = 2.7 kΩ, R_bottom = 1.0 kΩ, Vin = 9 V → expected Vmid ≈ 2.4–2.5 V.
2. With divider alone (no Pico connected): Vmid ≈ 2.5 V (OK).
3. Power Pico from 9 V → regulator → 5 V → Pico; connect divider mid to ADC pin.
4. Vmid collapses or the ADC reading is unexpectedly low only when the Pico is powered and the divider is attached.

## Additional tests performed (new since earlier note)

- Created and flashed a dedicated drive-test UF2 that repeatedly:
  - Reads the ADC (as input) and prints raw ADC values.
  - Switches the pin to a digital output and drives it high, then reads the ADC again (to observe driven level), then restores ADC input mode.
- Performed series-resistor isolation test with a 10 kΩ resistor between the divider midpoint and ADC pin, measuring pre/post resistor voltages.
- Performed continuity and unpowered impedance checks of the ADC pad.

## New key measurements and observations

- Divider alone: Vin = 9 V → Vmid ≈ 2.5 V (unchanged).
- Drive-test firmware results (selected logs from the test run):
  - When configured as digital output and driven high: measured ADC-equivalent raw ≈ 3960 → voltage ≈ 3.19–3.20 V (MCU output works and can reach Vdd).
  - When configured as ADC input and read: raw ≈ 32–33 → voltage ≈ 0.026–0.027 V (node reads near 0 V while input).
  - This pattern repeats: output drive shows full-voltage ability, input mode shows near-zero reading.
- Series resistor (10 kΩ) isolation test:
  - Measured Vmid on divider (pre-resistor) ≈ 2.4 V.
  - Measured Vpin after the 10 kΩ (node on Pico side) ≈ 0.43 V.
  - From these numbers we infer an effective clamp/load on the Pico-side node of roughly ~2.2 kΩ to ground (Vdrop across the 10 kΩ implies that the node is being pulled down by a ~2 kΩ equivalent resistance when the Pico is powered and connected).
- Unpowered pad checks: ADC pad measures very high impedance to ground (>1 MΩ) when the board is unpowered, indicating the clamp is power-dependent (not a hard short when unpowered).

## Strong conclusion from tests

- The evidence strongly indicates the root cause is a powered, board-side conductive path (clamp/pulldown) on the ADC net that only appears when the Pico and/or associated power/regulator network is powered. It behaves like a diode/clamp or an active protection network that becomes conductive under certain rail conditions.
- This is NOT a software-only issue: code paths calling `gpio_disable_pulls()` and the DMA sampler initialization were inspected and confirmed. The MCU pin itself is capable of sourcing to ~3.2 V when configured as output, so the MCU pad is not irreversibly shorted.

## Probable culprits (refined)

- ESD/protection diodes, clamp networks, or level-shifters on the ADC net that conduct into ground or into another rail when the board is powered.
- Regulator or diode wiring that creates a back-path or clamp under operating conditions.
- A misrouted net or erroneous component on the ADC trace that ties it to a powered clamp.

## Files changed (traceability)

- Removed: `lib/adc_sampler.cpp`, `lib/adc_sampler.h` (legacy timer sampler removed during cleanup).
- Edited: `lib/adc_config.h` (ADC_GPIO changed from GP26 → GP27, ADC_CHANNEL 0 → 1 for isolation testing).
- Created: `src/adc_test.cpp` (drive-test firmware used to collect the new measurements).
- No functional changes made to `lib/dma_adc_sampler.*` beyond code review; it already called `gpio_disable_pulls()`.

## Recommended next steps (user indicated they will pursue op-amp angle)

Short-term diagnostics (what we did / can still do):

- Confirm the clamp location with a scope if available: probe the divider midpoint and Pico-side node simultaneously while powering the Pico. Look for diode-like conduction and identify which net the clamp returns to.
- Remove or disconnect nearby protection diode or level-shifter nets on the ADC trace if feasible on your board revision, and re-run the 10 kΩ isolation test.
- Try powering only the Pico (no battery) or powering Pico from USB while keeping divider powered from the battery — see whether the clamp behavior depends on which supply arrangement is used.

Op-amp buffering (user is pursuing this):

- A low-cost, rail-to-rail input/output buffer in a unity-gain configuration (voltage follower) placed between the divider midpoint and the ADC pin will isolate the divider from any clamp on the ADC net. The buffer must be powered from the same 3.3 V rail as the Pico so its output range includes the divider voltage range.

- Preferred op-amp properties for this application:
  - Single-supply operation down to 3.3 V (or rail-to-rail input/output on 3.3 V).
  - Input common-mode range that includes ground and up to Vdd.
  - Low input bias current (so the divider accuracy isn't degraded) and low offset for more accurate voltage measurement.
  - Small SOT-23 or SOIC package for board rework.

- Candidate parts (examples; choose based on availability and footprint comfort): MCP6001/MCP6002, OPA2333, TS912 or TLV237x series. Validate the chosen part's input common-mode range and output swing given 3.3 V supply.

Hardware trade-offs and notes:

- Buffering is the least-invasive fix since it leaves the existing divider and board trace intact; it presents a low-impedance source to the ADC and prevents the board-side clamp from loading the divider.

- A permanent rework would be to remove the offending clamp/protection from the divider node or reroute the divider to a different ADC input that is not affected.

- Lowering divider impedance works as a temporary mitigation but increases quiescent current (wastes battery power) — not ideal for long-term deployment.

Recommended post-fix verification

- After adding the buffer or removing the clamp: verify Vmid measured on the multimeter ≈ expected (2.4–2.5 V) and ADC readings match the meter within expected error.

- Re-run the 10 kΩ isolation test: the Pico-side node should no longer be pulled down by a ~2 kΩ equivalent load.

Notes for the op-amp route (practical checklist)

1. Pick a rail-to-rail op-amp rated for single-supply 3.3 V operation.
2. Place the op-amp as a unity-gain buffer: divider midpoint → op-amp non-inverting input; output → ADC pin; op-amp inverting input → output (feedback).
3. Decouple the op-amp supply pins near the device (100 nF + 1 µF recommended).
4. Confirm the op-amp output can swing to the expected measurement range (e.g., 0–3.3 V) without hitting internal saturation.
5. Re-run the drive-test UF2 and multimeter checks.

---

If you want I can also:
- Provide a small reference schematic snippet for the op-amp buffer (pin connections and decoupling) and a BOM with 2–3 recommended devices.
- Create an updated UF2 that exercises the buffer path and logs ADC vs. pre-buffer voltage (if you place a test point pre-buffer).

*Prepared by: debugging session with the airsoft-display repo (branch: fix/adc-zero) — 2025-11-03*
