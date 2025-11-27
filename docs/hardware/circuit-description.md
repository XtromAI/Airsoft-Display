# 🎯 Airsoft Display Battery Monitoring Circuit Description

This document describes the electronic schematic for a robust and safe airsoft display system. The circuit provides stable $5\text{V}$ power while accurately monitoring the LiPo battery voltage for level and shot detection. Two regulator options are documented: the **LM7805 Linear Regulator** (for prototyping) and the **MP1584 Buck Module** (recommended for final builds).

---

## 🛡️ I. Input Protection Block

This section provides crucial physical and electrical fault protection for the entire circuit, connected directly to the LiPo battery.

| Component | Value/Type | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **F1** | PPTC Fuse | Series with LiPo + | **Overcurrent Protection.** A self-resetting fuse that trips to high resistance during sustained short circuits, protecting the rest of the board. |
| **D\_RP** | Schottky Diode | Series after F1 | **Reverse Polarity Protection.** Blocks current flow if the battery is accidentally plugged in backward. The forward voltage drop ($\mathbf{V_F} \approx 0.4\text{V}$) must be added back during ADC calculation to find the true LiPo voltage. |

---

## ⚡️ II. Power Regulation Block (PICO\_VSYS) - LM7805 Option

> ⚠️ **Note:** The LM7805 linear regulator has low efficiency (40-60%) and is not recommended for final builds. Use the MP1584 Buck Module (Section IIa) instead for better power efficiency.

This section converts the variable LiPo voltage (after $\text{D}_{\text{RP}}$) into a stable $\mathbf{+5\text{V}}$ supply, addressing both stability and transient load concerns.

| Component | Value/Type | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **LM7805** | Linear Regulator | Input $\to \text{LIPO}$; Output $\to \text{PICO\_VSYS}$ | **Stable Supply.** Converts variable input voltage to a regulated $\mathbf{+5\text{V}}$ output. |
| **C2** | $0.33\mu\text{F}$ | LM7805 Input to $\text{GND}$ | **Input Decoupling.** Ensures the regulator is stable and filters high-frequency noise from the input line. |
| **C3** | $0.1\mu\text{F}$ | LM7805 Output to $\text{GND}$ | **Output Decoupling.** Improves the transient response of the regulated $\mathbf{+5\text{V}}$ rail. |
| **D\_FLY** | Diode ($\text{1N4001}$) | LM7805 Output to Input | **Flyback Protection.** Prevents stored charge in $\text{C4}/\text{C3}$ from discharging backward through the regulator's ground pin during power-down. |
| **C4** | $1000\mu\text{F}$ | $\text{PICO\_VSYS}$ to $\text{GND}$ | **Bulk Stabilization.** Acts as a **charge reservoir** to prevent the $\mathbf{+5\text{V}}$ rail from sagging when the LM7805 briefly loses regulation due to the motor's high current sag. |

---

## ⚡️ IIa. Power Regulation Block (PICO\_VSYS) - MP1584 Buck Module Option (Recommended)

This section describes the recommended regulator for final builds, using a high-efficiency switching buck converter module.

| Component | Value/Type | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **MP1584** | Buck Converter Module | IN $\to \text{V\_1a}$; G $\to \text{GND}$; OUT $\to \text{PICO\_VSYS}$ | **High-Efficiency Supply.** Converts variable input voltage to a regulated $\mathbf{+5\text{V}}$ output with high efficiency (up to 96%). |
| **C4** | $1000\mu\text{F}$ | $\text{PICO\_VSYS}$ to $\text{GND}$ | **Bulk Stabilization.** Acts as a **charge reservoir** to prevent voltage sag during motor current spikes. |

---

## 📈 III. Voltage Scaling Block

This section safely reduces the battery voltage (after $\text{D}_{\text{RP}}$) to a level measurable by the Pico's ADC.

