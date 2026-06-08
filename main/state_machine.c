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
    gpio_set_level(PIN_IN1, fwd_a ? 0 : 1);
    gpio_set_level(PIN_IN2, fwd_a ? 1 : 0);
    gpio_set_level(PIN_IN3, fwd_b ? 1 : 0);
    gpio_set_level(PIN_IN4, fwd_b ? 0 : 1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm_a);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, pwm_b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

/* ══════════════════════════════════════════════════════════════
 *  STATE MACHINE
 * ══════════════════════════════════════════════════════════════ */

typedef enum { MV_STOP, MV_FORWARD, MV_BACK, MV_LEFT, MV_RIGHT } ManualMove;
typedef enum { AVOID_NONE, AVOID_LEFT, AVOID_RIGHT } AvoidDir;

static RobotMode   s_mode             = MODE_MANUAL;
static RobotState  s_state            = STATE_MANUAL;
static ManualMove  s_move             = MV_STOP;
static int         s_speed            = SPEED_BASE;

static int64_t     s_avoid_clear_us   = 0;
static int64_t     s_post_avoid_us    = 0;
static int         s_fwd_speed        = SPEED_POST_CORR;
static AvoidDir    s_avoid_dir        = AVOID_NONE;

/* Manual driving: all at app-controlled s_speed. */
static void m_stop(void)       { drive(0,       true,  0,       true); }
static void m_forward(void)    { drive(s_speed, true,  s_speed, true); }
static void m_reverse(void)    { drive(s_speed, false, s_speed, false); }
static void m_spin_left(void)  { drive(s_speed, false, s_speed, true); }
static void m_spin_right(void) { drive(s_speed, true,  s_speed, false); }

/* Line corrections: always SPEED_CORRECTION (firm but not violent). */
static void do_pivot_left(void)  { drive(0,               true,  SPEED_CORRECTION, true); }
static void do_pivot_right(void) { drive(SPEED_CORRECTION, true,  0,               true); }
static void do_spin_left(void)   { drive(SPEED_CORRECTION, false, SPEED_CORRECTION, true); }
static void do_spin_right(void)  { drive(SPEED_CORRECTION, true,  SPEED_CORRECTION, false); }

void state_machine_init(void)
{
    motors_hw_init();
    ESP_LOGI(TAG, "State machine ready");
}

/* ── Manual mode: all directions use the same s_speed ─────────────────── */
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

/* ── IR follow: center-gated forward + gradual ramp ─────────────────────
 *
 * Center sensor is the gate for forward motion:
 *   center ON  → robot is over the line → allowed to drive forward (ramp).
 *   center OFF → robot is not over the line → correction only, never forward.
 *
 * This prevents driving into air when picked up, and keeps corrections
 * gentle (SPEED_CORRECTION = 110, never max RPM).
 *
 * All sensors off → enter RECOVERING (brief creep then stop). */
static void run_ir_follow(IRData ir)
{
    int l = (ir.left   == IR_LINE_VALUE);
    int c = (ir.center == IR_LINE_VALUE);
    int r = (ir.right  == IR_LINE_VALUE);

    /* All off: car lifted or fully off track → stop immediately */
    if (!l && !c && !r) {
        s_fwd_speed = SPEED_POST_CORR;
        s_state     = STATE_LIFTED;
        m_stop();
        return;
    }

    /* Center NOT on line: correct without driving forward */
    if (!c) {
        s_fwd_speed = SPEED_POST_CORR;
        if      ( l && !r) do_spin_right();  /* line on left  → spin right to center */
        else if (!l &&  r) do_spin_left();   /* line on right → spin left  to center */
        else               drive(SPEED_POST_CORR, true, SPEED_POST_CORR, true); /* straddle */
        return;
    }

    /* Center on line: check sides for gentle pivot, else roll forward */
    if (l && !r) {
        /* Center + left: slight right drift → gentle pivot right */
        s_fwd_speed = SPEED_POST_CORR;
        do_pivot_right();
    } else if (!l && r) {
        /* Center + right: slight left drift → gentle pivot left */
        s_fwd_speed = SPEED_POST_CORR;
        do_pivot_left();
    } else {
        /* Center only, all three, or straddle: confirmed on line → ramp up */
        if (s_fwd_speed < s_speed) {
            s_fwd_speed += SPEED_RAMP_STEP;
            if (s_fwd_speed > s_speed) s_fwd_speed = s_speed;
        }
        drive(s_fwd_speed, true, s_fwd_speed, true);
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
        s_post_avoid_us  = 0;
        s_avoid_dir      = AVOID_NONE;
        ESP_LOGD(TAG, "Obstacle %d cm → AVOIDING", min_d);
        return;
    }

    /* After obstacle avoidance, creep forward briefly before handing off to
     * the IR table — avoids the erratic full-speed lurch back onto the line. */
    if (s_post_avoid_us > 0) {
        if (esp_timer_get_time() - s_post_avoid_us < (int64_t)POST_AVOID_FWD_MS * 1000) {
            m_forward();
            return;
        }
        s_fwd_speed     = SPEED_POST_CORR;
        s_post_avoid_us = 0;
    }

    run_ir_follow(ir);
}

