#include "sensors.h"
#include "config.h"
#include <string.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

/* ── IR debounce ──────────────────────────────────────────────────────── */
static int ir_state[3] = {0};
static int ir_prev[3]  = {0};
static int ir_count[3] = {0};

/* ── Ultrasonic — interrupt-driven, one trigger per call ─────────────── */
typedef struct {
    int              trig;
    int              echo;
    volatile int64_t rise_us;   /* rising-edge timestamp, written by ISR */
    volatile int64_t trig_us;   /* when trigger was last fired */
    volatile int     dist_cm;   /* result, written by ISR on falling edge */
    volatile bool    measuring; /* true from trigger until echo complete */
} USSensor;

static USSensor us[3];
static int      us_trig_idx = 0;

static void IRAM_ATTR echo_isr(void *arg)
{
    USSensor *s = (USSensor *)arg;
    int64_t   t = esp_timer_get_time();
    if (gpio_get_level(s->echo)) {
        s->rise_us = t;                          /* rising edge: start timing */
    } else if (s->measuring) {
        int64_t pulse = t - s->rise_us;
        s->dist_cm   = (int)(pulse / 58);        /* falling edge: compute cm  */
        s->measuring = false;
    }
}

/* ── Battery ADC ─────────────────────────────────────────────────────── */
static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t         s_cali = NULL;

static const char *TAG_S = "SENSORS";

/* ─────────────────────────────────────────────────────────────────────── */

void sensors_init(void)
{
    /* IR — inputs, no pull */
    gpio_config_t ir_cfg = {
        .pin_bit_mask = (1ULL<<PIN_IR_LEFT)|(1ULL<<PIN_IR_CENTER)|(1ULL<<PIN_IR_RIGHT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&ir_cfg);

    /* Ultrasonic TRIG — outputs */
    gpio_config_t trig_cfg = {
        .pin_bit_mask = (1ULL<<PIN_TRIG_LEFT)|(1ULL<<PIN_TRIG_CENTER)|(1ULL<<PIN_TRIG_RIGHT),
        .mode         = GPIO_MODE_OUTPUT,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&trig_cfg);

    /* Ultrasonic ECHO — inputs with any-edge interrupt for ISR capture.
     * GPIO35/36 are input-only: no pull-up/down available. */
    gpio_config_t echo_cfg = {
        .pin_bit_mask = (1ULL<<PIN_ECHO_LEFT)|(1ULL<<PIN_ECHO_CENTER)|(1ULL<<PIN_ECHO_RIGHT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&echo_cfg);

    us[0] = (USSensor){ .trig=PIN_TRIG_LEFT,   .echo=PIN_ECHO_LEFT,   .dist_cm=400 };
    us[1] = (USSensor){ .trig=PIN_TRIG_CENTER, .echo=PIN_ECHO_CENTER, .dist_cm=400 };
    us[2] = (USSensor){ .trig=PIN_TRIG_RIGHT,  .echo=PIN_ECHO_RIGHT,  .dist_cm=400 };

    esp_err_t isr_ret = gpio_install_isr_service(0);
    if (isr_ret != ESP_OK && isr_ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG_S, "GPIO ISR service failed (%d)", isr_ret);
    }
    gpio_isr_handler_add(PIN_ECHO_LEFT,   echo_isr, &us[0]);
    gpio_isr_handler_add(PIN_ECHO_CENTER, echo_isr, &us[1]);
    gpio_isr_handler_add(PIN_ECHO_RIGHT,  echo_isr, &us[2]);

    /* ADC — oneshot, 12-bit, 0-3.3 V */
    adc_oneshot_unit_init_cfg_t adc_cfg = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&adc_cfg, &s_adc);

    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(s_adc, ADC_CHANNEL_6, &ch_cfg); /* GPIO34 */

    /* Calibration — line-fitting is the correct scheme for ESP32.
     * This corrects for the actual ADC full-scale (~3100 mV at DB_12,
     * not 3300 mV) and for per-chip eFuse reference voltage data. */
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG_S, "ADC cali unavailable (err %d) — battery readings will be approximate", ret);
        s_cali = NULL;
    }
}

/* ── IR — debounce: accept reading after IR_DEBOUNCE_READS consistent samples */
IRData sensors_read_ir(void)
{
    const int pins[3] = { PIN_IR_LEFT, PIN_IR_CENTER, PIN_IR_RIGHT };

    for (int i = 0; i < 3; i++) {
        int v = gpio_get_level(pins[i]);
        if (v == ir_prev[i]) {
            if (++ir_count[i] >= IR_DEBOUNCE_READS) {
                ir_state[i] = v;
                ir_count[i] = 0;
            }
        } else {
            ir_prev[i]  = v;
            ir_count[i] = 1;
        }
    }

    return (IRData){ ir_state[0], ir_state[1], ir_state[2] };
}

/* ── Ultrasonic — fire one trigger per call, ISR records the echo time.
 *
 * The old polling design missed echoes from objects <~170 cm because the
 * echo pulse (e.g. 1.8 ms for 30 cm) was already gone by the time the
 * 10 ms loop checked.  The ISR captures the exact rising/falling edges
 * regardless of loop timing.
 */
UltraData sensors_read_ultrasonic(void)
{
    int64_t now = esp_timer_get_time();

    /* Expire any measurement that has been waiting longer than the timeout */
    for (int i = 0; i < 3; i++) {
        if (us[i].measuring && (now - us[i].trig_us) > ULTRA_TIMEOUT_US) {
            us[i].dist_cm  = 400;
            us[i].measuring = false;
        }
    }

    /* Fire the next sensor's trigger only when it is idle */
    USSensor *s = &us[us_trig_idx];
    if (!s->measuring) {
        s->trig_us   = esp_timer_get_time();
        s->measuring = true;
        gpio_set_level(s->trig, 1);
        esp_rom_delay_us(10);
        gpio_set_level(s->trig, 0);
        us_trig_idx = (us_trig_idx + 1) % 3;
    }

    return (UltraData){ us[0].dist_cm, us[1].dist_cm, us[2].dist_cm };
}

/* ── Battery — ADC + voltage divider scaling */
float sensors_read_battery(void)
{
    /* Average 8 samples to reduce RF/switching noise */
    int sum = 0;
    for (int i = 0; i < 8; i++) {
        int r = 0;
        adc_oneshot_read(s_adc, ADC_CHANNEL_6, &r);
        sum += r;
    }
    int raw = sum / 8;

    int mv = 0;
    if (s_cali) {
        adc_cali_raw_to_voltage(s_cali, raw, &mv);
    } else {
        /* Fallback: use 3100 mV (actual DB_12 full-scale), not 3300 mV */
        mv = (int)((float)raw / BATTERY_ADC_MAX * 3100.0f);
    }

    float v_pin = mv / 1000.0f;
    return v_pin * (BATTERY_R1 + BATTERY_R2) / BATTERY_R2;
}
