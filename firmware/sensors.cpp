#include "sensors.h"
#include "config.h"
#include <Arduino.h>

// =============================================================================
// sensors.cpp — Sensor implementation
// Ultrasonic reads are non-blocking — one sensor triggered per loop cycle.
// IR reads are debounced — must be stable for IR_DEBOUNCE_READS cycles.
// =============================================================================

// -----------------------------------------------------------------------------
// IR state — debounce tracking
// -----------------------------------------------------------------------------
static bool irRaw[3]     = {false, false, false};
static bool irStable[3]  = {false, false, false};
static int  irCount[3]   = {0, 0, 0};

// -----------------------------------------------------------------------------
// Ultrasonic state — non-blocking rotation
// -----------------------------------------------------------------------------
static int      ultraIndex = 0;           // which sensor is currently being read (0=L, 1=C, 2=R)
static unsigned long ultraLastTrigger = 0;
static bool     ultraWaiting = false;     // true = pulse sent, waiting for echo

// Latest known distances — updated one sensor at a time
static UltraData ultraLatest = {999, 999, 999};

static const int TRIG_PINS[3] = { PIN_TRIG_LEFT,   PIN_TRIG_CENTER,  PIN_TRIG_RIGHT  };
static const int ECHO_PINS[3] = { PIN_ECHO_LEFT,   PIN_ECHO_CENTER,  PIN_ECHO_RIGHT  };

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------
void sensorsInit() {
    // IR pins
    pinMode(PIN_IR_LEFT,   INPUT);
    pinMode(PIN_IR_CENTER, INPUT);
    pinMode(PIN_IR_RIGHT,  INPUT);

    // Ultrasonic pins
    for (int i = 0; i < 3; i++) {
        pinMode(TRIG_PINS[i], OUTPUT);
        pinMode(ECHO_PINS[i], INPUT);
        digitalWrite(TRIG_PINS[i], LOW);
    }

    // Battery ADC
    analogReadResolution(12);
}

// -----------------------------------------------------------------------------
// IR — debounced read
// Returns true for each sensor where line is detected
// -----------------------------------------------------------------------------
IRData readIR() {
    bool rawNow[3] = {
        digitalRead(PIN_IR_LEFT)   == IR_LINE_VALUE,
        digitalRead(PIN_IR_CENTER) == IR_LINE_VALUE,
        digitalRead(PIN_IR_RIGHT)  == IR_LINE_VALUE
    };

    for (int i = 0; i < 3; i++) {
        if (rawNow[i] == irRaw[i]) {
            irCount[i]++;
            if (irCount[i] >= IR_DEBOUNCE_READS) {
                irStable[i] = irRaw[i];
            }
        } else {
            irRaw[i]  = rawNow[i];
            irCount[i] = 0;
        }
    }

    return { irStable[0], irStable[1], irStable[2] };
}

// -----------------------------------------------------------------------------
// Ultrasonic — non-blocking rotating read
// One sensor per ULTRA_CYCLE_MS — rotates L → C → R → L → ...
// Returns latest known values for all three sensors
// -----------------------------------------------------------------------------
UltraData readUltrasonic() {
    unsigned long now = millis();

    if (!ultraWaiting) {
        // Time to trigger the next sensor?
        if (now - ultraLastTrigger >= ULTRA_CYCLE_MS) {
            // Send trigger pulse
            digitalWrite(TRIG_PINS[ultraIndex], LOW);
            delayMicroseconds(2);
            digitalWrite(TRIG_PINS[ultraIndex], HIGH);
            delayMicroseconds(10);
            digitalWrite(TRIG_PINS[ultraIndex], LOW);

            ultraLastTrigger = now;
            ultraWaiting = true;
        }
    } else {
        // Read echo with timeout
        long duration = pulseIn(ECHO_PINS[ultraIndex], HIGH, ULTRA_TIMEOUT_US);
        int distanceCm = (duration == 0) ? 999 : (int)(duration * 0.034f / 2.0f);

        // Store result for this sensor
        if (ultraIndex == 0) ultraLatest.left   = distanceCm;
        if (ultraIndex == 1) ultraLatest.center = distanceCm;
        if (ultraIndex == 2) ultraLatest.right  = distanceCm;

        // Move to next sensor
        ultraIndex = (ultraIndex + 1) % 3;
        ultraWaiting = false;
    }

    return ultraLatest;
}

// -----------------------------------------------------------------------------
// Battery — voltage calculation from ADC reading
// Uses voltage divider: Vbat = Vadc * (R1 + R2) / R2
// -----------------------------------------------------------------------------
float readBattery() {
    int raw = analogRead(PIN_BATTERY);
    float vadc = (raw / BATTERY_ADC_MAX) * BATTERY_VREF;
    float vbat = vadc * ((BATTERY_R1 + BATTERY_R2) / BATTERY_R2);
    return vbat;
}
