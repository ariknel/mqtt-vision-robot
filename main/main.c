#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "config.h"
#include "provisioning.h"
#include "sensors.h"
#include "state_machine.h"
#include "mqtt.h"
#include "oled.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "MQTTVision booting");

    /* Provisioning blocks on first boot until BLE delivers credentials,
     * then restarts. On subsequent boots it loads from NVS and returns. */
    provisioning_init();

    sensors_init();
    state_machine_init();
    mqtt_init();
    oled_init();

    ESP_LOGI(TAG, "All modules ready — entering main loop");

    TickType_t last_telem = xTaskGetTickCount();
    TickType_t last_oled  = xTaskGetTickCount();

    while (1) {
        IRData    ir      = sensors_read_ir();
        UltraData ultra   = sensors_read_ultrasonic();
        float     battery = sensors_read_battery();

        state_machine_update(ir, ultra);

        TickType_t now = xTaskGetTickCount();

        if (now - last_telem >= pdMS_TO_TICKS(TELEMETRY_INTERVAL_MS)) {
            last_telem = now;
            mqtt_publish_telemetry(ir, ultra, battery);
        }

        if (now - last_oled >= pdMS_TO_TICKS(OLED_UPDATE_MS)) {
            last_oled = now;
            oled_update(battery, mqtt_connected(),
                        state_machine_get_mode(), state_machine_get_state());
        }

        vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_MS));
    }
}
