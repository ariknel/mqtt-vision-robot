#pragma once

// =============================================================================
// sensors.h — All sensor reading
// Pure read functions only. No motor calls, no state changes.
// =============================================================================

// -----------------------------------------------------------------------------
// Data structs — passed around the codebase cleanly
// -----------------------------------------------------------------------------
struct IRData {
    bool left;
    bool center;
    bool right;
};

struct UltraData {
    int left;    // cm
    int center;  // cm
    int right;   // cm
};

// Call once in setup()
void sensorsInit();

// IR — returns debounced readings
// true = line detected under that sensor
IRData readIR();

// Ultrasonic — non-blocking rotating read
// Call every loop() — internally cycles through left/center/right
// Returns latest known distances for all three
UltraData readUltrasonic();

// Battery — returns actual voltage in V
float readBattery();
