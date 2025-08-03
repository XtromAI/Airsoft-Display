# Airsoft Computer Wiring Guide

**Date:** August 2, 2025  
**Project:** Minimal Airsoft Computer  
**Hardware:** Raspberry Pi Pico + SH1107 OLED Display

---

## Pin Configuration Summary

| Component | Signal | Pico Pin | GPIO | Notes |
|-----------|--------|----------|------|-------|
| **SH1107 Display** | SCL (Clock) | Pin 19 | GP14 | SPI Clock |
| | SDA (Data) | Pin 20 | GP15 | SPI MOSI |
| | DC (Data/Command) | Pin 27 | GP21 | Command/Data select |
| | CS (Chip Select) | Pin 17 | GP13 | SPI Chip Select |
| | RST (Reset) | Pin 26 | GP20 | Display Reset |
| | VCC | 3V3 | - | 3.3V Power |
| | GND | GND | - | Ground |
| **Button** | Terminal 1 | Pin 12 | GP9 | Output (drives low) |
| | Terminal 2 | Pin 14 | GP10 | Input (pull-up) |
| **Battery Monitor** | ADC Input | Pin 31 | GP26 | ADC0 |
| **Status LED** | Onboard LED | Pin 30 | GP25 | Built-in LED |

---

## Detailed Wiring Instructions

### SH1107 OLED Display (SPI Mode)

The display uses SPI communication with additional control pins:

```
SH1107 Display    →    Raspberry Pi Pico
───────────────────────────────────────
VCC               →    3V3 (Pin 36)
GND               →    GND (Pin 38)
SCL (Clock)       →    GP14 (Pin 19)
SDA (Data)        →    GP15 (Pin 20)
DC (Data/Cmd)     →    GP21 (Pin 27)
CS (Chip Select)  →    GP13 (Pin 17)
RST (Reset)       →    GP20 (Pin 26)
```

**Important Notes:**
- Ensure display is set to SPI mode (not I2C)
- VCC should be 3.3V, not 5V
- All control signals are 3.3V logic level

### Button (Single Push Button)

The button is wired across two GPIO pins for reliable detection:

```
Button Terminal 1  →    GP9 (Pin 12)  [Output - drives LOW]
Button Terminal 2  →    GP10 (Pin 14) [Input - pull-up enabled]
```

**How it works:**
- GP9 continuously outputs LOW (0V)
- GP10 is configured as input with internal pull-up (reads HIGH normally)
- When button is pressed: GP9 and GP10 connect, GP10 reads LOW
- Software detects button press when GP10 transitions from HIGH to LOW

### Battery Voltage Monitor

For monitoring airsoft gun battery voltage:

```
Battery Voltage Divider  →    GP26 (Pin 31) [ADC0]
```

**Voltage Divider Circuit:**
```
Battery+ ──[R1]──┬──[R2]── Battery-
                 │
                 └── GP26 (ADC Input)
```

- R1 = R2 for 2:1 voltage division
- Maximum input: 6.6V battery (3.3V at ADC)
- Code applies 2x multiplier to get actual battery voltage

### Status LED

Uses the built-in LED on the Pico:

```
Onboard LED  →    GP25 (Pin 30) [Built-in]
```

- No external wiring required
- LED blinks to indicate system status
- 2Hz blink rate during normal operation

---

## Power Supply

- **Pico Power:** USB 5V or VSYS pin
- **Display Power:** 3.3V from Pico 3V3 pin
- **Current Draw:** ~50mA total (Pico + Display)

---

## Pinout Reference

```
         Raspberry Pi Pico Pinout
    ┌─────────────────────────────────┐
    │  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○  │
    │  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○  │
    └─────────────────────────────────┘
       1     5     10    15    20

Pin Assignments:
- Pin 12 (GP9):  Button Output
- Pin 14 (GP10): Button Input  
- Pin 17 (GP13): Display CS
- Pin 19 (GP14): Display SCL
- Pin 20 (GP15): Display SDA
- Pin 26 (GP20): Display RST
- Pin 27 (GP21): Display DC
- Pin 30 (GP25): Status LED
- Pin 31 (GP26): Battery ADC
- Pin 36: 3V3 Power
- Pin 38: Ground
```

---

## Testing

### Display Test Sequence

Flash `display_test.uf2` to verify display functionality:

1. **LED flashes 5 times** (system startup)
2. **Clear screen** (2 seconds)
3. **Checkerboard pattern** (3 seconds)
4. **"HELLO WORLD 12345"** text (3 seconds)
5. **Full screen "AB" pattern** (3 seconds)
6. **Continuous LED blinking** (system ready)

### Button Test

During display test:
- Press button to restart test sequence
- Monitor serial output (115200 baud) for "Button pressed!" message

### Main Application

Flash `airsoft_computer.uf2` for full functionality:
- Displays battery voltage, shot count, baseline voltage, sample rate
- Button resets shot counter
- LED blinks at 2Hz during operation

---

## Troubleshooting

### Display Issues

1. **Nothing on display:**
   - Check all 7 display connections
   - Verify 3.3V power supply
   - Ensure display is SPI mode, not I2C

2. **Garbled display:**
   - Check SCL/SDA connections
   - Verify DC and CS connections
   - Try different SPI clock speed

### Button Issues

1. **Button not responding:**
   - Check GP9 and GP10 connections
   - Verify button makes good contact
   - Monitor serial output for debug messages

2. **False button presses:**
   - Check for loose connections
   - Verify pull-up is working
   - Increase debounce time in software

### ADC Issues

1. **Incorrect voltage readings:**
   - Check voltage divider resistor values
   - Verify GP26 connection
   - Calibrate voltage multiplier in code

---

## Software Configuration

The pin assignments are defined in the code:

```cpp
// Pin definitions
#define BATTERY_PIN 26      // ADC to measure battery voltage - GP26
#define BUTTON_OUT_PIN  9   // Button output pin - GP9 (drives low)
#define BUTTON_IN_PIN  10   // Button input pin - GP10 (reads state)
#define LED_PIN     25      // Onboard LED

// Display pins (in display.h)
#define PIN_MOSI 15         // SDA - GP15
#define PIN_SCK  14         // SCL - GP14
#define PIN_CS   13         // CS - GP13
#define PIN_DC   21         // DC - GP21
#define PIN_RST  20         // RST - GP20
```

---

## Bill of Materials

| Component | Quantity | Notes |
|-----------|----------|-------|
| Raspberry Pi Pico | 1 | Microcontroller |
| SH1107 OLED Display | 1 | 128x128 pixel, SPI mode |
| Push Button | 1 | Momentary contact |
| Resistors (for voltage divider) | 2 | Equal values, e.g., 10kΩ |
| Jumper Wires | ~10 | For connections |
| Breadboard/PCB | 1 | For assembly |

---

## Revision History

- **v1.0** (Aug 2, 2025): Initial wiring guide with corrected pin assignments
