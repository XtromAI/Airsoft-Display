# 🎯 Airsoft Display Battery Monitoring Circuit Description

This document describes the finalized electronic schematic designed for a LiPo battery-powered airsoft display, which provides both a stable 5V power supply and an accurate, noise-immune voltage signal for monitoring battery level and counting shots via a microcontroller's (e.g., Raspberry Pi Pico) Analog-to-Digital Converter (ADC).

---

## ⚡️ I. Power Regulation Block (PICO_VSYS)

This section converts the variable LiPo battery voltage (≈ 7.4V to 12.6V) into a stable 5V rail to safely power the microcontroller and display.

| Component | Value/Type | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **LIPO** | Battery | Input Power | **Power Source.** Supplies the primary unregulated voltage. |
| **LM7805** | Linear Regulator | Input → LIPO; Output → PICO_VSYS | **Stable Supply.** Converts variable input voltage to a regulated **+5V** output. |
| **C2** | 0.33µF | LM7805 Input to GND | **Input Decoupling.** Ensures the regulator is stable by filtering high-frequency noise from the input. |
| **C3** | 0.1µF | LM7805 Output to GND | **Output Decoupling.** Improves the transient response and stability of the regulated **+5V** output. |
| **C4** | 470µF | PICO_VSYS to GND | **Bulk Stabilization.** Acts as a **charge reservoir** to prevent the **+5V** rail from sagging or browning out when the LiPo voltage temporarily drops (sags) due to high current motor draw. |

---

## ⚖️ II. Voltage Division & Scaling Block

This section safely reduces the high LiPo voltage to a low-voltage signal (V_ADC, max ≈ 2.93V) suitable for the microcontroller's ADC.

| Component | Value | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **R_TOP** | **3.3kΩ** | LiPo + to Divider Output | **Voltage Reduction.** Scales the max 12.6V down to a safe 2.93V max, protecting the 3.3V ADC input. |
| **R_BOT** | **1kΩ** | Divider Output to GND | **Voltage Reduction.** Forms the lower part of the divider. |
| **Ratio** | 1.0 / 4.3 ≈ 0.23256 | N/A | **Scaling Factor.** The actual LiPo voltage is **4.3 × V_ADC**. |

---

## 🛡️ III. Buffering & Signal Conditioning Block

This section isolates the divider, guarantees signal accuracy, and filters noise before the signal reaches the Pico's ADC.

| Component | Value/Type | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **R1** | 100Ω | Divider Output to Op-Amp + | **Input Protection.** Limits fault current into the sensitive input stage of the **MCP6002** during overvoltage events. |
| **MCP6002** | Op-Amp (RRO) | Wired as Voltage Follower | **Accuracy Buffer.** Provides high input impedance and **Rail-to-Rail Output (RRO)** ensures accurate measurement down to **0V** (low battery). |
| **C1** | 0.1µF | Op-Amp VDD to VSS | **Op-Amp Decoupling.** Filters high-frequency noise from the PICO_VSYS supply for a clean reference. |
| **R2** | 100Ω | Op-Amp Output to C5/PICO_ADC | **Output Isolation & Filter Resistor.** Protects the Op-Amp output. Acts as the series resistor for the final **RC** Low-Pass Filter. |
| **C5** | 1.0µF | PICO_ADC line to GND | **Low-Pass Filter Capacitor.** Forms an **RC** LPF with R2 (f_c ≈ 1.59kHz) to suppress very high-frequency noise from the ADC signal. |
| **PICO_ADC** | Output Pin | Final Filter Output | **Signal Output.** Provides the clean, buffered, scaled, and noise-filtered voltage signal to the microcontroller's ADC pin. |