/* ── AVOIDING: slow, cautious navigation around obstacle ─────────────────
 *
 * Phase 1 — STEER: front sensor still blocked → pivot at SPEED_AVOID toward
 *   the side with more space.  Direction is locked in on first entry.
 *
 * Phase 2 — CREEP: front clear → drive forward at SPEED_AVOID.  All three
 *   sensors are watched continuously:
 *     • If the side we're heading toward gets blocked: flip direction.
 *     • If the opposite side gets too close: stop briefly to reassess.
 *   After AVOID_MIN_MS of fully-clear sensors → exit to post-avoid creep. */
static void run_avoiding(UltraData ultra)
{
    /* Lock in direction once: go where there is more space. */
    if (s_avoid_dir == AVOID_NONE) {
        s_avoid_dir      = (ultra.left >= ultra.right) ? AVOID_LEFT : AVOID_RIGHT;
        s_avoid_clear_us = 0;
        ESP_LOGD(TAG, "Avoid → %s", s_avoid_dir == AVOID_LEFT ? "LEFT" : "RIGHT");
    }

    bool front_blocked = (ultra.center < OBSTACLE_WARN_CM);
    bool left_close    = (ultra.left   < OBSTACLE_STOP_CM);
    bool right_close   = (ultra.right  < OBSTACLE_STOP_CM);

    /* ── Phase 1: front still blocked → keep pivoting ─────────────────── */
    if (front_blocked) {
        s_avoid_clear_us = 0;
        if (s_avoid_dir == AVOID_RIGHT)
            drive(SPEED_AVOID, true, 0, true);   /* pivot right */
        else
            drive(0, true, SPEED_AVOID, true);   /* pivot left  */
        return;
    }

    /* ── Phase 2: front clear, watch sides ────────────────────────────── */

    /* If our chosen path is now blocked: flip direction, reset timer. */
    if ((s_avoid_dir == AVOID_RIGHT && right_close) ||
        (s_avoid_dir == AVOID_LEFT  && left_close)) {
        s_avoid_dir      = (s_avoid_dir == AVOID_RIGHT) ? AVOID_LEFT : AVOID_RIGHT;
        s_avoid_clear_us = 0;
        m_stop();
        ESP_LOGD(TAG, "Side blocked, flip direction");
        return;
    }

    /* If opposite side is close but not in our path: pause briefly. */
    if (left_close || right_close) {
        s_avoid_clear_us = 0;
        m_stop();
        return;
    }

    /* All sensors clear: creep forward and count clear time. */
    int64_t now = esp_timer_get_time();
    if (s_avoid_clear_us == 0) s_avoid_clear_us = now;

    if (now - s_avoid_clear_us > (int64_t)AVOID_MIN_MS * 1000) {
        s_avoid_clear_us = 0;
        s_avoid_dir      = AVOID_NONE;
        s_post_avoid_us  = now;
        s_fwd_speed      = SPEED_POST_CORR;
        s_state          = STATE_FOLLOWING;
        ESP_LOGD(TAG, "Obstacle clear → FOLLOWING");
        return;
    }

    drive(SPEED_AVOID, true, SPEED_AVOID, true);
}

/* ── LIFTED: all sensors off — car is picked up or off track ─────────────
 * Motors stay stopped.  The moment any sensor sees the line the car is back
 * on the ground and following resumes from rest (SPEED_POST_CORR ramp). */
static void run_lifted(IRData ir)
{
    if (ir.left   == IR_LINE_VALUE ||
        ir.center == IR_LINE_VALUE ||
        ir.right  == IR_LINE_VALUE) {
        s_fwd_speed = SPEED_POST_CORR;
        s_state     = STATE_FOLLOWING;
        ESP_LOGD(TAG, "Sensors live → FOLLOWING");
        return;
    }
    m_stop();
}

/* ── Main update — called every MAIN_LOOP_MS from app_main ───────────── */
void state_machine_update(IRData ir, UltraData ultra)
{
    if (s_mode == MODE_MANUAL) { run_manual(); return; }

    switch (s_state) {
        case STATE_FOLLOWING:  run_following(ir, ultra); break;
        case STATE_AVOIDING:   run_avoiding(ultra);      break;
        case STATE_LIFTED:     run_lifted(ir);           break;
        default:               break;
    }
}

/* ── MQTT-driven setters (called from mqtt task — writes are word-atomic) */
void state_machine_set_mode(RobotMode mode)
{
    s_mode      = mode;
    s_state     = (mode == MODE_LINE_FOLLOW) ? STATE_LIFTED : STATE_MANUAL;
    s_fwd_speed = SPEED_POST_CORR;
    s_avoid_dir = AVOID_NONE;
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
    ESP_LOGI(TAG, "Move → %s", cmd);
}

void state_machine_set_speed(int speed)
{
    s_speed = speed < 0 ? 0 : speed > 255 ? 255 : speed;
    ESP_LOGI(TAG, "Speed → %d", s_speed);
}

RobotMode  state_machine_get_mode(void)  { return s_mode;  }
RobotState state_machine_get_state(void) { return s_state; }
