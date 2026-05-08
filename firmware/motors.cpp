#include "motors.h"
#include "config.h"
#include <Arduino.h>

// =============================================================================
// motors.cpp — Motor implementation
// Translates speed values (-255 to +255) to L298N PWM signals.
// Negative speed = reverse direction for that motor.
// =============================================================================

void motorsInit() {
    // Set direction pins as outputs
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_IN3, OUTPUT);
    pinMode(PIN_IN4, OUTPUT);

    // Configure PWM channels for enable pins
    ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_ENA, PWM_CHANNEL_A);
    ledcAttachPin(PIN_ENB, PWM_CHANNEL_B);

    stopMotors();
}

// -----------------------------------------------------------------------------
// Core function — everything else calls this
// leftSpeed / rightSpeed: -255 (full reverse) to +255 (full forward)
// -----------------------------------------------------------------------------
void setMotors(int leftSpeed, int rightSpeed) {
    // Clamp values
    leftSpeed  = constrain(leftSpeed,  -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    // Motor A (left)
    if (leftSpeed >= 0) {
        digitalWrite(PIN_IN1, HIGH);
        digitalWrite(PIN_IN2, LOW);
    } else {
        digitalWrite(PIN_IN1, LOW);
        digitalWrite(PIN_IN2, HIGH);
    }
    ledcWrite(PWM_CHANNEL_A, abs(leftSpeed));

    // Motor B (right)
    if (rightSpeed >= 0) {
        digitalWrite(PIN_IN3, HIGH);
        digitalWrite(PIN_IN4, LOW);
    } else {
        digitalWrite(PIN_IN3, LOW);
        digitalWrite(PIN_IN4, HIGH);
    }
    ledcWrite(PWM_CHANNEL_B, abs(rightSpeed));
}

// -----------------------------------------------------------------------------
// High-level commands
// -----------------------------------------------------------------------------
void forward(int speed) {
    setMotors(speed, speed);
}

void reverse(int speed) {
    setMotors(-speed, -speed);
}

void stopMotors() {
    setMotors(0, 0);
}

void spinLeft(int speed) {
    setMotors(-speed, speed);
}

void spinRight(int speed) {
    setMotors(speed, -speed);
}

// Curves — inner wheel slowed to SPEED_TURN, outer stays at baseSpeed
void curveLeft(int baseSpeed) {
    setMotors(SPEED_TURN, baseSpeed);
}

void curveRight(int baseSpeed) {
    setMotors(baseSpeed, SPEED_TURN);
}

// Hard turns — inner wheel fully stopped
void hardLeft(int baseSpeed) {
    setMotors(SPEED_HARD_TURN, baseSpeed);
}

void hardRight(int baseSpeed) {
    setMotors(baseSpeed, SPEED_HARD_TURN);
}
