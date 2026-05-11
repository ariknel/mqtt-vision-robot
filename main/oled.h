#pragma once
#include <stdbool.h>
#include "state_machine.h"

void oled_init(void);
void oled_update(float battery, bool mqtt_ok, RobotMode mode, RobotState state);
