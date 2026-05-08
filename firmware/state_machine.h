#pragma once

#include "sensors.h"

// =============================================================================
// state_machine.h — Robot brain
// All autonomous decisions made here.
// Only file allowed to call motor functions based on sensor data.
// =============================================================================

// -----------------------------------------------------------------------------
// Robot states
// -----------------------------------------------------------------------------
enum RobotState {
    STATE_MANUAL,       // App controls motors directly via MQTT
    STATE_FOLLOWING,    // Autonomous line following
    STATE_AVOIDING,     // Obstacle detected — navigating around it
    STATE_RECOVERING    // Line lost — searching
};

enum RobotMode {
    MODE_MANUAL,
    MODE_LINE_FOLLOW
};

// -----------------------------------------------------------------------------
// Public interface
// -----------------------------------------------------------------------------
void stateMachineInit();
void stateMachineUpdate(IRData ir, UltraData ultra);

// Called by mqtt_client when commands arrive
void setMode(RobotMode mode);
void setManualMove(const char* direction);   // "forward","back","left","right","stop"

// Called by main.cpp / oled for display
RobotState getCurrentState();
RobotMode  getCurrentMode();
int        getCurrentSpeedLeft();
int        getCurrentSpeedRight();
