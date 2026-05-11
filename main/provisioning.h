#pragma once
#include "esp_err.h"

/*
 * BLE provisioning service — first-boot credential delivery.
 *
 * On first boot (no NVS credentials):
 *   Advertises as "IoT-Robot". App connects, writes a JSON credential
 *   to the credential characteristic. Device replies with "OK" via
 *   notify, then restarts and connects to WiFi.
 *
 * Subsequent boots:
 *   Loads credentials from NVS and returns immediately.
 *
 * BLE service UUID  : 4fafc201-1fb5-459e-8fcc-c5c9c331914b
 * Credential char   : beb5483e-36e1-4688-b7f5-ea07361b26a8
 *   WRITE  — send JSON: {"ssid":"...","password":"...","broker":"..."}
 *   NOTIFY — reply:     "OK" on success, "ERR" on bad JSON
 *
 * Call provisioning_reset() to wipe NVS and re-enter provisioning mode.
 */

esp_err_t   provisioning_init(void);
const char *provisioning_get_ssid(void);
const char *provisioning_get_password(void);
const char *provisioning_get_broker(void);
void        provisioning_reset(void);
