#pragma once

/* MQTT */
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "ESP32Robot"

/* MQTT topics */
#define TOPIC_MOVE     "robot/control/move"
#define TOPIC_MODE     "robot/control/mode"
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
#define PIN_ECHO_RIGHT    2
#define ULTRA_TIMEOUT_US  25000  /* ~4 m max, abort pulse wait after this */

/* Battery ADC — GPIO34 = ADC1_CH6, voltage divider R1=10k R2=3k3 */
#define PIN_BATTERY      34
#define BATTERY_R1       10000.0f
#define BATTERY_R2        3300.0f
#define BATTERY_VREF         3.3f
#define BATTERY_ADC_MAX   4095.0f

/* OLED — SSD1306 128x32, I2C remapped to GPIO4/5 */
#define PIN_OLED_SDA   4
#define PIN_OLED_SCL   5
#define OLED_ADDRESS   0x3C

/* Speed — 8-bit PWM duty */
#define SPEED_BASE       180
#define SPEED_TURN       120
#define SPEED_HARD_TURN    0
#define SPEED_SLOW        80
#define SPEED_REVERSE    150

/* Obstacle thresholds (cm) */
#define OBSTACLE_WARN_CM  20
#define OBSTACLE_STOP_CM  12

/* State machine timing */
#define RECOVERY_SWEEP_MS  2000
#define RECOVERY_SPIN_MS   5000
#define AVOID_MIN_MS        500

/* Loop intervals */
#define TELEMETRY_INTERVAL_MS  100
#define OLED_UPDATE_MS         500
#define MAIN_LOOP_MS            10
