# Op-amp unity-gain buffer for ADC divider (reference)

This page contains a compact, practical schematic and BOM for placing a unity-gain buffer (voltage follower) between a high-impedance voltage divider and the Pico ADC input. The goal is to isolate the divider midpoint from any powered board-side clamp/protection on the ADC net and present a low-impedance source to the ADC.

## Summary / intent

- Place a rail-to-rail op-amp in a voltage-follower configuration between the divider midpoint and the ADC pin.
- Keep divider values (Rtop/Rbot) as-is for accuracy/low power, or lower them temporarily if you need immediate robustness while debugging.
- Add decoupling close to the op-amp. Optionally add small series/output resistor and input protection for robustness.

## Schematic (text / wiring)

Notes: net names used below — DIV_MID = divider midpoint (between R_TOP and R_BOT), OP_OUT = op-amp output connected to ADC pin (ADC_IN), VDD = 3.3V, GND = 0V.

DIVIDER (unchanged):
  VIN ---- R_TOP ----+---- R_BOT ---- GND
                     |
                   DIV_MID (test point) ---> to op-amp non-inverting input (VIN+)

OP-AMP (unity-gain follower):
  VDD (3.3V)
   |
  [op-amp V+ pin]

  GND
   |
  [op-amp V- pin]

  VIN+  <--- DIV_MID (direct connect)
  VIN-  <--- OP_OUT (feedback; connect output to VIN-)
  OP_OUT ---> optional series resistor R_OUT (~22–100 Ω) ---> ADC_IN (Pico ADC pin)

Optional small input series resistor (R_IN ~100 Ω–1 kΩ) between DIV_MID and VIN+ can limit input surge and isolate the divider during transients, but it raises source impedance slightly — prefer only if needed.

Add test points:
- TP_DIV_PRE: probe DIV_MID directly (before op-amp input)
- TP_OP_OUT: probe op-amp output (after any R_OUT) which connects to ADC pin

## Decoupling and layout notes

- Place a 100 nF ceramic capacitor as close as possible to the op-amp supply pins (VDD to GND).
- Add a 1 µF (X7R) ceramic near the same pins for bulk decoupling if board area permits.
- Keep the feedback trace (OP_OUT -> VIN-) short.
- Keep DIV_MID -> VIN+ short and away from noisy digital traces.
- If you need ESD protection, place it on the divider side (DIV_MID) referenced to GND, not in the path from DIV_MID to VIN+ (avoid low-value clamp elements in series with divider node).

## Protection suggestions (optional)

- Small series resistor R_IN (100 Ω–1 kΩ) between DIV_MID and VIN+ protects against accidental shorts; use larger values only if you understand the effect on source impedance.
- Small series resistor R_OUT (22–100 Ω) on the op-amp output protects the op-amp from capacitive loads and the ADC from transient currents. Typical: 47 Ω.
- If the board has suspect clamp diodes, prefer removing or relocating them rather than adding parallel clamps.

## Component selection guidance

Important op-amp properties:
- Single-supply operation at 3.3 V (or lower), rail-to-rail input and output recommended.
- Input common-mode range including ground and up to VDD.
- Low input bias current (pA–nA) for minimal divider loading.
- Low offset voltage for accurate measurements (or acceptable offset if calibrating).
- Small package if rework is required: SOT-23-5 or SOIC-8 are common.

Example candidate parts (verify availability and footprints):

- MCP6001 (Microchip) — single op-amp, rail-to-rail I/O, Vdd down to 1.8 V. Package: SOT-23-5. Good low-cost option.
- OPA2333 (Texas Instruments) — dual; zero-drift versions are available (OPA2333 is dual RRO). Low offset, low bias, rail-to-rail, 3.3 V friendly.
- TS912 / TLV237x family — single/dual op-amps with rail-to-rail behavior; check input range.

Notes on part choice:
- For best low-power and low-input-bias performance, prefer CMOS-input op-amps (e.g., MCP6001 or OPA2333) over older bipolar op-amps.
- If you need the absolute best ADC accuracy, consider an op-amp with microvolt-level offset or compensate in software.

## Example BOM (reference)

1) U1 — Op-amp, rail-to-rail, single-supply
   - Example: MCP6001T-E/OT (SOT-23-5) or OPA2333AIDR (SOIC-8 / UDFN variants)
   - Qty: 1

2) C1 — 100 nF ceramic, 0805 (decoupling)
   - Qty: 1

3) C2 — 1 µF ceramic, 0805 (bulk decoupling)
   - Qty: 1

4) R_OUT — 47 Ω, 0603 (optional output resistor)
   - Qty: 1

5) R_IN — 220 Ω (optional input series resistor for protection)
   - Qty: 1

6) TP_DIV_PRE, TP_OP_OUT — test pads or 2.54mm pin headers (optional)

## Example netlist mapping (for your board)

- DIV_MID -> PCB net name: e.g., BAT_DIV_MID
- U1 VIN+ -> BAT_DIV_MID (via R_IN if used)
- U1 VIN- -> U1 OUT (feedback)
- U1 OUT -> R_OUT (optional) -> PCB net connected to Pico ADC pin (e.g., ADC_VMON)
- U1 VDD -> 3V3 (same rail as Pico)
- U1 GND -> GND

## Verification procedure after assembly

1. With power off, continuity check: ensure U1 OUT is not shorted to ground.
2. Power the board. With the divider connected and the Pico powered, measure:
   - TP_DIV_PRE: should read expected divider value (~2.4–2.5 V for the example divider)
   - TP_OP_OUT: should read the same (within op-amp offset and any R_OUT drop)
   - ADC reading on Pico should now match the multimeter on TP_OP_OUT.
3. If TP_OP_OUT reads correct but ADC still low, check that ADC pin is not otherwise clamped and that the net between OP_OUT and ADC pin is connected.

## Notes & alternatives

- If you cannot rework the board, using a low-value divider (e.g., 2.2 kΩ / 1 kΩ) temporarily reduces the error but increases current draw.
- If the clamp source is found and removable, deleting or rerouting the offending clamp is the clean solution and removes the need for the buffer.

---

If you want, I can also:
- Produce a small SVG or KiCad snippet of the schematic and footprint placement.
- Create a one-page PDF with the schematic and BOM pinned for assembly.

Tell me which format you prefer next (KiCad schematic snippet, SVG, or PDF) and I will generate it.
