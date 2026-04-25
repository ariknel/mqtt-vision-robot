# 🤖 ESP32 Line Sensing Robot

A line-following robot built around the **ESP32-DEVKITC**, featuring dual motor control via L298N, 3 IR line sensors, 3 ultrasonic distance sensors, and an LM2596S-ADJ buck converter powered by a 2S LiPo (8.8V).

---

## 📋 Table of Contents

- [Hardware Overview](#hardware-overview)
- [Pinout Reference](#pinout-reference)
- [Power System](#power-system)
- [Sensors](#sensors)
- [Motor Control](#motor-control)
- [Schematic Notes](#schematic-notes)
- [Known Issues & Planned Fixes](#known-issues--planned-fixes)
- [Build Log](#build-log)

---

## Hardware Overview

| Component | Part | Notes |
|-----------|------|-------|
| MCU | ESP32-DEVKITC | Dual-core, WiFi/BT capable |
| Motor Driver | L298N | 4-channel, dual H-bridge |
| Buck Converter | LM2596S-ADJ | 8.8V → 5V, set via RV1 trimpot |
| Power Input | 2S LiPo | 8.8V via JST J3 connector |
| Line Sensors | IR Sensor x3 | ⏳ To be added to schematic |
| Distance Sensors | HC-SR04 x3 | ⏳ To be added to schematic |

---

## Pinout Reference

### Motor Driver — L298N

| L298N Pin | ESP32 GPIO | Type | Notes |
|-----------|-----------|------|-------|
| ENA | GPIO33 | PWM Output | Motor A speed control — moved from GPIO12 (strapping pin conflict) |
| IN1 | GPIO13 | Digital Output | Motor A direction |
| IN2 | GPIO14 | Digital Output | Motor A direction |
| ENB | GPIO25 | PWM Output | Motor B speed control |
| IN3 | GPIO27 | Digital Output | Motor B direction |
| IN4 | GPIO26 | Digital Output | Motor B direction |

### IR Line Sensors — ⏳ To Be Wired

| Sensor | ESP32 GPIO | Type | Notes |
|--------|-----------|------|-------|
| IR Left | TBD | Digital/Analog Input | Avoid ADC2 pins if WiFi used |
| IR Center | TBD | Digital/Analog Input | Avoid ADC2 pins if WiFi used |
| IR Right | TBD | Digital/Analog Input | Avoid ADC2 pins if WiFi used |

> ⚠️ **ADC2 Warning:** GPIOs 0, 2, 4, 12–15, 25–27 use ADC2 which is **disabled when WiFi is active**. Use ADC1 pins (GPIO32–39) for analog IR readings if WiFi is needed.

### Ultrasonic Sensors — ⏳ To Be Wired

| Sensor | TRIG GPIO | ECHO GPIO | Notes |
|--------|----------|----------|-------|
| Ultrasonic Left | TBD | TBD | ECHO needs 3.3V logic — use voltage divider on ECHO pin |
| Ultrasonic Center | TBD | TBD | ECHO needs 3.3V logic — use voltage divider on ECHO pin |
| Ultrasonic Right | TBD | TBD | ECHO needs 3.3V logic — use voltage divider on ECHO pin |

> ⚠️ **Logic Level Warning:** HC-SR04 ECHO pin outputs 5V. ESP32 GPIO is **3.3V max**. Always use a voltage divider (1kΩ / 2kΩ) or logic level shifter on each ECHO line.

### Battery Monitor

| Function | ESP32 GPIO | Notes |
|----------|-----------|-------|
| Battery ADC | TBD (ADC1) | R1/R2 voltage divider on J3 Pin 2 (8.8V) |

### JST Power Connector — J3

| Pin | Signal |
|-----|--------|
| 1 | GND |
| 2 | 8.8V (LiPo input) |

---

## Power System

```
[2S LiPo 8.8V]
      │
     J3 (JST)
      │
      ├──────────────────────────── L298N VS pin (motor supply, 8.8V)
      │
   LM2596S-ADJ
   (RV1 trimpot sets output)
      │
     5V rail
      │
      └──────────────────────────── ESP32-DEVKITC VIN
```

- **LM2596S-ADJ** output voltage set by trimpot **RV1**
- Output capacitor: 220µF (low-ESR recommended)
- Input capacitor: 220µF bulk + 100nF ceramic (planned)
- Freewheeling diodes on all L298N outputs

---

## Sensors

### IR Line Sensors (x3)
> ⏳ Not yet added to schematic

3 IR reflectance sensors positioned underneath the robot chassis for line detection. Will output either digital (HIGH/LOW) or analog values depending on sensor module chosen.

**GPIO allocation notes:**
- Use **ADC1** pins (GPIO32–39) for analog reading
- GPIO34, 35, 36, 39 are **input-only** — suitable for sensors, not outputs

### Ultrasonic Sensors HC-SR04 (x3)
> ⏳ Not yet added to schematic

3 ultrasonic sensors for obstacle detection (left, center, right). Requires 6 GPIOs total (3x TRIG + 3x ECHO).

**Required per sensor:**
- TRIG: any digital output GPIO
- ECHO: needs **3.3V logic level shifting** (HC-SR04 outputs 5V on ECHO)
  - Voltage divider: 1kΩ (top) + 2kΩ (bottom) between ECHO and GND, tap to GPIO

---

## Motor Control

Dual H-bridge via **L298N**. Each motor channel is independently controllable for direction and speed.

| Channel | Enable (PWM) | Dir A | Dir B |
|---------|-------------|-------|-------|
| Motor A | GPIO33 | GPIO13 | GPIO14 |
| Motor B | GPIO25 | GPIO27 | GPIO26 |

**Direction logic:**

| IN1 | IN2 | Motor A |
|-----|-----|---------|
| HIGH | LOW | Forward |
| LOW | HIGH | Reverse |
| LOW | LOW | Coast |
| HIGH | HIGH | Brake |

---

## Schematic Notes

### Rev 1 — April 2026
- R1/R2 on schematic are **battery voltage monitor** (ADC divider), NOT part of LM2596 feedback network
- LM2596 feedback set entirely by RV1 trimpot — functional but see planned fixes below
- ENA originally on GPIO12 (strapping pin) — **corrected to GPIO33**

---

## Known Issues & Planned Fixes

| Priority | Issue | Status |
|----------|-------|--------|
| 🔴 Critical | LM2596 FB trimpot-only risk: wiper failure could send 8.8V to ESP32 | ⏳ Planned fix |
| 🔴 Critical | Planned fix: add R_upper 1kΩ (VOUT→FB) + R_lower 1kΩ (FB→GND), keep RV1 in series with R_lower | ⏳ Next revision |
| 🔴 Fixed | ENA on GPIO12 (strapping pin) — moved to GPIO33 | ✅ Fixed |
| 🟡 Warning | HC-SR04 ECHO pins need 3.3V level shifting before connecting to ESP32 | ⏳ To be added |
| 🟡 Warning | Use Schottky diodes on L298N outputs (not 1N4007) | 🔍 To verify |
| 🟡 Warning | Low-ESR cap recommended on LM2596 output | 🔍 To verify |
| 🟡 Warning | 100nF ceramic decoupling caps needed on ESP32 VCC pins | ⏳ Planned |
| 🟡 Warning | 100nF ceramic in parallel with input bulk cap (220µF1) | ⏳ Planned |
| 🟢 OK | JST J3: Pin1=GND, Pin2=8.8V confirmed | ✅ |
| 🟢 OK | L298N VS pin on 8.8V rail (motor supply) | ✅ To verify |
| 🟢 OK | R1/R2 voltage divider confirmed as battery ADC monitor | ✅ |

### LM2596 Planned Fix Detail

Current state: RV1 trimpot alone sets 5V output — works but risky.

```
VOUT ── R_upper (1kΩ) ── FB ── RV1 (1kΩ trim) ── R_lower (1kΩ) ── GND
```

With fixed resistors as base, a wiper failure cannot cause a catastrophic voltage spike. Formula: `Vout = 1.23 × (1 + R_lower_total / R_upper)`

---

## Build Log

| Date | Entry |
|------|-------|
| Apr 2026 | Rev 1 schematic completed. ESP32 + LM2596 + L298N architecture established. |
| Apr 2026 | Schematic reviewed — GPIO12 strapping conflict on ENA found and fixed → GPIO33. |
| Apr 2026 | Confirmed R1/R2 are battery monitor divider, not LM2596 feedback. |
| Apr 2026 | IR sensors (x3) and ultrasonic sensors (x3) to be added to schematic next. |

---

## 📌 GPIO Quick Reference — Remaining Available

Safe GPIOs still free for IR + ultrasonic sensors:

| GPIO | ADC | PWM | Notes |
|------|-----|-----|-------|
| GPIO32 | ADC1 ✅ | ✅ | Good for IR analog |
| GPIO33 | — | ✅ | **Used: ENA** |
| GPIO34 | ADC1 ✅ | ❌ | Input only — good for ECHO |
| GPIO35 | ADC1 ✅ | ❌ | Input only — good for ECHO |
| GPIO36 | ADC1 ✅ | ❌ | Input only — good for ECHO |
| GPIO39 | ADC1 ✅ | ❌ | Input only — good for IR analog |
| GPIO21 | — | ✅ | General purpose |
| GPIO22 | — | ✅ | General purpose |
| GPIO23 | — | ✅ | General purpose |

> 💡 **Suggested allocation:** Use GPIO34/35/36 for the 3x ECHO pins (input-only, 3.3V safe after divider), GPIO32/39 + one more ADC1 pin for IR sensors, remaining GPIOs for TRIG pins.
