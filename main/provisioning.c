#include "provisioning.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "PROV";

#define NVS_NAMESPACE  "robot_cfg"
#define NVS_KEY_SSID   "ssid"
#define NVS_KEY_PASS   "password"
#define NVS_KEY_BROKER "broker"
#define NVS_KEY_FRESH  "fresh"   /* uint8_t: set when creds written, cleared after read */
#define CREDS_OK_BIT   BIT0

static EventGroupHandle_t s_events;
static uint16_t s_conn_handle  = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_cred_val_hdl = 0;
static bool     s_ble_stopping = false;

static char s_ssid[64]     = {0};
static char s_password[64] = {0};
static char s_broker[64]   = {0};

/* ── UUIDs (NimBLE = LSB first, opposite of UUID string) ─────────────────
 * Service:  4fafc201-1fb5-459e-8fcc-c5c9c331914b
 * Cred char: beb5483e-36e1-4688-b7f5-ea07361b26a8
 */
static const ble_uuid128_t s_svc_uuid = BLE_UUID128_INIT(
    0x4b,0x91,0x31,0xc3,0xc9,0xc5,0xcc,0x8f,
    0x9e,0x45,0xb5,0x1f,0x01,0xc2,0xaf,0x4f
);
static const ble_uuid128_t s_cred_uuid = BLE_UUID128_INIT(
    0xa8,0x26,0x1b,0x36,0x07,0xea,0xf5,0xb7,
    0x88,0x46,0xe1,0x36,0x3e,0x48,0xb5,0xbe
);

/* ── Notify helper ────────────────────────────────────────────────────── */
static void notify_str(const char *str)
{
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE || s_cred_val_hdl == 0) return;
    struct os_mbuf *om = ble_hs_mbuf_from_flat(str, strlen(str));
    if (om) ble_gatts_notify_custom(s_conn_handle, s_cred_val_hdl, om);
}

/* ── GATT write callback ──────────────────────────────────────────────── */
static int cred_chr_cb(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return 0;

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    char *buf = malloc(len + 1);
    if (!buf) return BLE_ATT_ERR_INSUFFICIENT_RES;
    ble_hs_mbuf_to_flat(ctxt->om, buf, len, NULL);
    buf[len] = '\0';

    cJSON *root     = cJSON_Parse(buf);
    free(buf);
    if (!root) { notify_str("ERR"); return 0; }

    cJSON *ssid_j   = cJSON_GetObjectItem(root, "ssid");
    cJSON *pass_j   = cJSON_GetObjectItem(root, "password");
    cJSON *broker_j = cJSON_GetObjectItem(root, "broker");

    if (!cJSON_IsString(ssid_j) || !ssid_j->valuestring[0] ||
        !cJSON_IsString(broker_j) || !broker_j->valuestring[0]) {
        cJSON_Delete(root);
        notify_str("ERR");
        return 0;
    }

    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, NVS_KEY_SSID,   ssid_j->valuestring);
        nvs_set_str(nvs, NVS_KEY_PASS,   cJSON_IsString(pass_j) ? pass_j->valuestring : "");
        nvs_set_str(nvs, NVS_KEY_BROKER, broker_j->valuestring);
        uint8_t fresh = 1;
        nvs_set_u8(nvs, NVS_KEY_FRESH, fresh);
        nvs_commit(nvs);
        nvs_close(nvs);
    }

    strncpy(s_ssid,     ssid_j->valuestring,                                sizeof(s_ssid) - 1);
    strncpy(s_password, cJSON_IsString(pass_j) ? pass_j->valuestring : "", sizeof(s_password) - 1);
    strncpy(s_broker,   broker_j->valuestring,                              sizeof(s_broker) - 1);
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Provisioned — ssid=%s broker=%s", s_ssid, s_broker);
    notify_str("OK");
    xEventGroupSetBits(s_events, CREDS_OK_BIT);
    return 0;
}

/* ── GATT service table ───────────────────────────────────────────────── */
static const struct ble_gatt_svc_def s_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &s_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid       = &s_cred_uuid.u,
                .access_cb  = cred_chr_cb,
                /* WRITE: app sends credentials; NOTIFY: device confirms OK/ERR */
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_cred_val_hdl,
            },
            { 0 }
        },
    },
    { 0 }
};

/* ── GAP event handler ────────────────────────────────────────────────── */
static int gap_event_cb(struct ble_gap_event *ev, void *arg)
{
    switch (ev->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (ev->connect.status == 0) {
                s_conn_handle = ev->connect.conn_handle;
                ESP_LOGI(TAG, "App connected");
            } else {
                s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            if (!s_ble_stopping) {
                ESP_LOGI(TAG, "App disconnected — re-advertising");
                ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                                  &(struct ble_gap_adv_params){
                                      .conn_mode = BLE_GAP_CONN_MODE_UND,
                                      .disc_mode = BLE_GAP_DISC_MODE_GEN,
                                  }, gap_event_cb, NULL);
            }
            break;
        default:
            break;
    }
    return 0;
}

/* ── Advertising ──────────────────────────────────────────────────────── */
static void start_advertising(void)
{
    const char *name = ble_svc_gap_device_name();

    struct ble_hs_adv_fields fields = {0};
    fields.flags            = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name             = (const uint8_t *)name;
    fields.name_len         = strlen(name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
    };
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &params, gap_event_cb, NULL);
    ESP_LOGI(TAG, "Advertising as '%s'", name);
}