| Component | Value | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **R\_TOP** | $\mathbf{3.3\text{k}\Omega}$ | LiPo Line to Divider Output | **Voltage Reduction.** Sets the scaling factor. This value ensures the maximum scaled voltage ($\approx 2.93\text{V}$) is safely below the Pico's $3.3\text{V}$ ADC limit. |
| **R\_BOT** | $\mathbf{1\text{k}\Omega}$ | Divider Output to $\text{GND}$ | **Voltage Reduction.** Completes the divider circuit. |
| **Ratio** | $1.0 / 4.3 \approx 0.23256$ | N/A | **Scaling Factor.** $\mathbf{V}_{\text{LIPO}} = (\mathbf{V}_{\text{ADC}} \times 4.3) + \mathbf{V_F}$. |

---

## 👂 IV. Buffering & Signal Conditioning Block

This final section isolates the divider, guarantees signal accuracy, and filters noise right before the ADC pin.

| Component | Value/Type | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **R-S** | $100\Omega$ | Divider Output to Op-Amp $+$ | **Input Protection.** Limits current into the sensitive input stage of the $\text{MCP6002}$ during faults. |
| **MCP6002** | Op-Amp (RRO) | Wired as Voltage Follower | **Accuracy Buffer.** Provides **ultra-high input impedance** (preventing loading of the divider) and **Rail-to-Rail Output (RRO)** for accurate measurement down to $\mathbf{0\text{V}}$. |
| **C1** | $0.1\mu\text{F}$ | Op-Amp $V_{DD}$ to $V_{SS}$ | **Op-Amp Decoupling.** Filters high-frequency noise from the $\text{PICO\_VSYS}$ supply for a clean reference. |
| **R\_A** | $100\Omega$ | Op-Amp Output to $\text{C5}/\text{PICO\_ADC}$ | **LPF Resistor & Output Isolation.** Protects the Op-Amp output and acts as the series resistor for the final low-pass filter. |
| **C5** | $1.0\mu\text{F}$ | $\text{PICO\_ADC}$ line to $\text{GND}$ | **Low-Pass Filter Capacitor.** Forms an $\mathbf{RC}$ LPF with R\_A ($\mathbf{f_c \approx 1.59\text{kHz}}$) to suppress high-frequency noise while preserving the slow shot sag signal. |
| **PICO\_ADC** | Analog Input | Final Filter Output | **Signal Output.** Provides the clean, buffered, scaled, and noise-filtered voltage signal to the microcontroller. |

---

## 🔘 V. AUX Switching Block

This section provides a simple auxiliary switch input for user interaction (e.g., mode selection, counter reset).

| Component | Value/Type | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **SW** | Momentary Switch | One terminal to $\text{GND}$ or signal | **User Input.** Provides a physical button for user interaction with the system. |
| **R** | $1\text{k}\Omega$ | Series between SW and GPIO | **Current Limiting.** Protects the GPIO pin from excessive current and provides a defined signal path. |
| **GPIO** | Digital Input | Connected via R | **Signal Input.** Reads the switch state (configure with internal pull-up/pull-down as needed). |

---

## 🔌 VI. External Signal Isolation Block (G\_EXT)

This section provides galvanic isolation for external trigger signals using an optocoupler, protecting the Pico from external voltage transients and ground loops.

| Component | Value/Type | Connections | Primary Role |
| :--- | :--- | :--- | :--- |
| **Optocoupler** | 4-pin (e.g., PC817) | LED side: Pins 1-2; Phototransistor side: Pins 3-4 | **Galvanic Isolation.** Electrically isolates the external signal source from the Pico circuit, preventing ground loops and protecting against voltage spikes. |
| **LED Input** | Pins 1-2 | External signal (G\_EXT) | **Signal Reception.** The internal LED is driven by the external trigger signal. |
| **R** | $470\Omega$ | Phototransistor output to $\text{GND}$ | **Pull-Down Resistor.** Provides a defined low state when the optocoupler is not activated and limits current through the phototransistor. |
| **GPIO** | Digital Input | Phototransistor collector | **Signal Output.** Reads the isolated external trigger signal for shot detection or other external events. |