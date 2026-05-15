/*
 * state_machine.c — robot autonomy logic + motor control.
 *
 * Motor control lives here (not a separate file) because nothing outside
 * this module drives the motors. Keeping them together removes a layer.
 */
#include "state_machine.h"
#include "config.h"
#include <string.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "SM";

/* ══════════════════════════════════════════════════════════════
 *  MOTOR CONTROL  (private to this module)
 * ══════════════════════════════════════════════════════════════ */

static void motors_hw_init(void)
{
    gpio_config_t dir = {
        .pin_bit_mask = (1ULL<<PIN_IN1)|(1ULL<<PIN_IN2)|(1ULL<<PIN_IN3)|(1ULL<<PIN_IN4),
        .mode         = GPIO_MODE_OUTPUT,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&dir);

    ledc_timer_config_t tmr = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz         = PWM_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&tmr);

    ledc_channel_config_t ch_a = {
        .gpio_num   = PIN_ENA,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty = 0, .hpoint = 0,
    };
    ledc_channel_config(&ch_a);

    ledc_channel_config_t ch_b = {
        .gpio_num   = PIN_ENB,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_0,
        .duty = 0, .hpoint = 0,
    };
    ledc_channel_config(&ch_b);
}

/* pwm_a/b: 0-255 duty. fwd_a/b: true = forward, false = reverse. */
static void drive(int pwm_a, bool fwd_a, int pwm_b, bool fwd_b)
{
    gpio_set_level(PIN_IN1, fwd_a ? 1 : 0);
    gpio_set_level(PIN_IN2, fwd_a ? 0 : 1);
    gpio_set_level(PIN_IN3, fwd_b ? 1 : 0);
    gpio_set_level(PIN_IN4, fwd_b ? 0 : 1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm_a);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, pwm_b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

static void m_stop(void)         { drive(0,           true,  0,           true); }
static void m_forward(void)      { drive(SPEED_BASE,  true,  SPEED_BASE,  true); }
static void m_reverse(void)      { drive(SPEED_REVERSE,false,SPEED_REVERSE,false);}
static void m_spin_left(void)    { drive(SPEED_TURN,  false, SPEED_TURN,  true); }
static void m_spin_right(void)   { drive(SPEED_TURN,  true,  SPEED_TURN,  false);}
static void m_curve_left(void)   { drive(SPEED_SLOW,  true,  SPEED_BASE,  true); }
static void m_curve_right(void)  { drive(SPEED_BASE,  true,  SPEED_SLOW,  true); }
static void m_hard_left(void)    { drive(SPEED_HARD_TURN,true,SPEED_BASE,true); }
static void m_hard_right(void)   { drive(SPEED_BASE,true,SPEED_HARD_TURN,true); }

/* ══════════════════════════════════════════════════════════════
 *  STATE MACHINE
 * ══════════════════════════════════════════════════════════════ */

typedef enum { MV_STOP, MV_FORWARD, MV_BACK, MV_LEFT, MV_RIGHT } ManualMove;

static RobotMode   s_mode             = MODE_MANUAL;
static RobotState  s_state            = STATE_MANUAL;
static ManualMove  s_move             = MV_STOP;
static int64_t     s_avoid_clear_us   = 0;
static int64_t     s_recover_start_us = 0;
static bool        s_last_ir_left     = false;

void state_machine_init(void)
{
    motors_hw_init();
    ESP_LOGI(TAG, "State machine ready");
}

/* ── Manual mode: just execute the last MQTT move command ─────────────── */
static void run_manual(void)
{
    switch (s_move) {
        case MV_FORWARD: m_forward();    break;
        case MV_BACK:    m_reverse();    break;
        case MV_LEFT:    m_spin_left();  break;
        case MV_RIGHT:   m_spin_right(); break;
        default:         m_stop();       break;
    }
}

/* ── IR 8-case truth table (line = 0, no-line = 1 on these sensors) ───── */
static void run_ir_follow(IRData ir)
{
    int l = (ir.left   == IR_LINE_VALUE);
    int c = (ir.center == IR_LINE_VALUE);
    int r = (ir.right  == IR_LINE_VALUE);

    if      ( l &&  c &&  r) m_forward();
    else if (!l &&  c && !r) m_forward();
    else if (!l &&  c &&  r) m_curve_left();
    else if ( l &&  c && !r) m_curve_right();
    else if (!l && !c &&  r) m_hard_left();
    else if ( l && !c && !r) m_hard_right();
    else if ( l && !c &&  r) m_forward();    /* straddle — keep going */
    else {                                    /* 0,0,0 — line lost */
        s_state            = STATE_RECOVERING;
        s_recover_start_us = esp_timer_get_time();
        ESP_LOGD(TAG, "Line lost → RECOVERING");
    }
}

/* ── FOLLOWING: check for obstacle, then follow line ─────────────────── */
static void run_following(IRData ir, UltraData ultra)
{
    int min_d = ultra.left;
    if (ultra.center < min_d) min_d = ultra.center;
    if (ultra.right  < min_d) min_d = ultra.right;

    if (min_d < OBSTACLE_WARN_CM) {
        s_state          = STATE_AVOIDING;
        s_avoid_clear_us = 0;
        ESP_LOGD(TAG, "Obstacle %d cm → AVOIDING", min_d);
        return;
    }
    run_ir_follow(ir);
}

/* ── AVOIDING: curve around obstacle until clear for AVOID_MIN_MS ─────── */
static void run_avoiding(UltraData ultra)
{
    int min_d = ultra.left;
    if (ultra.center < min_d) min_d = ultra.center;
    if (ultra.right  < min_d) min_d = ultra.right;

    if (min_d > OBSTACLE_STOP_CM) {
        int64_t now = esp_timer_get_time();
        if (s_avoid_clear_us == 0) {
            s_avoid_clear_us = now;
        } else if (now - s_avoid_clear_us > (int64_t)AVOID_MIN_MS * 1000) {
            s_avoid_clear_us = 0;
            s_state          = STATE_FOLLOWING;
            ESP_LOGD(TAG, "Obstacle clear → FOLLOWING");
            return;
        }
    } else {
        s_avoid_clear_us = 0;
    }

    if (ultra.left < ultra.right) m_curve_right();
    else                          m_curve_left();
}

/* ── RECOVERING: spin toward last-known direction, timeout stops robot ── */
static void run_recovering(IRData ir)
{
    /* If line found again, go back to following immediately */
    if (ir.left   == IR_LINE_VALUE ||
        ir.center == IR_LINE_VALUE ||
        ir.right  == IR_LINE_VALUE) {
        s_state = STATE_FOLLOWING;
        ESP_LOGD(TAG, "Line re-acquired → FOLLOWING");
        return;
    }

    int64_t elapsed = esp_timer_get_time() - s_recover_start_us;

    if (elapsed < (int64_t)RECOVERY_SWEEP_MS * 1000) {
        if (s_last_ir_left) m_spin_left();
        else                m_spin_right();
    } else if (elapsed < (int64_t)RECOVERY_SPIN_MS * 1000) {
        m_spin_right(); /* full-circle scan */
    } else {
        m_stop(); /* give up */
        ESP_LOGW(TAG, "Recovery timeout — stopped");
    }
}

/* ── Main update — called every MAIN_LOOP_MS from app_main ───────────── */
void state_machine_update(IRData ir, UltraData ultra)
{
    /* Track which side the line was last seen on for recovery direction */
    if (ir.left  == IR_LINE_VALUE) s_last_ir_left = true;
    if (ir.right == IR_LINE_VALUE) s_last_ir_left = false;

    if (s_mode == MODE_MANUAL) { run_manual(); return; }

    switch (s_state) {
        case STATE_FOLLOWING:  run_following(ir, ultra); break;
        case STATE_AVOIDING:   run_avoiding(ultra);      break;
        case STATE_RECOVERING: run_recovering(ir);       break;
        default:               break;
    }
}

/* ── MQTT-driven setters (called from mqtt task — writes are word-atomic) */
void state_machine_set_mode(RobotMode mode)
{
    s_mode  = mode;
    s_state = (mode == MODE_LINE_FOLLOW) ? STATE_FOLLOWING : STATE_MANUAL;
    m_stop();
    ESP_LOGI(TAG, "Mode → %s", mode == MODE_LINE_FOLLOW ? "LINE_FOLLOW" : "MANUAL");
}

void state_machine_set_move(const char *cmd)
{
    if      (strcmp(cmd, "forward") == 0) s_move = MV_FORWARD;
    else if (strcmp(cmd, "back")    == 0) s_move = MV_BACK;
    else if (strcmp(cmd, "left")    == 0) s_move = MV_LEFT;
    else if (strcmp(cmd, "right")   == 0) s_move = MV_RIGHT;
    else                                  s_move = MV_STOP;
}

RobotMode  state_machine_get_mode(void)  { return s_mode;  }
RobotState state_machine_get_state(void) { return s_state; }
