#pragma once

// =============================================================================
// motors.h — Motor abstraction layer
// All motor control goes through these functions.
// No logic, no sensors — pure hardware interface only.
// =============================================================================

// Call once in setup()
void motorsInit();

// -----------------------------------------------------------------------------
// High-level movement commands
// All call setMotors() internally
// -----------------------------------------------------------------------------
void forward(int speed);
void reverse(int speed);
void stopMotors();

// Differential steering — leftSpeed and rightSpeed set independently
// Use for gentle curves and hard turns
void setMotors(int leftSpeed, int rightSpeed);

// Spin in place (opposite directions)
void spinLeft(int speed);
void spinRight(int speed);

// Convenience curve helpers used by state machine
void curveLeft(int baseSpeed);   // left slower, right full
void curveRight(int baseSpeed);  // right slower, left full
void hardLeft(int baseSpeed);    // left stopped, right full
void hardRight(int baseSpeed);   // right stopped, left full
