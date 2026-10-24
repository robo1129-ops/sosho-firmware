/*
 * SO-SHO scroll inertia.
 *
 * Layer 4 is the momentary trackball scroll layer.  While that layer is
 * active, remember the recent scroll velocity.  After the sensor stops
 * reporting movement, emit a short decaying tail of wheel events.  This is
 * deliberately implemented outside the input-processor chain so the normal
 * pointer path and the existing dominant-axis lock are left unchanged.
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

#if IS_ENABLED(CONFIG_ZMK_POINTING)

/* Fixed-point values use eight fractional bits. */
#define SOSHO_SCROLL_Q8_SHIFT 8
#define SOSHO_SCROLL_Q8_ONE (1 << SOSHO_SCROLL_Q8_SHIFT)

/* Start the inertial tail only after the sensor has been quiet briefly. */
#define SOSHO_SCROLL_INERTIA_IDLE_MS 45
#define SOSHO_SCROLL_INERTIA_TICK_MS 16

/* 7/8 per tick gives a smooth, finite tail of roughly 0.2-0.4 seconds. */
#define SOSHO_SCROLL_INERTIA_DECAY_NUM 7
#define SOSHO_SCROLL_INERTIA_DECAY_DEN 8
#define SOSHO_SCROLL_INERTIA_STOP_Q8 20

/* The sensor path already applies the 7/10 build-time scroll scale.  Keep the
 * same scale here so the inertial tail starts at the same speed as live
 * motion. */
#define SOSHO_SCROLL_SCALE_NUM 7
#define SOSHO_SCROLL_SCALE_DEN 10

static struct k_spinlock inertia_lock;
static int32_t velocity_x_q8;
static int32_t velocity_y_q8;
static int32_t remainder_x_q8;
static int32_t remainder_y_q8;
static int64_t last_motion_ms;
static bool motion_seen;

static void inertia_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(inertia_work, inertia_work_handler);

static bool is_sosho_trackball_device(const struct device *dev) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(trackball), okay)
    if (dev == DEVICE_DT_GET(DT_NODELABEL(trackball))) {
        return true;
    }
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(trackball_split), okay)
    if (dev == DEVICE_DT_GET(DT_NODELABEL(trackball_split))) {
        return true;
    }
#endif

    return false;
}

static int32_t scale_to_q8(int32_t value) {
    return (value * SOSHO_SCROLL_SCALE_NUM * SOSHO_SCROLL_Q8_ONE) /
           SOSHO_SCROLL_SCALE_DEN;
}

static int32_t q8_report_value(int32_t *remainder, int32_t velocity_q8) {
    int32_t accumulated = *remainder + velocity_q8;
    int32_t report = accumulated / SOSHO_SCROLL_Q8_ONE;

    *remainder = accumulated - (report * SOSHO_SCROLL_Q8_ONE);
    return report;
}

static void stop_inertia(void) {
    velocity_x_q8 = 0;
    velocity_y_q8 = 0;
    remainder_x_q8 = 0;
    remainder_y_q8 = 0;
    motion_seen = false;
}

static void inertia_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    int32_t report_x;
    int32_t report_y;
    int32_t current_x_q8;
    int32_t current_y_q8;
    k_spinlock_key_t key = k_spin_lock(&inertia_lock);

    if (!motion_seen) {
        k_spin_unlock(&inertia_lock, key);
        return;
    }

    int64_t quiet_for = k_uptime_get() - last_motion_ms;
    if (quiet_for < SOSHO_SCROLL_INERTIA_IDLE_MS) {
        k_spin_unlock(&inertia_lock, key);
        k_work_reschedule(&inertia_work,
                          K_MSEC(SOSHO_SCROLL_INERTIA_IDLE_MS - quiet_for));
        return;
    }

    current_x_q8 = velocity_x_q8;
    current_y_q8 = velocity_y_q8;
    if ((current_x_q8 < SOSHO_SCROLL_INERTIA_STOP_Q8 &&
         current_x_q8 > -SOSHO_SCROLL_INERTIA_STOP_Q8) &&
        (current_y_q8 < SOSHO_SCROLL_INERTIA_STOP_Q8 &&
         current_y_q8 > -SOSHO_SCROLL_INERTIA_STOP_Q8)) {
        stop_inertia();
        k_spin_unlock(&inertia_lock, key);
        return;
    }

    report_x = q8_report_value(&remainder_x_q8, current_x_q8);
    report_y = q8_report_value(&remainder_y_q8, current_y_q8);
    velocity_x_q8 = (velocity_x_q8 * SOSHO_SCROLL_INERTIA_DECAY_NUM) /
                    SOSHO_SCROLL_INERTIA_DECAY_DEN;
    velocity_y_q8 = (velocity_y_q8 * SOSHO_SCROLL_INERTIA_DECAY_NUM) /
                    SOSHO_SCROLL_INERTIA_DECAY_DEN;
    k_spin_unlock(&inertia_lock, key);

    if (report_x != 0 || report_y != 0) {
        /* x is horizontal wheel and y is vertical wheel, matching
         * zmk_hid_mouse_scroll_set() and zip_xy_to_scroll_mapper. */
        zmk_hid_mouse_scroll_set((int16_t)report_x, (int16_t)report_y);
        zmk_endpoint_send_mouse_report();
        zmk_hid_mouse_scroll_set(0, 0);
    }

    k_work_reschedule(&inertia_work, K_MSEC(SOSHO_SCROLL_INERTIA_TICK_MS));
}

static void sosho_scroll_inertia_input_cb(struct input_event *event,
                                          void *user_data) {
    ARG_UNUSED(user_data);

    if (!is_sosho_trackball_device(event->dev) ||
        event->type != INPUT_EV_REL ||
        (event->code != INPUT_REL_X && event->code != INPUT_REL_Y) ||
        event->value == 0 || !zmk_keymap_layer_active(4)) {
        return;
    }

    /* The live scroll chain inverts X but deliberately leaves Y in its new
     * orientation.  Mirror that sign convention for the inertial tail. */
    int32_t sample = event->code == INPUT_REL_X ? -event->value : event->value;
    int32_t sample_q8 = scale_to_q8(sample);
    k_spinlock_key_t key = k_spin_lock(&inertia_lock);

    /* A short moving average avoids an abrupt jump when the finger/ball is
     * released and gives the tail a stable initial velocity. */
    if (event->code == INPUT_REL_X) {
        velocity_x_q8 = ((velocity_x_q8 * 3) + sample_q8) / 4;
    } else {
        velocity_y_q8 = ((velocity_y_q8 * 3) + sample_q8) / 4;
    }

    last_motion_ms = k_uptime_get();
    motion_seen = true;
    k_spin_unlock(&inertia_lock, key);

    k_work_reschedule(&inertia_work, K_MSEC(SOSHO_SCROLL_INERTIA_IDLE_MS));
}

INPUT_CALLBACK_DEFINE(NULL, sosho_scroll_inertia_input_cb, NULL);

#endif /* IS_ENABLED(CONFIG_ZMK_POINTING) */
