#pragma once

// =============================================================================
// config.h — Single source of truth for all constants and pin definitions
// Edit values here only. Do not hardcode numbers in other files.
// =============================================================================

// -----------------------------------------------------------------------------
// WiFi
// -----------------------------------------------------------------------------
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------
#define MQTT_BROKER_IP  "PHONE_LOCAL_IP"   // IP of Android phone running broker
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "ESP32Robot"

// MQTT Topics — Subscribe
#define TOPIC_MOVE      "robot/control/move"
#define TOPIC_SPEED     "robot/control/speed"
#define TOPIC_MODE      "robot/control/mode"

// MQTT Topics — Publish
#define TOPIC_IR        "robot/telemetry/ir"
#define TOPIC_ULTRA     "robot/telemetry/ultrasonic"
#define TOPIC_BATTERY   "robot/telemetry/battery"
#define TOPIC_SPEED_FB  "robot/telemetry/speed"

// -----------------------------------------------------------------------------
// Motor Pins
// -----------------------------------------------------------------------------
#define PIN_IN1   13    // Motor A direction 1
#define PIN_IN2   14    // Motor A direction 2
#define PIN_ENA   21    // Motor A enable (PWM)
#define PIN_IN3   27    // Motor B direction 1
#define PIN_IN4   26    // Motor B direction 2
#define PIN_ENB   25    // Motor B enable (PWM)

// PWM config for motor channels
#define PWM_CHANNEL_A   0
#define PWM_CHANNEL_B   1
#define PWM_FREQ        1000
#define PWM_RESOLUTION  8       // 8-bit = 0-255

// -----------------------------------------------------------------------------
// IR Sensor Pins
// -----------------------------------------------------------------------------
#define PIN_IR_LEFT     32
#define PIN_IR_CENTER   33
#define PIN_IR_RIGHT    15

// IR logic — set to 1 if sensor outputs HIGH on line, 0 if LOW on line
#define IR_LINE_VALUE   0

// Debounce — number of consecutive matching reads before accepting
#define IR_DEBOUNCE_READS  2

// -----------------------------------------------------------------------------
// Ultrasonic Sensor Pins
// -----------------------------------------------------------------------------
#define PIN_TRIG_LEFT   22
#define PIN_ECHO_LEFT   35
#define PIN_TRIG_CENTER 23
#define PIN_ECHO_CENTER 19
#define PIN_TRIG_RIGHT  18
#define PIN_ECHO_RIGHT   2

// Ultrasonic timing
#define ULTRA_TIMEOUT_US    25000   // max echo wait in microseconds (~4m range)
#define ULTRA_CYCLE_MS      20      // ms between triggering each sensor

// -----------------------------------------------------------------------------
// Battery ADC
// -----------------------------------------------------------------------------
#define PIN_BATTERY     34
#define BATTERY_R1      10000.0f    // voltage divider R1 (ohms) — adjust to match your resistors
#define BATTERY_R2      3300.0f     // voltage divider R2 (ohms)
#define BATTERY_VREF    3.3f        // ESP32 ADC reference voltage
#define BATTERY_ADC_MAX 4095.0f     // 12-bit ADC

// -----------------------------------------------------------------------------
// OLED I2C
// -----------------------------------------------------------------------------
#define PIN_OLED_SDA    4
#define PIN_OLED_SCL    5
#define OLED_ADDRESS    0x3C
#define OLED_WIDTH      128
#define OLED_HEIGHT     32
#define OLED_UPDATE_MS  500         // refresh rate in ms

// -----------------------------------------------------------------------------
// Speed Constants
// -----------------------------------------------------------------------------
#define SPEED_BASE      180         // normal line following speed
#define SPEED_TURN      120         // inner wheel speed during gentle curve
#define SPEED_HARD_TURN 0           // inner wheel speed during hard turn
#define SPEED_SLOW      80          // recovery / avoidance crawl speed
#define SPEED_REVERSE   150         // reverse speed

// -----------------------------------------------------------------------------
// Obstacle Avoidance Thresholds (cm)
// -----------------------------------------------------------------------------
#define OBSTACLE_WARN_CM    20      // start curving around obstacle
#define OBSTACLE_STOP_CM    12      // stop + reverse + turn away

// -----------------------------------------------------------------------------
// State Machine Timing
// -----------------------------------------------------------------------------
#define RECOVERY_SWEEP_MS   2000    // time to sweep last known direction before spinning
#define RECOVERY_SPIN_MS    5000    // time to spin before giving up
#define AVOID_MIN_MS        500     // minimum time to stay in AVOIDING before re-checking line

// -----------------------------------------------------------------------------
// Telemetry
// -----------------------------------------------------------------------------
#define TELEMETRY_INTERVAL_MS  100  // how often to publish MQTT telemetry
