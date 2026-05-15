#pragma once

typedef struct { int left, center, right; } IRData;
typedef struct { int left, center, right; } UltraData;

void      sensors_init(void);
IRData    sensors_read_ir(void);
UltraData sensors_read_ultrasonic(void);
float     sensors_read_battery(void);
