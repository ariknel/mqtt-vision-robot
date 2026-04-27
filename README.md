# 🤖 ESP32 Line Sensing Robot

A line-following robot built around the **ESP32-DEV (DEVKITC V1)**, featuring dual motor control via L298N, 3 IR line sensors, 3 ultrasonic distance sensors, and an LM2596S-ADJ buck converter powered by a 2S LiPo (8.8V). Controlled via a custom **Android MQTT app** built in Android Studio, with full telemetry monitoring and dual control modes (accelerometer tilt + WASD buttons).

---

## 📋 Table of Contents

- [Hardware Overview](#hardware-overview)
- [PCB Layout](#pcb-layout)
- [Pinout Reference](#pinout-reference)
- [JST Connector Wiring](#jst-connector-wiring)
- [Power System](#power-system)
- [Sensors](#sensors)
- [Motor Control](#motor-control)
- [Android App & MQTT Control](#android-app--mqtt-control)
- [Schematic Notes](#schematic-notes)
- [Known Issues & Planned Fixes](#known-issues--planned-fixes)
- [Build Log](#build-log)
- [Schematic](#schematic)

---

## Hardware Overview

| Component | Part | Notes |
|-----------|------|-------|
| MCU | ESP32-DEV (DEVKITC V1) | Dual-core, WiFi/BT capable |
| Motor Driver | L298N | Repurposed from L298N module — correct Schottky diodes confirmed ✅ |
| Buck Converter | LM2596S-ADJ | 8.8V → 5V, set via RV1 trimpot. Diode: SS34 Schottky ✅ |
| Power Input | 2S LiPo | 8.8V via JST J3 (2-pin) connector |
| Line Sensors | IR Sensor x3 | Digital output, 3.3V — via JST-B (10-pin) |
| Distance Sensors | HC-SR04 x3 | 2021+ version — 3.3V compatible ECHO, no voltage divider needed ✅ |
| Control App | Android (Android Studio) | MQTT-based, accelerometer + WASD control |

> ℹ️ **L298N Diodes:** Components repurposed from L298N module — correct Schottky freewheeling diodes confirmed, no substitution needed.
> ℹ️ **HC-SR04 Version:** 2021+ modules confirmed — ECHO pin outputs 3.3V logic, connects directly to ESP32 GPIO ✅

---

## PCB Layout

> 📷 *PCB layout image — to be added*

---


## PCB Layout

![PCB Layout](pcb_layout.png)

*KiCad PCB layout — ESP32-DEVKITC V1, LM2596S-ADJ power section, L298N motor driver, JST-A ultrasonic connector, JST-B IR sensor connector.*

---
## Pinout Reference

### Motor Driver — L298N

| L298N Pin | Label | ESP32 Pin | Type | Notes |
|-----------|-------|-----------|------|-------|
| IN1 | D13 | Pin 28 | Digital Output | Motor A direction |
| IN2 | D14 | Pin 26 | Digital Output | Motor A direction |
| EnA | D21 | Pin 11 | PWM Output | Motor A speed control |
| IN3 | D27 | Pin 25 | Digital Output | Motor B direction |
| IN4 | D26 | Pin 24 | Digital Output | Motor B direction |
| EnB | D25 | Pin 23 | PWM Output | Motor B speed control |

### Battery Monitor

| Function | Label | ESP32 Pin | Notes |
|----------|-------|-----------|-------|
| Battery ADC | D34 | Pin 19 | R1/R2 voltage divider from J3 Pin 2 (8.8V). Input-only, ADC1 — WiFi safe ✅ |

### IR Line Sensors — JST-B (10-pin)

| Sensor | Label | ESP32 Pin | Type | Notes |
|--------|-------|-----------|------|-------|
| IR Left | D32 | Pin 21 | Digital Input | ADC1 — WiFi/MQTT safe ✅ |
| IR Center | D33 | Pin 22 | Digital Input | ADC1 — WiFi/MQTT safe ✅ |
| IR Right | D15 | Pin 3 | Digital Input | Mild strapping pin — IR defaults LOW at boot, no issue ✅ |

### Ultrasonic Sensors HC-SR04 (2021+) — JST-A (10-pin)

| Sensor | TRIG Label | TRIG Pin | ECHO Label | ECHO Pin | Notes |
|--------|-----------|----------|-----------|----------|-------|
| Ultrasonic Left | D22 | Pin 14 | D35 | Pin 20 | ECHO input-only ADC1 ✅ |
| Ultrasonic Center | D23 | Pin 15 | D19 | Pin 10 | Direct connection ✅ |
| Ultrasonic Right | D18 | Pin 9 | D4 | Pin 5 | D4 mild strapping — ECHO is input, defaults safe ✅ |

> ✅ HC-SR04 2021+ version: ECHO outputs 3.3V logic — direct connection to ESP32, no voltage divider needed.

### JST Power Connector — J3 (2-pin JST)

| Pin | Signal |
|-----|--------|
| 1 | GND |
| 2 | 8.8V (LiPo input) |

---

## JST Connector Wiring

### JST-A — Ultrasonic Sensors HC-SR04 2021+ (10-pin JST)

| Pin | Signal | Label | ESP32 Pin | Notes |
|-----|--------|-------|-----------|-------|
| 1 | 5V shared | — | — | 5V rail — HC-SR04 VCC |
| 2 | GND shared | — | GND | |
| 3 | TRIG Left | D22 | Pin 14 | Digital output |
| 4 | ECHO Left | D35 | Pin 20 | Direct connection ✅ |
| 5 | TRIG Center | D23 | Pin 15 | Digital output |
| 6 | ECHO Center | D19 | Pin 10 | Direct connection ✅ |
| 7 | TRIG Right | D18 | Pin 9 | Digital output |
| 8 | ECHO Right | D4 | Pin 5 | Direct connection ✅ |
| 9 | — spare — | — | — | |
| 10 | — spare — | — | — | |

### JST-B — IR Line Sensors (10-pin)

| Pin | Signal | Label | ESP32 Pin | Notes |
|-----|--------|-------|-----------|-------|
| 1 | 3.3V shared | — | — | 3V3 rail |
| 2 | GND shared | — | GND | |
| 3 | IR Left | D32 | Pin 21 | Digital input, ADC1 |
| 4 | IR Center | D33 | Pin 22 | Digital input, ADC1 |
| 5 | IR Right | D15 | Pin 3 | Digital input |
| 6–10 | — spare — | — | — | Room to expand |

---

## Power System

```
[2S LiPo 8.8V]
      │
    J3 JST (2-pin)
      │
      ├─── R1/R2 voltage divider ─── D34 Pin 19 (battery monitor ADC)
      │
      ├─── 220uF1 bulk capacitor (input filter)
      │
      ├─── L298N VS pin (motor supply, 8.8V direct)
      │
   LM2596S-ADJ
   D1: SS34 Schottky
   RV1 trimpot → FB pin (sets 5V output)
   Output: 220uF4 capacitor
      │
     5V rail
      ├─── HC-SR04 VCC (via JST-A Pin 1)
      │
   ESP32-DEV VIN → onboard LDO → 3.3V rail
                                      │
                                      └─── IR sensors VCC (via JST-B Pin 1)
```

---

## Sensors

### IR Line Sensors (x3)

3 IR reflectance sensors under chassis for line detection. Digital output, 3.3V modules.

- All on ADC1 — WiFi/MQTT always active ✅
- No level shifting needed ✅
- Connected via JST-B

**MQTT telemetry:** `robot/telemetry/ir`
```json
{"left": 0, "center": 1, "right": 0}
```

### Ultrasonic Sensors HC-SR04 2021+ (x3)

3 ultrasonic sensors for obstacle detection. 2021+ version — ECHO outputs 3.3V logic.

- Powered from 5V rail
- TRIG: 3.3V ESP32 output sufficient ✅
- ECHO: direct connection, no divider needed ✅

**MQTT telemetry:** `robot/telemetry/ultrasonic`
```json
{"left": 24, "center": 8, "right": 31}
```

---

## Motor Control

Dual H-bridge via L298N. Speed via PWM on EnA/EnB, direction via IN1–IN4.

| Channel | Enable (PWM) | Dir Pin A | Dir Pin B |
|---------|-------------|-----------|-----------|
| Motor A | D21 Pin 11 (EnA) | D13 Pin 28 (IN1) | D14 Pin 26 (IN2) |
| Motor B | D25 Pin 23 (EnB) | D27 Pin 25 (IN3) | D26 Pin 24 (IN4) |

**Direction logic:**

| IN1 | IN2 | Motor A |
|-----|-----|---------|
| HIGH | LOW | Forward |
| LOW | HIGH | Reverse |
| LOW | LOW | Coast |
| HIGH | HIGH | Brake |

---

## Android App & MQTT Control

Custom Android app (Android Studio) communicates with the ESP32 over MQTT. The broker runs **embedded inside the APK** — no external server needed.

### Architecture

```
[Android App]
      ├── Embedded MQTT Broker (Moquette / HiveMQ, port 1883)
      └── MQTT Client
                │
           Local WiFi
                │
        [ESP32-DEV DEVKITC V1]
                │
         Motors / Sensors
```

> The phone acts as both broker and client. The ESP32 connects to the broker using the phone's local IP over shared WiFi. No cloud dependency, no external setup.

### MQTT Topic Structure

| Topic | Direction | Description |
|-------|-----------|-------------|
| `robot/control/move` | App → ESP32 | `forward` / `back` / `left` / `right` / `stop` |
| `robot/control/speed` | App → ESP32 | PWM value 0–255 |
| `robot/control/mode` | App → ESP32 | `manual` or `line_follow` |
| `robot/telemetry/ir` | ESP32 → App | IR sensor states JSON |
| `robot/telemetry/ultrasonic` | ESP32 → App | Distance readings JSON (cm) |
| `robot/telemetry/battery` | ESP32 → App | Battery voltage float (V) |
| `robot/telemetry/speed` | ESP32 → App | PWM values both motors |

### Control Modes

Toggled by a single button in the app UI. Accelerometer listener registered/unregistered on toggle to preserve battery.

**Mode 1 — Accelerometer tilt:**

| Tilt | Robot Action |
|------|-------------|
| Forward | Move forward |
| Backward | Reverse |
| Left | Steer left |
| Right | Steer right |
| Flat | Stop |

**Mode 2 — WASD buttons:**
```
      [ W ]
 [ A ][ S ][ D ]
      [STP]
```

### Telemetry Dashboard

| Widget | MQTT Topic | Notes |
|--------|-----------|-------|
| IR indicators | `robot/telemetry/ir` | 3 visual indicators — line / no line |
| Ultrasonic distances | `robot/telemetry/ultrasonic` | 3 live readouts in cm |
| Battery voltage | `robot/telemetry/battery` | Live voltage + low battery warning |
| Motor speed | `robot/telemetry/speed` | PWM values both channels |
| Connection status | Internal | Broker + ESP32 link indicator |

### ESP32 Firmware — MQTT Overview

```cpp
#include <WiFi.h>
#include <PubSubClient.h>

const char* mqtt_server = "PHONE_LOCAL_IP";

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = String((char*)payload).substring(0, length);
  if (String(topic) == "robot/control/move")  handleMove(msg);
  if (String(topic) == "robot/control/speed") setSpeed(msg.toInt());
  if (String(topic) == "robot/control/mode")  setMode(msg);
}

void publishTelemetry() {
  client.publish("robot/telemetry/battery",    String(batteryVoltage).c_str());
  client.publish("robot/telemetry/ir",         irStateJson().c_str());
  client.publish("robot/telemetry/ultrasonic", ultrasonicJson().c_str());
  client.publish("robot/telemetry/speed",      speedJson().c_str());
}
```

---

## Schematic Notes

### Rev 1 — 24 April 2026
- R1/R2 = battery voltage monitor ADC divider — NOT part of LM2596 feedback network
- LM2596 feedback set by RV1 trimpot — functional, fix planned for next revision
- EnA originally on GPIO12 (strapping pin conflict) — corrected to D21
- L298N repurposed from module — correct Schottky diodes confirmed ✅
- LM2596 diode D1 = SS34 Schottky ✅

### Rev 2 — 25 April 2026
- JST-A (10-pin) added for 3x HC-SR04 ultrasonic sensors
- JST-B (10-pin) added for 3x IR line sensors
- HC-SR04 confirmed as 2021+ version — no voltage dividers needed on ECHO lines ✅
- Full GPIO assignments finalised and conflict-checked

### Rev 3 — 27 April 2026
- Corrected ESP32 footprint from ESP32-WROOM-32D to ESP32-DEV DEVKITC V1
- All pin numbers updated to match DEVKITC V1 physical header layout
- ECHO Center changed from D36 (non-existent on DEVKITC V1) → D19 Pin 10
- ECHO Right changed from D39 (non-existent on DEVKITC V1) → D4 Pin 5
- Full pinout re-verified against DEVKITC V1 — no conflicts found ✅

---

## Known Issues & Planned Fixes

| Priority | Issue | Status |
|----------|-------|--------|
| 🔴 Critical | LM2596 trimpot-only FB risk — wiper failure could send 8.8V to ESP32. Fix: add R_upper 1kΩ (VOUT→FB) + R_lower 1kΩ (FB→GND), RV1 in series with R_lower for safe fine tuning | ⏳ Next revision |
| 🟡 Warning | Low-ESR capacitor recommended on LM2596 220uF output | 🔍 To verify |
| 🟢 OK | L298N diodes — repurposed from module, correct Schottky type ✅ | ✅ |
| 🟢 OK | LM2596 D1 = SS34 Schottky ✅ | ✅ |
| 🟢 OK | HC-SR04 2021+ — ECHO direct to GPIO, no voltage divider needed ✅ | ✅ |
| 🟢 OK | All IR GPIOs on ADC1 — WiFi/MQTT safe ✅ | ✅ |
| 🟢 OK | No strapping pin conflicts in final pinout ✅ | ✅ |
| 🟢 OK | D34 battery ADC — ADC1, input-only, WiFi safe ✅ | ✅ |
| 🟢 OK | J3 JST: Pin1=GND, Pin2=8.8V ✅ | ✅ |
| 🟢 OK | 100nF decoupling on ESP32 VCC not added — ESP32-DEVKITC has decoupling caps built onto the module ✅ | ✅ |
| 🟢 OK | D36/D39 non-existent on DEVKITC V1 — replaced with D19/D4 ✅ | ✅ |

---

## Build Log

| Date | Entry |
|------|-------|
| 24 Apr 2026 | Rev 1 schematic complete. ESP32 + LM2596 + L298N architecture established. |
| 24 Apr 2026 | D12 strapping conflict on EnA found and corrected → D21. |
| 24 Apr 2026 | R1/R2 confirmed as battery ADC divider, not LM2596 feedback. |
| 24 Apr 2026 | L298N repurposed from module — correct Schottky diodes confirmed. |
| 24 Apr 2026 | Android MQTT app architecture defined — embedded broker, dual control modes, telemetry dashboard. |
| 25 Apr 2026 | JST-A (ultrasonic) and JST-B (IR sensors) connectors added to schematic and PCB. |
| 25 Apr 2026 | Full GPIO conflict check performed — all pins verified clean. |
| 25 Apr 2026 | HC-SR04 confirmed as 2021+ version — voltage dividers removed from design. |
| 25 Apr 2026 | Final GPIO assignments locked in for all motors, sensors, and battery ADC. |
| 25 Apr 2026 | LM2596 D1 = SS34 Schottky confirmed from schematic. |
| 25 Apr 2026 | Schematic Rev 2 exported — JST-A and JST-B fully connected, all sensor GPIO traces complete. |
| 27 Apr 2026 | PCB design completed. |
| 27 Apr 2026 | PCB diode orientation cleaned up — all 8 L298N freewheeling diodes set to consistent A/C orientation. DRC run to verify no electrical conflicts. |
| 27 Apr 2026 | Discovered incorrect ESP32 footprint — was using ESP32-WROOM-32D. Corrected to ESP32-DEV DEVKITC V1. |
| 27 Apr 2026 | Updated to correct ESP32-DEVKITC V1 symbol and footprint. Footprint files uploaded to GitHub repo. |
| 27 Apr 2026 | All pin numbers updated to DEVKITC V1 physical header layout. D36/D39 non-existent on this board — replaced with D19 (Pin 10) and D4 (Pin 5). |
| 27 Apr 2026 | Full pinout re-verified against DEVKITC V1 — 16 GPIOs used, 0 conflicts, 9 spare pins remaining. |
| 27 Apr 2026 | Started Android app development — planning and working out details (to be discussed further). |

---

## 📌 Complete GPIO Map

| Label | ESP32 Pin | Function | Type | Notes |
|-------|-----------|----------|------|-------|
| D4 | Pin 5 | ECHO Right | Input | JST-A ultrasonic |
| D13 | Pin 28 | Motor A IN1 | Output | L298N |
| D14 | Pin 26 | Motor A IN2 | Output | L298N |
| D15 | Pin 3 | IR Right | Input | JST-B |
| D18 | Pin 9 | TRIG Right | Output | JST-A ultrasonic |
| D19 | Pin 10 | ECHO Center | Input | JST-A ultrasonic |
| D21 | Pin 11 | EnA Motor A | PWM Output | L298N |
| D22 | Pin 14 | TRIG Left | Output | JST-A ultrasonic |
| D23 | Pin 15 | TRIG Center | Output | JST-A ultrasonic |
| D25 | Pin 23 | EnB Motor B | PWM Output | L298N |
| D26 | Pin 24 | Motor B IN4 | Output | L298N |
| D27 | Pin 25 | Motor B IN3 | Output | L298N |
| D32 | Pin 21 | IR Left | Input | ADC1, JST-B |
| D33 | Pin 22 | IR Center | Input | ADC1, JST-B |
| D34 | Pin 19 | Battery ADC | Input | ADC1, input-only, R1/R2 divider |
| D35 | Pin 20 | ECHO Left | Input | ADC1, input-only, JST-A |

---

## Schematic

### Full Schematic — Rev 2

![Full Schematic](schematic_full.png)

*ESP32-DEV DEVKITC V1 + LM2596S-ADJ power section + L298N motor driver + JST-A ultrasonic connector + JST-B IR sensor connector*

---

### LM2596S-ADJ — Buck Converter & Battery Monitor

![LM2596 Section](schematic_lm2596.png)

*8.8V input via J3 JST (2-pin). R1/R2 voltage divider for battery ADC on D34. LM2596S-ADJ with SS34 Schottky diode D1, inductor L, 220µF output capacitor, and RV1 trimpot setting 5V output via FB pin.*

---

### L298N — Motor Driver

![L298N Section](schematic_l298n.png)

*L298N dual H-bridge with Schottky freewheeling diodes on all outputs. Motor A: IN1 (D13), IN2 (D14), EnA (D21). Motor B: IN3 (D27), IN4 (D26), EnB (D25). Outputs to J1 and J2 screw terminals.*
