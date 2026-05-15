#include "sensors.h"
#include "config.h"
#include <string.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

/* ── IR debounce ──────────────────────────────────────────────────────── */
static int ir_state[3] = {0};
static int ir_prev[3]  = {0};
static int ir_count[3] = {0};

/* ── Ultrasonic round-robin state machine ─────────────────────────────── */
typedef enum { US_IDLE, US_WAIT_HIGH, US_WAIT_LOW } USState;

typedef struct {
    int     trig;
    int     echo;
    USState state;
    int64_t start_us;
    int     dist_cm;
} USSensor;

static USSensor us[3];
static int      us_idx = 0;

/* ── Battery ADC ─────────────────────────────────────────────────────── */
static adc_oneshot_unit_handle_t s_adc;

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

    /* Ultrasonic ECHO — inputs (GPIO35 is input-only, no pull available) */
    gpio_config_t echo_cfg = {
        .pin_bit_mask = (1ULL<<PIN_ECHO_LEFT)|(1ULL<<PIN_ECHO_CENTER)|(1ULL<<PIN_ECHO_RIGHT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&echo_cfg);

    us[0] = (USSensor){ PIN_TRIG_LEFT,   PIN_ECHO_LEFT,   US_IDLE, 0, 400 };
    us[1] = (USSensor){ PIN_TRIG_CENTER, PIN_ECHO_CENTER, US_IDLE, 0, 400 };
    us[2] = (USSensor){ PIN_TRIG_RIGHT,  PIN_ECHO_RIGHT,  US_IDLE, 0, 400 };

    /* ADC — oneshot, 12-bit, 0-3.3 V */
    adc_oneshot_unit_init_cfg_t adc_cfg = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&adc_cfg, &s_adc);

    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(s_adc, ADC_CHANNEL_6, &ch_cfg); /* GPIO34 */
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

/* ── Ultrasonic — one sensor advanced per call, returns cached distances */
UltraData sensors_read_ultrasonic(void)
{
    USSensor *s   = &us[us_idx];
    int64_t   now = esp_timer_get_time(); /* µs */

    switch (s->state) {
        case US_IDLE:
            gpio_set_level(s->trig, 1);
            esp_rom_delay_us(10);          /* 10 µs trigger pulse */
            gpio_set_level(s->trig, 0);
            s->start_us = now;
            s->state    = US_WAIT_HIGH;
            break;

        case US_WAIT_HIGH:
            if (gpio_get_level(s->echo)) {
                s->start_us = now;
                s->state    = US_WAIT_LOW;
            } else if (now - s->start_us > ULTRA_TIMEOUT_US) {
                s->dist_cm = 400;
                s->state   = US_IDLE;
                us_idx     = (us_idx + 1) % 3;
            }
            break;

        case US_WAIT_LOW:
            if (!gpio_get_level(s->echo)) {
                s->dist_cm = (int)((now - s->start_us) / 58);
                s->state   = US_IDLE;
                us_idx     = (us_idx + 1) % 3;
            } else if (now - s->start_us > ULTRA_TIMEOUT_US) {
                s->dist_cm = 400;
                s->state   = US_IDLE;
                us_idx     = (us_idx + 1) % 3;
            }
            break;
    }

    return (UltraData){ us[0].dist_cm, us[1].dist_cm, us[2].dist_cm };
}

/* ── Battery — ADC + voltage divider scaling */
float sensors_read_battery(void)
{
    int raw = 0;
    adc_oneshot_read(s_adc, ADC_CHANNEL_6, &raw);
    float v_pin = ((float)raw / BATTERY_ADC_MAX) * BATTERY_VREF;
    return v_pin * (BATTERY_R1 + BATTERY_R2) / BATTERY_R2;
}
