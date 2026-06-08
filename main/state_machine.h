#pragma once
#include "sensors.h"

typedef enum { MODE_MANUAL, MODE_LINE_FOLLOW } RobotMode;
typedef enum { STATE_MANUAL, STATE_FOLLOWING, STATE_AVOIDING, STATE_RECOVERING, STATE_LIFTED } RobotState;

void       state_machine_init(void);
void       state_machine_update(IRData ir, UltraData ultra);
void       state_machine_set_mode(RobotMode mode);
void       state_machine_set_move(const char *cmd);
void       state_machine_set_speed(int speed);
RobotMode  state_machine_get_mode(void);
RobotState state_machine_get_state(void);
