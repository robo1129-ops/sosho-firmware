/*
 * SO-SHO scroll axis lock.
 *
 * The ZMK input listener receives relative X/Y values as separate events.
 * When a sensor report contains both axes, hold the first event until the
 * second axis arrives, then keep only the dominant axis. This prevents a
 * diagonal gesture from becoming simultaneous vertical and horizontal
 * scrolling while preserving ordinary single-axis motion.
 */

#define DT_DRV_COMPAT sosho_input_processor_axis_lock

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/sys/util.h>

#include <drivers/input_processor.h>

#define SOSHO_AXIS_LOCK_STATE_COUNT 8

struct sosho_axis_lock_state {
    uint16_t pending_code;
    int32_t pending_value;
};

static struct sosho_axis_lock_state states[SOSHO_AXIS_LOCK_STATE_COUNT];

static int64_t magnitude(int32_t value) {
    return value < 0 ? -(int64_t)value : (int64_t)value;
}

static struct sosho_axis_lock_state *state_for(const struct zmk_input_processor_state *state) {
    uint8_t index = state->input_device_index;

    if (index >= SOSHO_AXIS_LOCK_STATE_COUNT) {
        index = 0;
    }

    return &states[index];
}

static int axis_lock_handle_event(const struct device *dev, struct input_event *event,
                                  uint32_t param1, uint32_t param2,
                                  struct zmk_input_processor_state *processor_state) {
    ARG_UNUSED(dev);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    if (event->type != INPUT_EV_REL ||
        (event->code != INPUT_REL_X && event->code != INPUT_REL_Y)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    struct sosho_axis_lock_state *lock = state_for(processor_state);

    /* A complete one-axis report does not need buffering. */
    if (event->sync && lock->pending_code == 0) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (lock->pending_code == 0) {
        /* Keep the value for comparison, but do not let it reach the listener
         * until the possible second axis has been examined. */
        lock->pending_code = event->code;
        lock->pending_value = event->value;
        event->value = 0;
        return ZMK_INPUT_PROC_CONTINUE;
    }

    const uint16_t current_code = event->code;
    const int32_t current_value = event->value;
    const bool current_wins = magnitude(current_value) >= magnitude(lock->pending_value);

    if (current_code == lock->pending_code) {
        /* Some drivers can emit the same axis more than once in a report. */
        event->value = current_value + lock->pending_value;
    } else if (current_wins) {
        /* The current event is already the dominant axis. */
        event->code = current_code;
        event->value = current_value;
    } else {
        /* Reuse the current event slot for the previously buffered axis. */
        event->code = lock->pending_code;
        event->value = lock->pending_value;
    }

    lock->pending_code = 0;
    lock->pending_value = 0;
    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api axis_lock_api = {
    .handle_event = axis_lock_handle_event,
};

#define SOSHO_AXIS_LOCK_INST(n)                                                                    \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                  \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &axis_lock_api);

DT_INST_FOREACH_STATUS_OKAY(SOSHO_AXIS_LOCK_INST)
