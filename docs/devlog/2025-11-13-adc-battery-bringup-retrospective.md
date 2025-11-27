# ADC/Battery Measurement Debug & Bringup Retrospective

**Date:** 2025-11-13  
**Status:** Complete (ready for sample analysis)

---

## Overview
This document summarizes the process of testing, troubleshooting, and refining the ADC-based battery voltage measurement system for the Airsoft Display project. It covers the initial implementation, issues discovered, hardware/software fixes, and the learnings that will inform future work—especially as we move toward robust shot detection.

---

## Initial Implementation & Testing
- **Goal:** Accurately measure battery voltage using the RP2040 ADC, voltage divider, and buffer circuit, and display both the raw ADC voltage and scaled battery voltage on the OLED.
- **Setup:**
  - Voltage divider: Initially 28kΩ/10kΩ, later changed to 3.3kΩ/1kΩ
  - Buffer: TL072 op-amp, later to be replaced with MCP6002
  - 100Ω series resistor on ADC input
  - Diode in series with battery for reverse polarity protection
  - ADC sampling at 5kHz using DMA

### First Results
- ADC readings on display were consistently lower than measured at the ADC pin (e.g., 1.6V at pin, 1.2V displayed)
- Battery voltage displayed was also low
- Raw ADC counts confirmed the discrepancy

---

## Troubleshooting & Discoveries
### 1. Hardware/Software Mismatch
- Discovered ADC channel config was mismatched with hardware pin; confirmed correct use of GP27/ADC1

### 2. Voltage Divider & Buffer Effects
- Realized divider values in code did not match hardware (updated to 3.3kΩ/1kΩ)
- Confirmed that high divider impedance can cause ADC droop, but with buffer and 100Ω series resistor, source impedance was within RP2040 spec

### 3. Calibration & Real-World Effects
- Measured actual voltages at ADC pin and battery, compared to display
- Calculated and applied a calibration factor to correct for:
  - Buffer output impedance
  - Series resistor
  - Divider resistor tolerances
  - ADC reference voltage error
- Measured actual divider ratio (battery after diode / ADC pin voltage) and used this instead of theoretical value
- Added compensation for diode drop to display true battery voltage (pre-diode)

### 4. Iterative Testing
- Repeated process: measure at pin, compare to display, adjust calibration and divider ratio
- Verified with multiple battery voltages and loads
- Ensured display now matches multimeter at both ADC pin and battery (pre-diode)

---

## Results & Challenges
- **Results:**
  - Displayed ADC voltage and battery voltage now match physical measurements within a few percent
  - System compensates for all real-world effects (divider, buffer, diode, calibration)
  - Ready to collect and analyze real ADC samples for noise and event detection
- **Challenges:**
  - Hardware tolerances and real-world effects (diode drop, op-amp offset, resistor values) required empirical calibration
  - ADC readings are sensitive to source impedance and layout; buffer and series resistor are essential
  - Need to document all hardware changes and calibration steps for reproducibility

---

## Learnings & Best Practices
- Always measure and use real divider ratios and calibration factors, not just theoretical values
- Buffering the ADC input and using a low source impedance is critical for high-speed sampling
- Account for all voltage drops (e.g., diodes) in the measurement path
- Use the display for real-time debug output during bringup
- Document every hardware and software change, and keep a log of calibration data
- Plan for future recalibration if hardware changes (e.g., new op-amp, different resistors)

---

## Current State
- ADC and battery voltage measurement is accurate and stable
- Display shows both post-diode (ADC pin) and pre-diode (true battery) voltages
- System is ready for sample collection and analysis
- All calibration and compensation is documented in code and hardware

---

## Next Steps
1. **Analyze collected ADC samples:**
   - Characterize noise (frequency, amplitude, sources)
   - Determine what noise should be filtered in software/hardware
   - Evaluate effectiveness of current filter chain (median + low-pass)
2. **Evaluate hardware improvements:**
   - Consider MCP6002 op-amp swap
   - Test with different divider values or additional filtering caps
3. **Begin shot detection implementation:**
   - Use clean, calibrated ADC data as input
   - Develop and test detection algorithms

---

## Looking Ahead
- Continue to document all findings and calibration steps
- Use this bringup experience as a template for future sensor integration and troubleshooting
- Maintain a clear separation between hardware and software compensation for easier debugging

---

**Document prepared by GitHub Copilot, 2025-11-13**
