# 🤖 ESP32 Line Sensing Robot

A line-following robot built around the **ESP32-DEVKITC**, featuring dual motor control via L298N, 3 IR line sensors, 3 ultrasonic distance sensors, and an LM2596S-ADJ buck converter powered by a 2S LiPo (8.8V). Controlled via a custom **Android MQTT app** built in Android Studio, with full telemetry monitoring and dual control modes (accelerometer tilt + WASD buttons).

---

## 📋 Table of Contents

- [Hardware Overview](#hardware-overview)
- [Pinout Reference](#pinout-reference)
- [Power System](#power-system)
- [Sensors](#sensors)
- [Motor Control](#motor-control)
- [Android App & MQTT Control](#android-app--mqtt-control)
- [Schematic Notes](#schematic-notes)
- [Known Issues & Planned Fixes](#known-issues--planned-fixes)
- [Build Log](#build-log)

---

## Hardware Overview

| Component | Part | Notes |
|-----------|------|-------|
| MCU | ESP32-DEVKITC | Dual-core, WiFi/BT capable |
| Motor Driver | L298N | Repurposed from L298N module — correct Schottky diodes included ✅ |
| Buck Converter | LM2596S-ADJ | 8.8V → 5V, set via RV1 trimpot |
| Power Input | 2S LiPo | 8.8V via JST J3 connector |
| Line Sensors | IR Sensor x3 | ⏳ To be added to schematic |
| Distance Sensors | HC-SR04 x3 | ⏳ To be added to schematic |
| Control App | Android (Android Studio) | MQTT-based, accelerometer + WASD control |

> ℹ️ **L298N Diodes:** Components are repurposed directly from an L298N module board. The module uses correct fast-recovery/Schottky freewheeling diodes by default — no diode substitution needed.

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
| IR Left | TBD | Digital/Analog Input | Use ADC1 pin — ADC2 disabled when WiFi active |
| IR Center | TBD | Digital/Analog Input | Use ADC1 pin — ADC2 disabled when WiFi active |
| IR Right | TBD | Digital/Analog Input | Use ADC1 pin — ADC2 disabled when WiFi active |

> ⚠️ **ADC2 Warning:** GPIOs 0, 2, 4, 12–15, 25–27 use ADC2 which is **disabled when WiFi is active**. Since MQTT requires WiFi, use **ADC1 pins (GPIO32–39)** for all analog IR readings.

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
- Freewheeling diodes on all L298N outputs (correct type confirmed — repurposed from module)

---

## Sensors

### IR Line Sensors (x3)
> ⏳ Not yet added to schematic

3 IR reflectance sensors positioned underneath the robot chassis for line detection. Output digital (HIGH/LOW) or analog depending on module.

**GPIO allocation notes:**
- WiFi must stay active for MQTT — use **ADC1 only** (GPIO32–39) for analog IR
- GPIO34, 35, 36, 39 are **input-only** — suitable for sensor inputs

**MQTT telemetry:** IR sensor states published to broker so the Android app can monitor line detection in real time.

### Ultrasonic Sensors HC-SR04 (x3)
> ⏳ Not yet added to schematic

3 ultrasonic sensors for obstacle detection (left, center, right). Requires 6 GPIOs total (3x TRIG + 3x ECHO).

**Required per sensor:**
- TRIG: any digital output GPIO
- ECHO: **must be level-shifted to 3.3V** (HC-SR04 outputs 5V)
  - Voltage divider: 1kΩ (top) + 2kΩ (bottom), tap midpoint to GPIO

**MQTT telemetry:** Distance readings from all 3 sensors streamed to the Android app for live obstacle monitoring.

---

## Motor Control

Dual H-bridge via **L298N**. Each motor channel independently controllable for direction (digital) and speed (PWM).

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

## Android App & MQTT Control

The robot is controlled via a **custom Android app** built in Android Studio. All communication runs over **MQTT**, with the ESP32 connecting to a broker hosted directly inside the APK over the local WiFi network.

---

### Architecture Overview

```
[Android App]
      │
      ├── Embedded MQTT Broker (runs inside APK)
      │         │
      │         │  port 1883 — local WiFi
      │         │
      └── MQTT Client (subscribes to telemetry, publishes commands)
                │
                │  WiFi / Local Network
                │
         [ESP32-DEVKITC]
                │
         Motors / Sensors
```

> The MQTT broker runs **embedded inside the Android APK** — no external server or cloud service required. The phone acts as both the broker host and the control client. The ESP32 connects to the broker using the phone's local IP address over shared WiFi.

---

### MQTT Broker (Embedded in APK)

A lightweight MQTT broker (e.g. **Moquette** or **HiveMQ Embedded**) is bundled and started automatically when the app launches. This gives a fully self-contained system — bring the phone, bring the robot, connect to the same WiFi and it works.

```java
// Example: starting Moquette embedded broker in Android
MoquetteServer broker = new MoquetteServer();
broker.startServer(); // Listens on port 1883
```

The ESP32 firmware points to the phone's local IP as its MQTT server address.

---

### MQTT Topic Structure

| Topic | Direction | Description |
|-------|-----------|-------------|
| `robot/control/move` | App → ESP32 | Movement command: `forward` / `back` / `left` / `right` / `stop` |
| `robot/control/speed` | App → ESP32 | PWM speed value (0–255) |
| `robot/control/mode` | App → ESP32 | `manual` or `line_follow` |
| `robot/telemetry/ir` | ESP32 → App | IR sensor states — JSON: `{left, center, right}` |
| `robot/telemetry/ultrasonic` | ESP32 → App | Distance readings — JSON: `{left, center, right}` in cm |
| `robot/telemetry/battery` | ESP32 → App | Battery voltage in V (float) |
| `robot/telemetry/speed` | ESP32 → App | Current PWM values for Motor A and B |

---

### Control Modes

The app supports **two manual control modes**, toggled by a single button in the UI. The button label updates to reflect the active mode. Switching modes registers/unregisters the accelerometer listener to preserve battery.

#### Mode 1 — Accelerometer (Tilt Control)
Phone tilt maps directly to robot movement commands:

| Tilt Direction | Robot Action |
|----------------|-------------|
| Forward | Move forward |
| Backward | Reverse |
| Left | Steer left |
| Right | Steer right |
| Flat/neutral | Stop |

- Sensitivity adjustable via in-app slider
- Accelerometer data read from Android `SensorManager`
- Commands published continuously while tilted past threshold

#### Mode 2 — WASD Button Control
On-screen directional pad:

```
        [ W ]
   [ A ][ S ][ D ]
        [STP]
```

- W = Forward, S = Reverse, A = Left, D = Right
- Hold to move, release to stop
- Speed adjustable via slider

---

### Telemetry Dashboard

The app includes a live monitoring screen that subscribes to all `robot/telemetry/#` topics and displays:

| Widget | Data Source | Notes |
|--------|------------|-------|
| IR Indicators | `robot/telemetry/ir` | 3 visual indicators — line detected / not detected |
| Ultrasonic Distances | `robot/telemetry/ultrasonic` | 3 live readouts in cm (Left / Center / Right) |
| Battery Voltage | `robot/telemetry/battery` | Live voltage + low battery warning |
| Motor Speed | `robot/telemetry/speed` | PWM values for both channels |
| Connection Status | Internal | MQTT broker + ESP32 connection indicator |

---

### ESP32 Firmware — MQTT Overview

```cpp
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid        = "YOUR_WIFI";
const char* password    = "YOUR_PASS";
const char* mqtt_server = "PHONE_LOCAL_IP"; // e.g. 192.168.1.x

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = String((char*)payload).substring(0, length);

  if (String(topic) == "robot/control/move") {
    handleMove(msg); // "forward" / "back" / "left" / "right" / "stop"
  }
  if (String(topic) == "robot/control/speed") {
    setSpeed(msg.toInt()); // 0–255
  }
  if (String(topic) == "robot/control/mode") {
    setMode(msg); // "manual" or "line_follow"
  }
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

### Rev 1 — April 2026
- R1/R2 on schematic are **battery voltage monitor** (ADC divider), NOT part of LM2596 feedback network
- LM2596 feedback set entirely by RV1 trimpot — functional but see planned fixes below
- ENA originally on GPIO12 (strapping pin) — **corrected to GPIO33**
- L298N components repurposed from module — correct freewheeling diodes confirmed ✅

---

## Known Issues & Planned Fixes

| Priority | Issue | Status |
|----------|-------|--------|
| 🔴 Critical | LM2596 FB trimpot-only risk: wiper failure could send 8.8V to ESP32 | ⏳ Planned |
| 🔴 Critical | Fix: R_upper 1kΩ (VOUT→FB) + R_lower 1kΩ (FB→GND), RV1 in series with R_lower | ⏳ Next revision |
| 🔴 Fixed | ENA on GPIO12 (strapping pin) — moved to GPIO33 | ✅ Fixed |
| 🟡 Warning | HC-SR04 ECHO pins need 3.3V level shifting | ⏳ To be added |
| 🟡 Warning | ADC2 unusable with WiFi — IR sensor GPIOs must be ADC1 | ⏳ To assign |
| 🟡 Warning | Low-ESR cap recommended on LM2596 output | 🔍 To verify |
| 🟡 Warning | 100nF ceramic decoupling caps on ESP32 VCC | ⏳ Planned |
| 🟡 Warning | 100nF ceramic in parallel with 220µF input cap | ⏳ Planned |
| 🟢 OK | L298N diodes — repurposed from module, correct type confirmed | ✅ |
| 🟢 OK | JST J3: Pin1=GND, Pin2=8.8V confirmed | ✅ |
| 🟢 OK | R1/R2 confirmed as battery ADC voltage divider | ✅ |

### LM2596 Planned Fix Detail

```
VOUT ── R_upper (1kΩ) ── FB ── RV1 (1kΩ trim) ── R_lower (1kΩ) ── GND
```

`Vout = 1.23 × (1 + R_lower_total / R_upper)`

With fixed resistors as base, a trimpot wiper failure cannot cause a catastrophic voltage spike on the 5V rail.

---

## Build Log

| Date | Entry |
|------|-------|
| Apr 2026 | Rev 1 schematic completed. ESP32 + LM2596 + L298N architecture established. |
| Apr 2026 | GPIO12 strapping conflict on ENA identified and corrected → GPIO33. |
| Apr 2026 | Confirmed R1/R2 are battery monitor divider, not LM2596 feedback. |
| Apr 2026 | L298N repurposed from existing module — correct freewheeling diodes confirmed, no substitution needed. |
| Apr 2026 | IR sensors (x3) and HC-SR04 ultrasonic sensors (x3) to be added to schematic. |
| Apr 2026 | Android MQTT app architecture defined: embedded broker in APK, dual control modes (accelerometer + WASD), full telemetry dashboard. |

---

## 📌 GPIO Quick Reference — Remaining Available

| GPIO | ADC | PWM | Notes |
|------|-----|-----|-------|
| GPIO32 | ADC1 ✅ | ✅ | Good for IR analog |
| GPIO34 | ADC1 ✅ | ❌ | Input only — ECHO or IR |
| GPIO35 | ADC1 ✅ | ❌ | Input only — ECHO or IR |
| GPIO36 | ADC1 ✅ | ❌ | Input only — ECHO or IR |
| GPIO39 | ADC1 ✅ | ❌ | Input only — IR analog |
| GPIO21 | — | ✅ | General purpose — TRIG |
| GPIO22 | — | ✅ | General purpose — TRIG |
| GPIO23 | — | ✅ | General purpose — TRIG |

> 💡 **Suggested allocation:** GPIO34/35/36 → 3x ECHO. GPIO32/39 + one more ADC1 → 3x IR. GPIO21/22/23 → 3x TRIG.