static void on_sync(void)  { ble_hs_util_ensure_addr(0); start_advertising(); }
static void on_reset(int r){ ESP_LOGE(TAG, "BLE host reset: %d", r); }

static void nimble_host_task(void *p)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
    vTaskDelete(NULL);
}

/* ── NVS init ─────────────────────────────────────────────────────────── */
static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS erase required");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return err;
}

/* ── Start NimBLE stack ───────────────────────────────────────────────── */
static void start_nimble(void)
{
    nimble_port_init();
    ble_hs_cfg.sync_cb  = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set("IoT-Robot");
    ble_gatts_count_cfg(s_svcs);
    ble_gatts_add_svcs(s_svcs);
    nimble_port_freertos_init(nimble_host_task);
}

/* ── Stop NimBLE and release the radio before WiFi starts ────────────── */
static void stop_nimble(void)
{
    s_ble_stopping = true;
    ble_gap_adv_stop();
    nimble_port_stop();
    vTaskDelay(pdMS_TO_TICKS(300));   /* host task drains event queue and exits */
    nimble_port_deinit();
    ESP_LOGI(TAG, "BLE stopped — radio handed to WiFi");
}

/* ── Public API ───────────────────────────────────────────────────────── */

/*
 * Boot sequence:
 *
 *   fresh flag set  ─────────────────────────────► skip BLE → WiFi/MQTT  (fast)
 *   no credentials  → BLE (forever) → creds arrive → set fresh → restart
 *   has_creds, no fresh → BLE window (BLE_PROV_WINDOW_MS)
 *       creds arrive  → set fresh → notify OK → restart  (fast next boot)
 *       window expires → stop_nimble() ──────────► WiFi/MQTT
 *
 * BLE and WiFi/MQTT never run simultaneously — stop_nimble() or esp_restart()
 * always separates the two phases.
 */
esp_err_t provisioning_init(void)
{
    esp_err_t err = init_nvs();
    if (err != ESP_OK) return err;

    s_events = xEventGroupCreate();

    /* Load credentials and fresh flag from NVS */
    bool has_creds = false;
    bool is_fresh  = false;
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        size_t n;
        n = sizeof(s_ssid);     nvs_get_str(nvs, NVS_KEY_SSID,   s_ssid,     &n);
        n = sizeof(s_password); nvs_get_str(nvs, NVS_KEY_PASS,   s_password, &n);
        n = sizeof(s_broker);   nvs_get_str(nvs, NVS_KEY_BROKER, s_broker,   &n);
        uint8_t fresh = 0;
        nvs_get_u8(nvs, NVS_KEY_FRESH, &fresh);
        if (s_ssid[0] && s_broker[0]) {
            has_creds = true;
            if (fresh) {
                is_fresh = true;
                nvs_set_u8(nvs, NVS_KEY_FRESH, 0);   /* consume the flag */
                nvs_commit(nvs);
            }
        }
        nvs_close(nvs);
    }

    /* ── CASE A: credentials just provisioned last boot ──────────────────
     * Skip BLE entirely — go straight to WiFi. */
    if (has_creds && is_fresh) {
        ESP_LOGI(TAG, "Fresh credentials — skipping BLE, starting WiFi: ssid=%s broker=%s",
                 s_ssid, s_broker);
        return ESP_OK;
    }

    /* ── CASE B / C: start BLE ───────────────────────────────────────────
     * Always advertise so re-provisioning is available. */
    ESP_LOGI(TAG, "Starting BLE — %s",
             has_creds ? "re-provision window open" : "waiting for first credentials");
    start_nimble();

    if (!has_creds) {
        /* ── CASE C: first boot — block until credentials arrive ─────── */
        xEventGroupWaitBits(s_events, CREDS_OK_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1500));  /* let notify packet transmit before radio dies */
        esp_restart();                    /* → CASE A on next boot */
        return ESP_OK;                    /* unreachable */
    }

    /* ── CASE B: has credentials — open re-provision window ─────────── */
    ESP_LOGI(TAG, "Re-provision window: %d s", BLE_PROV_WINDOW_MS / 1000);
    EventBits_t bits = xEventGroupWaitBits(
        s_events, CREDS_OK_BIT, pdFALSE, pdTRUE,
        pdMS_TO_TICKS(BLE_PROV_WINDOW_MS));

    if (bits & CREDS_OK_BIT) {
        vTaskDelay(pdMS_TO_TICKS(1500));  /* let notify packet transmit before radio dies */
        esp_restart();                    /* → CASE A on next boot */
    }

    /* Window expired with no new credentials — stop BLE, use stored creds */
    ESP_LOGI(TAG, "Window expired — using stored credentials: ssid=%s broker=%s",
             s_ssid, s_broker);
    stop_nimble();
    return ESP_OK;
}

const char *provisioning_get_ssid(void)     { return s_ssid; }
const char *provisioning_get_password(void) { return s_password; }
const char *provisioning_get_broker(void)   { return s_broker; }

void provisioning_reset(void)
{
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_all(nvs);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
    ESP_LOGI(TAG, "Credentials wiped — rebooting into provisioning mode");
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
}
