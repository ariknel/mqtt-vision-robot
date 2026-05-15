#include "mqtt.h"
#include "config.h"
#include "provisioning.h"
#include "state_machine.h"
#include <string.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "MQTT";

#define WIFI_CONNECTED_BIT  BIT0
#define MQTT_CONNECTED_BIT  BIT1

static EventGroupHandle_t        s_events;
static esp_mqtt_client_handle_t  s_client  = NULL;
static volatile bool             s_mqtt_up = false;

static void wifi_handler(void *arg, esp_event_base_t base,
                         int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi lost — reconnecting");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = data;
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_events, WIFI_CONNECTED_BIT);
    }
}

static void mqtt_handler(void *arg, esp_event_base_t base,
                         int32_t id, void *data)
{
    esp_mqtt_event_handle_t ev = data;

    switch (id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            s_mqtt_up = true;
            xEventGroupSetBits(s_events, MQTT_CONNECTED_BIT);
            esp_mqtt_client_subscribe(s_client, TOPIC_MOVE, 0);
            esp_mqtt_client_subscribe(s_client, TOPIC_MODE, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            s_mqtt_up = false;
            break;
        case MQTT_EVENT_DATA: {
            char topic[64] = {0};
            char msg[64]   = {0};
            int tlen = ev->topic_len < 63 ? ev->topic_len : 63;
            int mlen = ev->data_len  < 63 ? ev->data_len  : 63;
            memcpy(topic, ev->topic, tlen);
            memcpy(msg,   ev->data,  mlen);
            if (strcmp(topic, TOPIC_MOVE) == 0)
                state_machine_set_move(msg);
            else if (strcmp(topic, TOPIC_MODE) == 0)
                state_machine_set_mode(strcmp(msg, "line_follow") == 0 ? MODE_LINE_FOLLOW : MODE_MANUAL);
            break;
        }
        default: break;
    }
}

void mqtt_init(void)
{
    s_events = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wcfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    wifi_handler, NULL);
    esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, wifi_handler, NULL);

    wifi_config_t sta = { .sta = { .threshold.authmode = WIFI_AUTH_WPA2_PSK } };
    strncpy((char *)sta.sta.ssid,     provisioning_get_ssid(),     sizeof(sta.sta.ssid) - 1);
    strncpy((char *)sta.sta.password, provisioning_get_password(), sizeof(sta.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &sta);
    esp_wifi_start();
    esp_wifi_connect();

    EventBits_t bits = xEventGroupWaitBits(s_events, WIFI_CONNECTED_BIT,
                                           pdFALSE, pdTRUE, pdMS_TO_TICKS(10000));
    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGW(TAG, "WiFi timeout — MQTT skipped");
        return;
    }

    char uri[80];
    snprintf(uri, sizeof(uri), "mqtt://%s:%d", provisioning_get_broker(), MQTT_PORT);
    esp_mqtt_client_config_t mcfg = { .broker.address.uri = uri };
    s_client = esp_mqtt_client_init(&mcfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_handler, NULL);
    esp_mqtt_client_start(s_client);

    xEventGroupWaitBits(s_events, MQTT_CONNECTED_BIT,
                        pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
}

bool mqtt_connected(void) { return s_mqtt_up; }

void mqtt_publish_telemetry(IRData ir, UltraData ultra, float battery)
{
    if (!s_mqtt_up || !s_client) return;

    char buf[96];
    snprintf(buf, sizeof(buf), "{\"left\":%d,\"center\":%d,\"right\":%d}",
             ir.left, ir.center, ir.right);
    esp_mqtt_client_publish(s_client, TOPIC_IR, buf, 0, 0, 0);

    snprintf(buf, sizeof(buf), "{\"left\":%d,\"center\":%d,\"right\":%d}",
             ultra.left, ultra.center, ultra.right);
    esp_mqtt_client_publish(s_client, TOPIC_ULTRA, buf, 0, 0, 0);

    snprintf(buf, sizeof(buf), "%.2f", battery);
    esp_mqtt_client_publish(s_client, TOPIC_BATTERY, buf, 0, 0, 0);
}
