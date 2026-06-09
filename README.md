# ESP32 MQTT Vision Robot

A line-following robot built around the **ESP32-DEVKITC V1**, featuring dual motor control via L298N, 3 IR line sensors, 3 ultrasonic distance sensors, and an LM2596S-ADJ buck converter powered by a custom 2S 18650 battery pack (8.4V fully charged). Fully controlled via a custom **Android MQTT app** with live telemetry, accelerometer tilt control, and WASD buttons.

All firmware is written in pure **ESP-IDF v5.x** (C). No Arduino framework. WiFi credentials are provisioned over BLE using NimBLE. Communication with the app uses MQTT over WiFi. The Android app runs an embedded MQTT broker — no cloud or external server needed.

---

## Table of Contents

- [Hardware Bill of Materials](#hardware-bill-of-materials)
- [Schematic](#schematic)
- [PCB Design Process](#pcb-design-process)
- [PCB Assembly & Testing](#pcb-assembly--testing)
- [Chassis — 3D Printed](#chassis--3d-printed)
- [Pinout Reference](#pinout-reference)
- [JST Connector Wiring](#jst-connector-wiring)
- [Power System](#power-system)
- [Motor Control](#motor-control)
- [Sensors](#sensors)
- [Firmware — Build & Flash](#firmware--build--flash)
- [Android App](#android-app)
- [First Run & WiFi Provisioning](#first-run--wifi-provisioning)
- [Control Modes](#control-modes)
- [Known Issues](#known-issues)
- [Build Log](#build-log)

---

## Hardware Bill of Materials

| Component | Part | Notes |
|-----------|------|-------|
| MCU | ESP32-DEVKITC V1 | Dual-core, WiFi + Bluetooth |
| Motor Driver | L298N | Desoldered from L298N breakout module — correct Schottky freewheeling diodes reused |
| Buck Converter | LM2596S-ADJ | 8.4V → 5V, output set via RV1 trimpot. Rectifier diode: SS34 Schottky |
| Battery | Custom 2S 18650 pack (2× 18650 cells) | 8.4V fully charged, connected via JST J3 (2-pin) |
| Charger | Hailege 2S USB-C BMS Boost Charger | Charges to 8.4V via USB-C (5V input), overcharge and overcurrent protection |
| IR Sensors | Generic IR reflectance module × 3 | Digital output, 3.3V logic — connected via JST-B (10-pin) |
| Ultrasonic Sensors | HC-SR04 × 3 — **2021+ version** | ECHO outputs 3.3V logic — no voltage divider needed. Connected via JST-A (10-pin) |
| OLED Display | SSD1306 128×32 I2C | Mounted on a separate daughter PCB above the main board |
| Android App | Custom (Kotlin, Android Studio, API 29+) | Embedded MQTT broker + WASD / accelerometer control |

> The L298N was desoldered from a breakout module to reuse its Schottky freewheeling diodes, which are the correct type and are already soldered in the correct polarity.

> The HC-SR04 **must** be the 2021+ version, whose ECHO pin outputs 3.3V logic. Earlier versions output 5V on ECHO and require a voltage divider before connecting to ESP32 GPIO. Check the PCB revision marking or measure with a multimeter before connecting.

---

## Schematic

### Full Schematic — Rev 2

![Full Schematic](schematic_full.png)

*ESP32-DEVKITC V1 + LM2596S-ADJ power section + L298N motor driver + JST-A ultrasonic connector + JST-B IR / OLED connector*

### LM2596S-ADJ — Buck Converter & Battery Monitor

![LM2596 Section](schematic_lm2596.png)

*8.4V input via J3 (2-pin JST). R1/R2 voltage divider (100 kΩ / 100 kΩ) feeds battery voltage to D34 for ADC monitoring. LM2596S-ADJ with SS34 Schottky diode D1, inductor L1, 220 µF input and output electrolytic capacitors, RV1 trimpot setting the 5V output via the FB pin.*

### L298N — Motor Driver

![L298N Section](schematic_l298n.png)

*L298N dual H-bridge with 8 Schottky freewheeling diodes. Motor A: IN1 (D13), IN2 (D14), EnA (D21). Motor B: IN3 (D27), IN4 (D26), EnB (D25). Motor outputs routed to J1 and J2 screw terminals.*

---

## PCB Design Process

The PCB is designed in **KiCad**. Custom component footprints are in the `/footprints` folder of this repo.

### Step 1 — Schematic Capture

Built in KiCad's schematic editor. The following components were not in KiCad's default library and were imported manually:

- ESP32-DEVKITC V1
- LM2596S-ADJ
- L298N
- JST-A and JST-B connectors

### Step 2 — Component Placement

Components were placed manually, following these principles:

- **LM2596 power section** — tight left-to-right cluster: input capacitor → IC → inductor → Schottky diode → output capacitor. Keeps the switching current loop short to minimise EMI.
- **L298N** — centered on the board, freewheeling diodes grouped tightly around each output pair.
- **ESP32-DEVKITC V1** — centered, GPIO pins facing the connector side to reduce trace crossings.
- **JST-A and JST-B** — right edge of the board for clean external cable routing.
- **J1/J2 screw terminals** — bottom edge for easy motor wire access.
- **J3 power connector** — left edge, close to the LM2596 input.

### Step 3 — Design Rules

Configured via **File → Board Setup → Design Rules → Net Classes**:

| Netclass | Track Width | Clearance | Applied To |
|----------|------------|-----------|------------|
| Default | 0.25 mm | 0.2 mm | All signal traces (GPIO, sensors, PWM) |
| Power | 1.0 mm | 0.8 mm | GND, 8.4V battery rail, 5V rail, motor output nets |

Power netclass assigned to: `GND`, `Net-(J3-Pin_2)` (8.4V battery), `Net-(JST-A1-Pin_1)` (5V rail), `Net-(J1-Pin_1/2)` (Motor A outputs), `Net-(J2-Pin_1/2)` (Motor B outputs).

### Step 4 — Board Outline

Rectangle drawn on the `Edge.Cuts` layer with ~5 mm clearance around all components using **Place → Rectangle**.

### Step 5 — Mounting Holes

4× `MountingHole_4.3x6.2mm_M4_Pad` placed at each corner for chassis standoff mounting. References set to H1–H4 (required for FreeRouter SES import compatibility).

### Step 6 — DRC (pre-routing)

Run via **Inspect → Design Rules Checker → Run DRC**:
- 3 silkscreen overlap warnings — fixed
- 0 footprint errors
- 69 unconnected pads (expected before routing)

### Step 7 — Auto-Routing with FreeRouter

1. **File → Export → Specctra DSN**
2. Open in FreeRouter, run the auto-router
3. **File → Import → Specctra Session (.ses)**
4. Review all traces manually and re-run DRC

Post-import DRC found 28 violations: decorative text placed on F.Cu (causing trace conflicts), IC pad spacing vs. power clearance rule violations, and 3 dangling track stubs. All fixed:
- Decorative text moved to F.Silkscreen layer
- Power clearance reduced from 0.8 mm to 0.5 mm to match IC footprint pad spacing
- 3 dangling stubs deleted

Final DRC: **0 errors, 2 warnings** (mounting hole library mismatch — ignorable).

![Routed PCB](pcb_routed.png)

*Fully routed 2-layer PCB. Red = F.Cu, blue = B.Cu. Thick traces = Power netclass (0.8 mm), thin = Default (0.25 mm).*

### Step 8 — Gerber Export & Manufacturing

> Board is production-ready. Final DRC: 0 errors.

1. **File → Plot** → Gerber format
2. Export all layers: F.Cu, B.Cu, F.Silkscreen, B.Silkscreen, F.Mask, B.Mask, Edge.Cuts + drill files
3. Submit to manufacturer — ordered as 2-layer, HASL finish, 5 copies

Gerber files are in the `/gerber` folder of this repo.

---

### OLED Display Board

A second small PCB was designed to mount a **SSD1306 128×32 I2C OLED** on top of the robot chassis. It sits above the main PCB, hides the main board from view, and connects back via the spare pins on JST-B.

The board contains no active components — just 4 solder test pads (H1–H4) for wire routing, a 4-pin JST connector (J5), and the OLED module footprint. Gerber files are in the `/gerber` folder alongside the main board gerbers.

**Why a separate board:**
- The main PCB had no remaining space for an OLED footprint
- Mounting on top of the chassis hides the main board for a cleaner look
- Can be detached and replaced independently from the main board

#### OLED Board Schematic

![OLED Schematic](oled_schematic.PNG)

*J5 (4-pin JST) connects to H1 (GND), H2 (VCC), H3 (SCL), H4 (SDA). Short wires run from these pads to the OLED module pins.*

#### OLED Board PCB Layout

![OLED PCB Layout](oled_pcb_layout.png)

*Minimal PCB — 4 test pad holes + JST connector. Sits on top of chassis above the main board.*

#### OLED Board 3D View

![OLED 3D View](pcb_layout2.PNG)

#### OLED — JST-B Connection

The OLED board connects using the spare pins on JST-B (the IR sensor connector):

| JST-B Pin | Signal | OLED Board Pad | ESP32 GPIO |
|-----------|--------|---------------|-----------|
| 6 | — spare — | — | — |
| 7 | SDA | H4 | D4 (Pin 5) |
| 8 | SCL | H3 | D5 (Pin 8) |
| 9 | VCC (5V) | H2 | — |
| 10 | GND | H1 | — |

> Verify your SSD1306 module accepts 5V on VCC. Most breakout modules have an onboard 3.3V LDO and accept 5V. Bare OLED modules without a regulator require 3.3V only.

> The ESP32's default I2C pins (D21/D22) are both used by the motor driver. I2C is remapped to D4 (SDA) and D5 (SCL) inside the ESP-IDF I2C driver configuration in `oled.c`.

#### OLED — What It Displays

| Line | Content |
|------|---------|
| 1 | Battery voltage |
| 2 | MQTT connection status |
| 3 | Current mode (MANUAL / LINE FOLLOW) |
| 4 | Robot state (FOLLOWING / AVOIDING / RECOVERING) |

---

## PCB Assembly & Testing

**Do not solder all components at once.** Bring up the power circuit first and verify the output voltage before soldering the ESP32 or any sensors. This protects the ESP32 from overvoltage if the trimpot is set incorrectly from the factory.

### Step 1 — Inspect the Received PCBs

- Check for visible manufacturing defects: solder bridges, missing soldermask, lifted pads
- Hold the PCB up to light to check copper layer alignment
- Verify silkscreen labels match the schematic reference designators

### Step 2 — Solder the Buck Converter Circuit Only

Solder only the following components. Leave everything else unpopulated.

| Ref | Component |
|-----|-----------|
| J3 | 2-pin JST power input connector |
| U_LM2596 | LM2596S-ADJ IC |
| D1 | SS34 Schottky diode — stripe (band) = cathode |
| L1 | Inductor |
| C_in | 220 µF electrolytic input capacitor — stripe = GND (negative) |
| C_out | 220 µF electrolytic output capacitor — stripe = GND (negative) |
| RV1 | Trimpot |
| R1, R2 | 100 kΩ resistors (battery ADC voltage divider) |

Do **not** solder: ESP32, L298N, JST-A, JST-B, J1/J2 screw terminals, or any sensors yet.

### Step 3 — Set the Buck Converter Output to 5V

This is the most critical step. The LM2596S-ADJ output voltage is set by RV1. If left at the wrong position it may output the full battery voltage (up to 8.4V) to the 5V rail, which will destroy the ESP32.

1. Connect the 2S 18650 battery pack to J3 (Pin 1 = GND, Pin 2 = battery positive)
2. Set your multimeter to DC voltage
3. Place probes across the output capacitor C_out (or the 5V rail test points)
4. Slowly adjust RV1 until the meter reads **5.0V**
5. Confirm the reading is stable with no oscillation
6. Disconnect the battery

### Step 4 — Verify the Battery ADC Divider

While the battery was connected in Step 3, or reconnect briefly now:

- Measure the voltage at the midpoint of R1 and R2 (the trace going to D34)
- With a fully charged 8.4V battery, you should read approximately **4.2V** (1:1 divider, 100 kΩ / 100 kΩ)
- This confirms the voltage divider is correctly wired before the ESP32 is installed

> The firmware ADC saturates when the divider midpoint exceeds ~3.1V, which corresponds to a battery voltage of ~6.2V. This is a hardware limitation — the battery reading shows ~6.2V on a healthy 2S pack and only starts falling below 6.2V when the pack is nearly depleted. The app thresholds are set at 6.0V / 5.5V to account for this.

### Step 5 — Solder Remaining Components

Now that the 5V rail is confirmed correct and safe:

| Ref | Component |
|-----|-----------|
| U_L298N | L298N motor driver IC + all 8 Schottky freewheeling diodes |
| U_ESP32 | ESP32-DEVKITC V1 |
| JST-A | 10-pin connector (ultrasonic sensors) |
| JST-B | 10-pin connector (IR sensors + OLED) |
| J1, J2 | Screw terminals (motor A and motor B outputs) |

Solder the OLED daughter board separately — it is a standalone PCB.

### Step 6 — Power-On Verification

1. Connect the battery to J3
2. The ESP32 should boot — the onboard LED comes on
3. Connect the ESP32 to a PC via USB
4. Open a serial monitor at **115200 baud**
5. You should see BLE provisioning log output: `I (xxx) PROV: advertising as "IoT-Robot"`

If nothing appears on the serial monitor:
- Check battery polarity on J3
- Re-measure the 5V rail — must be 5V, not higher
- Verify ESP32 is seated correctly and all pins are soldered

### Step 7 — Connect and Test Sensors

Connect sensors one at a time and watch the serial output:

1. **IR sensors (JST-B)** — move a reflective object under each sensor; the digital output should toggle between 0 and 1
2. **Ultrasonic sensors (JST-A)** — wave your hand in front of each sensor; distances should appear in the telemetry log
3. **OLED board** — connect via JST-B spare pins 7–10; the display should show battery voltage and MQTT status after provisioning
4. **Motors (J1, J2)** — connect motors and verify direction in manual mode via the Android app

---

## Chassis — 3D Printed

The robot chassis is designed in **Autodesk Inventor** and 3D printed in PLA. The STEP file is in the root of this repo for reference and modification.

![Chassis Assembly](assembly1.PNG)

*Autodesk Inventor assembly — PCB on standoffs, motor mounts, battery compartment, and sensor positions.*

**Design details:**
- IR sensors mount on the underside, facing down toward the ground for line detection
- HC-SR04 sensors mount on the front face in left, center, and right positions
- Motor mounts are integrated into the chassis base
- PCB sits on M4 standoffs matching the 4 corner mounting holes on the PCB
- Battery compartment is accessible from the bottom of the chassis

**Print settings used:** PLA, 0.2 mm layer height, 20% infill for non-structural sections, 50% infill for motor mounts and standoff pillars.

---

## Pinout Reference

### Motor Driver — L298N

| L298N Signal | ESP32 Label | ESP32 Pin | Type |
|-------------|-------------|-----------|------|
| IN1 | D13 | Pin 28 | Digital Output — Motor A direction |
| IN2 | D14 | Pin 26 | Digital Output — Motor A direction |
| EnA | D21 | Pin 11 | PWM Output — Motor A speed |
| IN3 | D27 | Pin 25 | Digital Output — Motor B direction |
| IN4 | D26 | Pin 24 | Digital Output — Motor B direction |
| EnB | D25 | Pin 23 | PWM Output — Motor B speed |

### Battery Monitor

| Function | ESP32 Label | ESP32 Pin | Notes |
|----------|-------------|-----------|-------|
| Battery ADC | D34 | Pin 19 | R1/R2 divider from J3 Pin 2. Input-only GPIO, ADC1 — WiFi safe |

### IR Line Sensors — JST-B (10-pin)

| Sensor | ESP32 Label | ESP32 Pin | Notes |
|--------|-------------|-----------|-------|
| IR Left | D32 | Pin 21 | ADC1 — WiFi/MQTT safe |
| IR Center | D33 | Pin 22 | ADC1 — WiFi/MQTT safe |
| IR Right | D15 | Pin 3 | Mild strapping pin — IR signal defaults LOW at boot, no conflict |

### Ultrasonic Sensors HC-SR04 (2021+) — JST-A (10-pin)

| Sensor | TRIG Label | TRIG Pin | ECHO Label | ECHO Pin | Notes |
|--------|-----------|----------|-----------|----------|-------|
| Left | D22 | Pin 14 | D35 | Pin 20 | ECHO input-only GPIO, ADC1 |
| Center | D23 | Pin 15 | D19 | Pin 10 | |
| Right | D18 | Pin 9 | D2 | Pin 4 | GPIO2 = onboard LED — LED flashes with echoes (cosmetic, sensor reads correctly) |

> HC-SR04 2021+: ECHO outputs 3.3V logic — direct connection to ESP32, no voltage divider needed.

### JST Power Connector — J3 (2-pin)

| Pin | Signal |
|-----|--------|
| 1 | GND |
| 2 | Battery positive (8.4V fully charged) |

### Complete GPIO Map

| Label | ESP32 Pin | Function | Type |
|-------|-----------|----------|------|
| D2 | Pin 4 | ECHO Right (ultrasonic) | Input |
| D4 | Pin 5 | OLED SDA | I2C |
| D5 | Pin 8 | OLED SCL | I2C |
| D13 | Pin 28 | Motor A IN1 | Output |
| D14 | Pin 26 | Motor A IN2 | Output |
| D15 | Pin 3 | IR Right | Input |
| D18 | Pin 9 | TRIG Right (ultrasonic) | Output |
| D19 | Pin 10 | ECHO Center (ultrasonic) | Input |
| D21 | Pin 11 | EnA — Motor A speed | PWM Output |
| D22 | Pin 14 | TRIG Left (ultrasonic) | Output |
| D23 | Pin 15 | TRIG Center (ultrasonic) | Output |
| D25 | Pin 23 | EnB — Motor B speed | PWM Output |
| D26 | Pin 24 | Motor B IN4 | Output |
| D27 | Pin 25 | Motor B IN3 | Output |
| D32 | Pin 21 | IR Left | Input — ADC1 |
| D33 | Pin 22 | IR Center | Input — ADC1 |
| D34 | Pin 19 | Battery ADC | Input — ADC1, input-only GPIO |
| D35 | Pin 20 | ECHO Left (ultrasonic) | Input — ADC1, input-only GPIO |

---

## JST Connector Wiring

### JST-A — Ultrasonic Sensors (10-pin)

| Pin | Signal | ESP32 Label | ESP32 Pin |
|-----|--------|-------------|-----------|
| 1 | 5V (shared VCC) | — | — |
| 2 | GND (shared) | — | GND |
| 3 | TRIG Left | D22 | Pin 14 |
| 4 | ECHO Left | D35 | Pin 20 |
| 5 | TRIG Center | D23 | Pin 15 |
| 6 | ECHO Center | D19 | Pin 10 |
| 7 | TRIG Right | D18 | Pin 9 |
| 8 | ECHO Right | D2 | Pin 4 |
| 9–10 | — spare — | — | — |

### JST-B — IR Line Sensors + OLED (10-pin)

| Pin | Signal | ESP32 Label | ESP32 Pin |
|-----|--------|-------------|-----------|
| 1 | 3.3V (shared VCC) | — | — |
| 2 | GND (shared) | — | GND |
| 3 | IR Left | D32 | Pin 21 |
| 4 | IR Center | D33 | Pin 22 |
| 5 | IR Right | D15 | Pin 3 |
| 6 | — spare — | — | — |
| 7 | OLED SDA | D4 | Pin 5 |
| 8 | OLED SCL | D5 | Pin 8 |
| 9 | OLED VCC (5V) | — | — |
| 10 | OLED GND | — | GND |

VCC and GND are daisy-chained across all sensors within each connector group.

---

## Power System

```
[2S 18650 Battery Pack — 8.4V fully charged]
          │
       J3 (2-pin JST)  —  Pin 1: GND,  Pin 2: 8.4V
          │
          ├── R1 / R2 voltage divider (100 kΩ / 100 kΩ) ──── D34 Pin 19 (battery ADC)
          ├── 220 µF bulk input capacitor
          ├── L298N VS pin  (motors run from raw battery voltage)
          │
       LM2596S-ADJ  (D1: SS34 Schottky, RV1 trimpot → FB → 5V out)
       Output: 220 µF capacitor
          │
         5V rail ─────────── HC-SR04 VCC (JST-A Pin 1)
          │                  OLED VCC (JST-B Pin 9)
          │
       ESP32-DEV VIN ──── onboard LDO ──── 3.3V rail ──── IR sensor VCC (JST-B Pin 1)
```

The motors are powered directly from the battery voltage through the L298N VS pin — they operate at the full battery voltage, not the regulated 5V. The 5V rail powers only the ultrasonic sensors and OLED. The ESP32's onboard LDO produces the 3.3V rail for IR sensors and internal logic.

---

## Motor Control

The robot uses differential drive with the L298N dual H-bridge. Speed is controlled by PWM on the Enable pins (EnA/EnB). Direction is set by the IN1–IN4 logic inputs.

### H-Bridge Truth Table (per channel)

| IN_A | IN_B | Result |
|------|------|--------|
| HIGH | LOW | Forward |
| LOW | HIGH | Reverse |
| LOW | LOW | Coast (free spin) |
| HIGH | HIGH | Brake |

### Differential Steering

| Action | Motor A (Left) | Motor B (Right) |
|--------|---------------|----------------|
| Forward | Full speed | Full speed |
| Turn Left | Reduced speed | Full speed |
| Turn Right | Full speed | Reduced speed |
| Hard Left | Stop | Full speed |
| Hard Right | Full speed | Stop |
| Reverse | Full back | Full back |
| Spin Left | Full back | Full forward |
| Spin Right | Full forward | Full back |

> **Hardware note:** Motor A (left) has IN1 and IN2 wired backwards on the PCB. This is corrected in firmware inside the `drive()` function by inverting the polarity sent to PIN_IN1/PIN_IN2 for Motor A only. All higher-level motor commands (forward, reverse, turn, spin) are unaffected.

---

## Sensors

### IR Line Sensors (×3)

3 IR reflectance sensors mounted under the chassis for line detection. Digital output, 3.3V modules, connected via JST-B.

- All three GPIOs are on ADC1 — WiFi and MQTT remain active while reading
- No level shifting required (sensors output 3.3V logic)
- Firmware applies a 2-consecutive-read debounce before accepting a new state

**MQTT telemetry topic:** `robot/telemetry/ir`
```json
{"left": 0, "center": 1, "right": 0}
```

`1` = line detected (dark surface), `0` = no line (bright surface).

### Ultrasonic Sensors HC-SR04 2021+ (×3)

3 ultrasonic distance sensors for obstacle detection (left, center, right), connected via JST-A. Powered from the 5V rail.

ECHO connects directly to ESP32 GPIO — no voltage divider required (2021+ version outputs 3.3V logic).

Distance measurement uses an **interrupt-driven ISR** (`GPIO_INTR_ANYEDGE`). The rising edge records a timestamp in µs; the falling edge computes the distance as `pulse_us / 58`. This replaces the earlier polling approach, which missed short echo pulses at distances under ~170 cm.

**MQTT telemetry topic:** `robot/telemetry/ultrasonic`
```json
{"left": 24, "center": 8, "right": 31}
```

Values are in centimetres.

---

## Firmware — Build & Flash

The firmware is written in **C using ESP-IDF v5.4.1**. No Arduino framework, no PlatformIO.

### Project Structure

```
firmware/
├── CMakeLists.txt          — project root (IDF project)
├── sdkconfig.defaults      — NimBLE enabled, 8 KB main stack
└── main/
    ├── CMakeLists.txt      — lists all source files and dependencies
    ├── main.c              — app_main + 10 ms main loop
    ├── config.h            — all pin definitions and constants
    ├── provisioning.c/h    — BLE credential provisioning (NimBLE + NVS)
    ├── sensors.c/h         — IR, ultrasonic (ISR-driven), battery ADC
    ├── state_machine.c/h   — autonomy logic + motor control
    ├── mqtt.c/h            — WiFi STA + esp-mqtt client (event-driven)
    └── oled.c/h            — SSD1306 raw I2C driver + 5×7 font (no external lib)
```

### Firmware Architecture

```
app_main
  ├── provisioning_init()   — blocks on first boot (BLE); loads NVS on subsequent boots
  ├── sensors_init()        — configures GPIO, ISR handlers, ADC
  ├── state_machine_init()  — initialises LEDC channels and motor GPIOs
  ├── mqtt_init()           — blocks until WiFi + MQTT connected
  └── oled_init()           — I2C probe, skip gracefully if no OLED

Main loop (10 ms period)
  ├── sensors_read_ir()
  ├── sensors_read_ultrasonic()
  ├── sensors_read_battery()
  ├── state_machine_update()
  ├── every 100 ms → mqtt_publish_telemetry()
  └── every 500 ms → oled_update()
```

### Building on Windows (Docker + WSL2)

Build runs inside a Docker Dev Container. Flashing runs via WSL2 Ubuntu.

**One-time setup:**

1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop)
2. Install WSL2 Ubuntu — PowerShell as Administrator:
   ```
   wsl --install -d Ubuntu
   ```
3. Install usbipd — PowerShell as Administrator:
   ```
   winget install usbipd
   ```
4. Inside Ubuntu terminal, install esptool:
   ```bash
   sudo apt update && sudo apt install pipx -y
   pipx install esptool
   pipx ensurepath
   source ~/.bashrc
   ```
5. Add yourself to the dialout group (then close and reopen the Ubuntu terminal):
   ```bash
   sudo usermod -aG dialout $USER
   ```

**Build (each session):**

1. Make sure Docker Desktop is running
2. Open the `mqttvision` folder in VS Code
3. `Ctrl+Shift+P` → **Dev Containers: Reopen in Container**
4. In the integrated terminal:
   ```bash
   cd /project/firmware
   idf.py build
   ```

**Flash (each session):**

1. Attach the ESP32 to WSL2 — PowerShell as Administrator:
   ```
   usbipd list
   usbipd bind --busid <BUSID>
   usbipd attach --wsl --busid <BUSID>
   ```
   The BUSID is listed next to "Silicon Labs CP210x" or "CH340". It may change if you use a different USB port.

2. Fix USB permissions if needed — Ubuntu terminal:
   ```bash
   sudo chmod 666 /dev/ttyUSB0
   ```

3. Flash — Ubuntu terminal:
   ```bash
   cd /mnt/c/Users/<you>/Desktop/mqttvision/firmware/build
   esptool.py --chip esp32 -p /dev/ttyUSB0 -b 460800 --before default_reset --after hard_reset write_flash @flash_args
   ```

4. Open the serial monitor:
   ```bash
   screen /dev/ttyUSB0 115200
   ```
   Exit with `Ctrl+A` then `K`.

**Erase flash** (when NVS needs clearing — e.g. stale WiFi credentials):
```bash
esptool.py --chip esp32 -p /dev/ttyUSB0 erase_flash
```
Then re-flash using Step 3 above.

### Building on Linux (Arch / Ubuntu native)

**One-time setup (Arch):**
```bash
sudo pacman -S git cmake ninja python python-pip python-pipx
sudo usermod -aG uucp $USER        # log out and back in after this
git clone --recursive https://github.com/espressif/esp-idf.git ~/esp/idf
cd ~/esp/idf && git checkout v5.4.1 && ./install.sh esp32
echo 'source ~/esp/idf/export.sh' >> ~/.bashrc && source ~/.bashrc
idf.py --version                   # should show ESP-IDF v5.4.1
```

**Build and flash:**
```bash
cd path/to/mqttvision/firmware
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

If permission is denied on `/dev/ttyUSB0`: `sudo chmod 666 /dev/ttyUSB0` (permanent: `sudo usermod -aG uucp $USER` then log out/in).

### Key Constants (config.h)

```c
/* Motor pins — L298N */
#define PIN_IN1   13    /* Motor A direction (wired backwards on PCB — corrected in drive()) */
#define PIN_IN2   14
#define PIN_ENA   21    /* Motor A speed (PWM) */
#define PIN_IN3   27    /* Motor B direction */
#define PIN_IN4   26
#define PIN_ENB   25    /* Motor B speed (PWM) */

/* IR sensors */
#define PIN_IR_LEFT    32
#define PIN_IR_CENTER  33
#define PIN_IR_RIGHT   15
#define IR_LINE_VALUE   0    /* GPIO level when sensor is on a dark line */
#define IR_DEBOUNCE_READS  2

/* Ultrasonic sensors */
#define PIN_TRIG_LEFT    22
#define PIN_ECHO_LEFT    35
#define PIN_TRIG_CENTER  23
#define PIN_ECHO_CENTER  19
#define PIN_TRIG_RIGHT   18
#define PIN_ECHO_RIGHT    2  /* GPIO2 = onboard LED — flashes with echoes (cosmetic only) */
#define ULTRA_TIMEOUT_US  25000  /* ~4 m max range, abort echo wait after this */

/* OLED I2C */
#define PIN_OLED_SDA   4
#define PIN_OLED_SCL   5
#define OLED_ADDRESS   0x3C

/* Battery ADC — GPIO34, ADC1_CH6 */
#define PIN_BATTERY      34
#define BATTERY_R1       100000.0f
#define BATTERY_R2       100000.0f

/* Speed — 8-bit PWM duty (0–255) */
#define SPEED_BASE        150   /* default speed, overridden by app slider */
#define SPEED_AVOID        90   /* cautious obstacle avoidance speed */
#define SPEED_CORRECTION   75   /* gentle correction turn speed */
#define SPEED_POST_CORR    80   /* forward speed when re-acquiring line */
#define SPEED_RAMP_STEP     2   /* PWM duty added per 10 ms tick while on-line */

/* Obstacle thresholds (cm) */
#define OBSTACLE_WARN_CM  20
#define OBSTACLE_STOP_CM  12

/* State machine timing */
#define RECOVERY_SWEEP_MS    2000
#define RECOVERY_SPIN_MS     5000
#define AVOID_MIN_MS          500
#define POST_AVOID_FWD_MS     600
#define RECOVERY_CREEP_MS     150

/* Loop intervals */
#define TELEMETRY_INTERVAL_MS  100
#define OLED_UPDATE_MS         500
#define MAIN_LOOP_MS            10

/* BLE provisioning window on every boot */
#define BLE_PROV_WINDOW_MS  30000
```

### Autonomous Mode — State Machine

The firmware runs a 3-state machine when in autonomous (line-follow) mode:

```
         ┌──────────────┐
    ┌───▶│  FOLLOWING   │◀────────────┐
    │    └──────┬───────┘             │
    │           │ obstacle < 20 cm    │ line found
    │           ▼                     │
    │    ┌──────────────┐             │
    │    │   AVOIDING   │             │
    │    └──────┬───────┘             │
    │           │ line lost (0,0,0)   │ line found
    │           ▼                     │
    │    ┌──────────────┐             │
    └────│  RECOVERING  │─────────────┘
         └──────────────┘
               │ no line after 5 s
               ▼
            STOP — await manual override via MQTT
```

**FOLLOWING** — normal line tracking using the IR truth table below. In the table, `1` = line detected (sensor on dark surface), `0` = not on line.

| IR Left | IR Center | IR Right | Action |
|---------|-----------|---------|--------|
| 0 | 1 | 0 | Forward — centered on line |
| 0 | 1 | 1 | Curve right gently |
| 1 | 1 | 0 | Curve left gently |
| 0 | 0 | 1 | Turn right hard |
| 1 | 0 | 0 | Turn left hard |
| 1 | 1 | 1 | All sensors on line — forward |
| 1 | 0 | 1 | Junction or crossing — continue forward |
| 0 | 0 | 0 | Line lost — enter RECOVERING |

**AVOIDING** — triggered when any ultrasonic sensor reads < 20 cm. Robot curves around the obstacle using the left/right distance comparison to choose which side to go, keeping the outer IR sensor tracking the line edge for as long as possible.

**RECOVERING** — all IR sensors show 0 (line lost):
1. Remember which side sensor was last active
2. Slow down, turn that direction for up to 2 s
3. If line is found → return to FOLLOWING
4. If not found after 2 s → slow 360° spin scan
5. If still not found → stop and wait for manual override via MQTT

In **manual mode**, the state machine is bypassed entirely. MQTT move commands drive the motors directly.

---

## Android App

The Android app is built in **Kotlin** with **Jetpack Compose** (no XML layouts), targeting **API 29 (Android 10+)**. It embeds a full MQTT broker — no external server or cloud service needed.

### Architecture

```
[Android Phone]
      │
      ├── Moquette MQTT Broker (foreground service, port 1883)
      │         │
      │         └── Eclipse Paho MQTT Client (subscribes + publishes)
      │
      │   Local WiFi Network
      │
[ESP32-DEVKITC V1]
      │
      └── esp-mqtt client (connects to phone IP:1883)
            ├── Subscribes: robot/control/move, mode, speed
            └── Publishes:  robot/telemetry/#
```

The phone's local WiFi IP address acts as the broker address. The ESP32 learns this IP during BLE provisioning and connects on port 1883.

**Connection sequence:**
1. App launches → Moquette broker starts as a foreground service on port 1883
2. `MainActivity` waits for `MqttBrokerService.isRunning`, then Paho connects to `localhost:1883`
3. ESP32 boots → 30-second BLE window expires → WiFi connects → esp-mqtt connects to phone IP:1883
4. Handshake complete — telemetry flows in, controls work

### File Structure

```
MqttVisionRobot/app/src/main/java/com/ariknel/mqttvisionrobot/
├── MainActivity.kt           — entry point, broker lifecycle, screen routing
├── MqttBrokerService.kt      — Moquette broker as Android foreground service
├── MqttClientManager.kt      — Eclipse Paho client, publish helpers
├── BleProvisioningManager.kt — BLE scan, connect, write credentials, receive reply
├── RobotState.kt             — global Compose state (telemetry, connection, mode)
├── TelemetryParser.kt        — parses robot/telemetry/# JSON into RobotState
├── RobotControlScreen.kt     — main control UI (StatusBar, D-pad, speed, tilt, telemetry)
└── ProvisioningScreen.kt     — BLE provisioning form (auto-detects broker IP)
```

### MQTT Topics

| Topic | Direction | Payload |
|-------|-----------|---------|
| `robot/control/move` | App → ESP32 | `forward` / `back` / `left` / `right` / `stop` |
| `robot/control/speed` | App → ESP32 | PWM value `0`–`255` |
| `robot/control/mode` | App → ESP32 | `manual` or `line_follow` |
| `robot/telemetry/ir` | ESP32 → App | `{"left":0,"center":1,"right":0}` |
| `robot/telemetry/ultrasonic` | ESP32 → App | `{"left":24,"center":8,"right":31}` |
| `robot/telemetry/battery` | ESP32 → App | `8.21` (float string, volts) |
| `robot/telemetry/speed` | ESP32 → App | `{"a":180,"b":180}` |

All messages use **QoS 0** (fire-and-forget). At a 100 ms telemetry rate, a missed packet is irrelevant.

### Telemetry Dashboard

| Widget | Source | Notes |
|--------|--------|-------|
| IR indicators | `robot/telemetry/ir` | 3 dot indicators — filled = line detected |
| Ultrasonic distances | `robot/telemetry/ultrasonic` | Live cm readouts Left / Center / Right |
| Battery voltage | `robot/telemetry/battery` | Colour-coded: green / amber / red |
| BROKER status | Internal | Live dot — Moquette running on phone |
| MQTT status | Internal | Live dot — ESP32 connected to broker |

### App UI

![App UI](app_front.png)

### Dependencies

```kotlin
// MQTT Broker — Moquette embedded
implementation("io.moquette:moquette-broker:0.17")

// MQTT Client — Eclipse Paho
implementation("org.eclipse.paho:org.eclipse.paho.client.mqttv3:1.2.5")
implementation("org.eclipse.paho:org.eclipse.paho.android.service:1.1.1")

// Coroutines — async broker startup
implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3")
```

---

## First Run & WiFi Provisioning

WiFi credentials and the MQTT broker IP are not hardcoded in firmware. On first boot, the ESP32 advertises over BLE and waits for provisioning.

**BLE service details:**

```
Device name:    IoT-Robot
Service UUID:   4fafc201-1fb5-459e-8fcc-c5c9c331914b
Characteristic: beb5483e-36e1-4688-b7f5-ea07361b26a8  (WRITE + NOTIFY)

Write payload:  {"ssid":"YourWiFi","password":"YourPass","broker":"192.168.x.x"}
Reply (notify): "OK"  — credentials saved, ESP32 restarts into WiFi mode
                "ERR" — malformed JSON, try again
```

**Boot behaviour:**

| Condition | What happens |
|-----------|-------------|
| No credentials in NVS (first boot) | BLE advertises indefinitely until credentials are received |
| Credentials present, normal boot | BLE advertises for 30 s (re-provision window), then stops and connects to WiFi |
| Boot immediately after provisioning | BLE skipped entirely (`fresh` NVS flag set), goes straight to WiFi |

**Provisioning steps:**

1. Power on the robot — the ESP32 advertises as `IoT-Robot`
2. Open the Android app and tap **BLE PROVISIONING**
3. The app auto-detects the broker IP from the phone's WiFi interface — only the WiFi SSID and password need to be typed in manually
4. Tap **PROVISION** — the app writes the JSON credentials over BLE
5. The ESP32 replies with "OK" and restarts
6. After restarting, the ESP32 connects to WiFi then to the MQTT broker running on the phone
7. The app status bar shows **BROKER OK** and **MQTT** connected

**Re-provisioning:** Power on the robot and connect via BLE within 30 seconds of boot.

**Full reset:** Erase NVS to force a first-boot state:
```bash
esptool.py --chip esp32 -p /dev/ttyUSB0 erase_flash
```
Then reflash the firmware.

---

## Control Modes

The app has two independent mode axes that can be combined freely.

### Drive Source — WASD or Tilt

Toggled by the **TILT/WASD** button. The accelerometer listener is registered and unregistered on each toggle to preserve battery.

**WASD mode (default):**

```
      [ W ]
 [ A ][ S ][ D ]
      [STP]
```

Hold a button to move; releasing it sends `stop`. The speed slider sets the PWM value (0–255).

**Tilt mode (accelerometer):**

| Tilt direction | Action |
|---------------|--------|
| Forward | Move forward |
| Backward | Reverse |
| Left | Steer left |
| Right | Steer right |
| Flat | Stop |

Tilt speed is dynamic — a linear ramp from 80 to 255 as tilt angle increases. The speed slider updates in real time.

| Axis | Trigger threshold |
|------|------------------|
| Forward | `sensitivity − 0.6` (triggers slightly earlier for natural feel) |
| Left / Right | `sensitivity` |
| Reverse | `sensitivity + reverseGuard` |

A collapsible sensitivity panel (visible in tilt mode only) exposes two sliders:

| Slider | Effect |
|--------|--------|
| SENSITIVITY | Controls the trigger threshold for all axes |
| REVERSE GUARD | Extra tilt required to trigger reverse. Set to maximum (10) to disable reverse entirely |

### Autonomy — AUTO/MAN

Toggled by the **AUTO/MAN** button in the status bar.

- **MAN** — manual control via WASD or tilt. App publishes `manual` to `robot/control/mode`.
- **AUTO** — autonomous line-follow mode. App publishes `line_follow` to `robot/control/mode`. The control pad shows an "AUTO MODE" overlay and ignores all touch input. The ESP32 runs its state machine. The speed slider remains active — the firmware applies the current MQTT speed value in autonomous mode as well.

---

## Known Issues

| Priority | Issue | Status |
|----------|-------|--------|
| Critical | LM2596 trimpot-only FB risk — if the RV1 wiper fails open, the full battery voltage reaches the 5V rail and destroys the ESP32. Fix: add R_upper 1 kΩ (VOUT→FB) + R_lower 1 kΩ (FB→GND) with RV1 in series with R_lower as a failsafe floor | Next PCB revision |
| Warning | Low-ESR capacitor recommended on LM2596 220 µF output capacitor | To verify |
| Info | ECHO Right on GPIO2 — the onboard LED flashes in sync with right ultrasonic echoes. GPIO2 cannot be changed without a PCB revision. The sensor reads correctly; the flashing is cosmetic only. | Accepted |
| OK | L298N Schottky freewheeling diodes — repurposed from module, correct type and polarity | Done |
| OK | LM2596 D1 = SS34 Schottky | Done |
| OK | HC-SR04 2021+ — ECHO direct to ESP32 GPIO, no voltage divider | Done |
| OK | All IR GPIOs on ADC1 — WiFi and MQTT remain active during sensor reads | Done |
| OK | No strapping pin conflicts in final pinout | Done |
| OK | D34 battery ADC — ADC1, input-only GPIO, WiFi safe | Done |
| OK | J3: Pin 1 = GND, Pin 2 = battery positive (8.4V) | Done |
| OK | ESP32-DEVKITC V1 has onboard 100 nF decoupling — no external cap needed | Done |
| OK | GPIO36/GPIO39 do not exist on DEVKITC V1 — replaced with D19 and D2 | Done |
| OK | BLE invisible after first provisioning — fixed with 30 s re-provision window + `fresh` NVS flag | Done |
| OK | BLE and WiFi radio conflict — NimBLE kept alive and competed with WiFi — fixed with `stop_nimble()` clean handoff before WiFi init | Done |
| OK | OLED I2C blocking serial (720 ms/update) when no OLED connected — fixed with `i2c_master_probe()` presence check at init | Done |
| OK | Android broker IP returning cellular IP on dual-SIM phones — fixed by filtering `allNetworks` for WiFi transport only | Done |
| OK | Moquette failing silently (`catch Exception` missed Netty errors) — fixed with `catch Throwable` + `data_path = filesDir.absolutePath` | Done |
| OK | ESP32 ignoring the speed slider — added `robot/control/speed` subscription + `state_machine_set_speed()` | Done |
| OK | Motor A (left) IN1/IN2 wired backwards on PCB — corrected in `drive()` by inverting PIN_IN1/PIN_IN2 polarity for Motor A only | Done |
| OK | WASD fast-tap locks direction (`collectIsPressedAsState` missed short presses) — replaced with `pointerInput` + `awaitEachGesture` / `awaitFirstDown` / `waitForUpOrCancellation` | Done |
| OK | No AUTO/MAN mode toggle in app — AUTO/MAN button added to StatusBar; sends `line_follow` / `manual` to `robot/control/mode` | Done |
| OK | Autonomous speed ignored MQTT speed commands — all `m_*` motor helpers now scale from `s_speed` instead of hardcoded constants | Done |
| OK | Battery reading ~13V (wrong VREF, no calibration) — replaced with `adc_cali_line_fitting` + 8-sample averaging; fallback uses correct 3100 mV full-scale | Done |
| Warning | Battery divider 2×100 kΩ (1:1) saturates ADC above ~6.2V — a healthy 2S pack reads fixed at ~6.2V; only drops below when the pack is nearly dead. App thresholds set to 6.0V / 5.5V. | Hardware limitation — acceptable |
| OK | OLED I2C timeouts at 400 kHz — SCL reduced to 100 kHz, timeout extended to 50 ms; error counter: bus reset on 5th consecutive fail, auto-disable on 20th | Done |
| OK | Ultrasonic always reading 400 cm (polling loop missed short echo pulses < ~170 cm) — replaced with interrupt-driven ISR (`GPIO_INTR_ANYEDGE`) recording µs-precision timestamps | Done |

---

## Build Log

| Date | Entry |
|------|-------|
| 24 Apr 2026 | Rev 1 schematic complete. ESP32 + LM2596S-ADJ + L298N architecture established. |
| 24 Apr 2026 | EnA strapping conflict on D12 found and corrected → D21. |
| 24 Apr 2026 | R1/R2 confirmed as battery ADC voltage divider, not LM2596 feedback resistors. |
| 24 Apr 2026 | L298N repurposed from breakout module — correct Schottky freewheeling diodes confirmed. |
| 24 Apr 2026 | Android MQTT app architecture defined — embedded broker, dual control modes, telemetry dashboard. |
| 25 Apr 2026 | JST-A (ultrasonic) and JST-B (IR) connectors added to schematic and PCB. |
| 25 Apr 2026 | Full GPIO conflict check — all pins verified clean. |
| 25 Apr 2026 | HC-SR04 confirmed 2021+ version — voltage dividers removed from design. |
| 25 Apr 2026 | Final GPIO assignments locked in. LM2596 D1 = SS34 Schottky confirmed. |
| 25 Apr 2026 | Schematic Rev 2 exported — JST-A and JST-B fully connected. |
| 27 Apr 2026 | PCB design completed. L298N diode orientation corrected — all 8 diodes consistent anode/cathode orientation. |
| 27 Apr 2026 | Incorrect ESP32 footprint (WROOM-32D) found and replaced → ESP32-DEV DEVKITC V1. Custom footprints uploaded to `/footprints`. |
| 27 Apr 2026 | All pin numbers updated to DEVKITC V1 layout. GPIO36/39 replaced with D19/D2. 16 GPIOs used, 0 conflicts, 9 spare. |
| 27 Apr 2026 | DRC run — 3 silkscreen warnings fixed. 0 footprint errors. |
| 27 Apr 2026 | JST connector found placed upside down — fixed by flipping in KiCad PCB editor. |
| 27 Apr 2026 | Mounting hole references changed from numeric (1,2,3,4) to H1–H4 to resolve FreeRouter SES import error. |
| 27 Apr 2026 | FreeRouter auto-routing completed with 0 unrouted connections. Power netclass: 0.8 mm trace, 0.7 mm clearance. Signal: 0.25 mm trace, 0.2 mm clearance. |
| 27 Apr 2026 | SES imported into KiCad. All 28 DRC violations fixed: decorative text moved to F.Silkscreen, power clearance reduced to 0.5 mm to match IC pad spacing, 3 dangling stubs deleted. |
| 27 Apr 2026 | PCB routing complete. Final DRC: 0 errors, 2 warnings (mounting hole library mismatch — ignorable). Board is production-ready. |
| 27 Apr 2026 | GitHub URL added to PCB silkscreen. Gerber files exported and uploaded to `/gerber`. |
| 27 Apr 2026 | Power source confirmed: custom 2S 18650 pack (8.4V fully charged), charged via Hailege 2S USB-C BMS boost charger. |
| 27 Apr 2026 | Android app built and running. WASD + tilt mode, telemetry panel, speed slider. Dynamic tilt speed ramp, reverse guard, collapsible sensitivity panel added. |
| 27 Apr 2026 | Chassis design started in Autodesk Inventor. STEP file uploaded to repo. |
| 29 Apr 2026 | OLED board designed in KiCad — 128×32 SSD1306 I2C, separate PCB for mounting on chassis top. Connects via JST-B spare pins 7–10. ECHO Right moved D4 → D2; D4 and D5 assigned to OLED SDA/SCL. Gerbers added. |
| 29 Apr 2026 | Chassis assembly rendered in Autodesk Inventor. Assembly image added to repo. |
| 30 Apr 2026 | Final gerber review — main PCB and OLED board gerbers cross-checked. All layers verified (F.Cu, B.Cu, F.Silkscreen, B.Silkscreen, F.Mask, B.Mask, Edge.Cuts, drill). |
| 2 May 2026 | Main PCB and OLED board ordered from JLCPCB. 2-layer, HASL finish, 5 copies each. |
| 3 May 2026 | Firmware development started. All module headers defined: provisioning, sensors, state_machine, mqtt, oled. |
| 4 May 2026 | Chassis design finalised — mounting hole positions and motor mount spacing verified. |
| 4 May 2026 | sensors.c: IR reading with 2-read debounce, round-robin ultrasonic (non-blocking), battery ADC with voltage divider formula. state_machine.c skeleton. |
| 6 May 2026 | state_machine.c — 3-state machine (FOLLOWING / AVOIDING / RECOVERING) implemented. mqtt.c — WiFi connect, esp-mqtt event-driven client, telemetry JSON at 100 ms. |
| 7 May 2026 | provisioning.c — NimBLE GATT service, JSON credentials written to NVS. oled.c — SSD1306 raw driver, 5×7 font, battery + MQTT + mode/state display. |
| 8 May 2026 | main.c complete — all modules wired together and compiling. Chassis sent to 3D printer (PLA, 0.2 mm layer height, 20 % / 50 % infill). |
| 10 May 2026 | CLAUDE.md created. README audited and updated: OLED features, BLE provisioning section, constants. |
| 10 May 2026 | Auto-changelog hook configured in Claude Code — build log updated automatically after each code change. |
| 11 May 2026 | Full migration from Arduino to ESP-IDF v5.x. All .cpp files replaced with .c. Arduino framework, PubSubClient, ArduinoJson, and Adafruit SSD1306 removed. NimBLE replaces Bluedroid. esp-mqtt replaces PubSubClient. adc_oneshot API replaces legacy ADC. LEDC replaces ledcSetup/ledcWrite. I2C legacy driver with raw SSD1306 init sequence and 5×7 font inline. Motors merged into state_machine.c. Provisioning char upgraded to WRITE+NOTIFY. |
| 12 May 2026 | Android provisioning screen: broker IP auto-detected from phone's WiFi interface. WiFi network filter prevents cellular IP being selected on dual-SIM phones. Broker field is read-only with AUTO-DETECTED badge. |
| 12 May 2026 | HOW_TO_BUILD_AND_FLASH.txt updated: Ubuntu terminal open instructions, erase flash step corrected. |
| 13 May 2026 | BLE redesigned: 30 s re-provision window on every boot. `NVS_KEY_FRESH` flag skips BLE on boot immediately after provisioning. `stop_nimble()` (adv stop → port stop → 300 ms → deinit) called before WiFi init for clean radio handoff. |
| 13 May 2026 | OLED graceful no-hardware handling: `i2c_master_probe()` presence check at init. `s_present` guard in `oled_update()` eliminates 720 ms I2C blocking per loop when no OLED is connected. |
| 14 May 2026 | Android manifest: `FOREGROUND_SERVICE_CONNECTED_DEVICE` permission added — required for Android 14+ foreground service. |
| 14 May 2026 | Android `MainActivity`: fixed 1500 ms startup delay replaced with `MqttBrokerService.isRunning` poll — Paho connects only after Moquette has bound to port 1883. |
| 15 May 2026 | Moquette: `catch (Exception)` → `catch (Throwable)` so Netty startup errors surface in logcat. Root cause: H2 store path defaulting to `/data/` (permission denied). Fixed with `data_path = filesDir.absolutePath`. |
| 15 May 2026 | Android StatusBar: BROKER OK / BROKER DOWN indicator added as live Compose state. |
| 15 May 2026 | End-to-end MQTT verified: ESP32 connects to Moquette on phone, move / mode / speed commands received, telemetry published correctly. |
| 15 May 2026 | Speed control wired end-to-end: `TOPIC_SPEED` subscribed in `mqtt_init()`, `state_machine_set_speed()` added. `run_manual()` uses dynamic `s_speed` (turns use 2/3 of speed). |
| 19 May 2026 | PCBs arrived from JLCPCB. Buck converter circuit assembled (LM2596, diode, inductor, capacitors, trimpot, divider resistors). Output set to 5.0V and verified with multimeter. Battery ADC divider midpoint confirmed at ~4.2V. |
| 22 May 2026 | Remaining components soldered: ESP32, L298N, JST connectors, screw terminals. BLE provisioning functional — minor bugs noted: MQTT status indicator fires too early (shows connected before ESP32 reaches broker), GPIO watchdog trigger on sensor GPIOs. |
| 26 May 2026 | Motor driver tested — both channels functional. Mechanical on/off switch added to the battery line. |
| 28 May 2026 | Manual drive mode fully working. IR sensor data arriving in app correctly. Ultrasonic sensors under investigation (reading 400 cm). |
| 2 Jun 2026 | Motor A direction fixed — IN1/IN2 are wired backwards on the PCB hardware; corrected in `drive()` by inverting PIN_IN1/PIN_IN2 polarity for Motor A only. All move commands now correct. |
| 2 Jun 2026 | AUTO/MAN toggle added to app StatusBar. Sends `line_follow` / `manual` to `robot/control/mode`. D-pad shows overlay and ignores input when AUTO is active. |
| 2 Jun 2026 | WASD fast-tap fix — `collectIsPressedAsState` + `LaunchedEffect` replaced with `pointerInput` + `awaitEachGesture`. Fast taps now correctly send move on press and stop on release. |
| 2 Jun 2026 | Autonomous speed respects MQTT speed — all `m_*` motor helpers (forward, spin, curve, hard-turn) now scale from `s_speed`. Speed slider affects auto mode. |
| 2 Jun 2026 | Battery ADC calibration: `adc_cali_line_fitting` scheme added, 8-sample averaging, fallback uses 3100 mV full-scale (was 3300 mV). App thresholds updated to 6.0V / 5.5V to account for the 6.2V ADC saturation ceiling. |
| 2 Jun 2026 | OLED I2C stability: SCL reduced from 400 kHz to 100 kHz, timeout extended from 10 ms to 50 ms. Error counter: `i2c_master_bus_reset` on 5th consecutive fail, `s_present = false` on 20th (permanently stops updates if OLED disconnects). |
| 2 Jun 2026 | Ultrasonic sensors fixed: polling state machine replaced with interrupt-driven ISR (`GPIO_INTR_ANYEDGE`). Rising edge stores timestamp, falling edge computes `pulse_us / 58` cm. All three sensors now read correctly at all distances. |
| 2 Jun 2026 | ECHO Right confirmed on GPIO2 with ISR. GPIO36/39 were tried and rejected due to ESP32 WiFi/RTC errata causing spurious ISR triggers. Onboard LED flashing with echoes is cosmetic — accepted. |
