#pragma once

/* MQTT */
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "ESP32Robot"

/* MQTT topics */
#define TOPIC_MOVE     "robot/control/move"
#define TOPIC_MODE     "robot/control/mode"
#define TOPIC_SPEED    "robot/control/speed"
#define TOPIC_IR       "robot/telemetry/ir"
#define TOPIC_ULTRA    "robot/telemetry/ultrasonic"
#define TOPIC_BATTERY  "robot/telemetry/battery"

/* Motor pins — L298N */
#define PIN_IN1  13
#define PIN_IN2  14
#define PIN_ENA  21
#define PIN_IN3  27
#define PIN_IN4  26
#define PIN_ENB  25

/* PWM — 1 kHz, 8-bit (0-255) via LEDC */
#define PWM_FREQ  1000

/* IR sensors */
#define PIN_IR_LEFT    32
#define PIN_IR_CENTER  33
#define PIN_IR_RIGHT   15
#define IR_LINE_VALUE      0   /* sensor output when on the line */
#define IR_DEBOUNCE_READS  2

/* Ultrasonic sensors — HC-SR04 (3.3 V echo) */
#define PIN_TRIG_LEFT    22
#define PIN_ECHO_LEFT    35
#define PIN_TRIG_CENTER  23
#define PIN_ECHO_CENTER  19
#define PIN_TRIG_RIGHT   18
#define PIN_ECHO_RIGHT    2  /* GPIO2 — onboard LED flashes with echoes (cosmetic only) */
#define ULTRA_TIMEOUT_US  25000  /* ~4 m max, abort pulse wait after this */

/* Battery ADC — GPIO34 = ADC1_CH6, voltage divider R1=100k R2=100k (1:1).
 * V_pin = Vbat/2.  ADC DB_12 ceiling ~3.1V → saturates above ~6.2V.
 * For 2S this means the reading is fixed at ~6.2V when battery is healthy,
 * and only starts dropping below 6.2V when the pack is critically low. */
#define PIN_BATTERY      34
#define BATTERY_R1       100000.0f
#define BATTERY_R2       100000.0f
#define BATTERY_ADC_MAX    4095.0f

/* OLED — SSD1306 128x32, I2C remapped to GPIO4/5 */
#define PIN_OLED_SDA   4
#define PIN_OLED_SCL   5
#define OLED_ADDRESS   0x3C

/* Speed — 8-bit PWM duty */
#define SPEED_BASE         150  /* default driving speed, overridden by app slider */
#define SPEED_AVOID         90  /* obstacle avoidance — barely moving, cautious */
#define SPEED_CORRECTION    75  /* correction turn speed — gentle, never max RPM */
#define SPEED_POST_CORR     80  /* forward speed when first re-acquiring line */
#define SPEED_RAMP_STEP      2  /* PWM duty added per 10 ms tick while on-line */
#define RECOVERY_CREEP_MS  150  /* creep forward this long after line fully lost before stopping */

/* Obstacle thresholds (cm) */
#define OBSTACLE_WARN_CM  20
#define OBSTACLE_STOP_CM  12

/* State machine timing */
#define RECOVERY_SWEEP_MS   2000
#define RECOVERY_SPIN_MS    5000
#define AVOID_MIN_MS         500
#define POST_AVOID_FWD_MS    600  /* forward creep after obstacle cleared before re-acquiring line */

/* Loop intervals */
#define TELEMETRY_INTERVAL_MS  100
#define OLED_UPDATE_MS         500
#define MAIN_LOOP_MS            10

/* BLE provisioning — re-provision window on every boot (ms) */
#define BLE_PROV_WINDOW_MS  30000
