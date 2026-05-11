#pragma once
#include <stdbool.h>
#include "sensors.h"

void  mqtt_init(void);
bool  mqtt_connected(void);
void  mqtt_publish_telemetry(IRData ir, UltraData ultra, float battery);
