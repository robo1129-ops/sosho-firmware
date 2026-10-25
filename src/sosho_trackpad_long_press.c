/*
 * SO-SHO Cirque Pinnacle touch gesture processor.
 *
 * The official Pinnacle driver reports a primary contact as INPUT_BTN_TOUCH.
 * ZMK's input listener normally turns that into left-button down/up events
 * immediately. This processor delays the short tap until release so a
 * contact held for the configured duration can become a right click instead.
 * Relative motion is never changed, so a held contact remains usable for
 * drag-and-drop once the right button has been pressed.
 */

#define DT_DRV_COMPAT sosho_input_processor_trackpad_long_press

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <drivers/input_processor.h>

#define SOSHO_LONG_PRESS_STATE_COUNT 8

struct sosho_long_press_config {
    uint32_t long_press_ms;
    uint32_t tap_move_threshold;
};

struct sosho_long_press_state {
    struct k_timer timer;
    const struct device *source;
    bool pressed;
    bool right_pressed;
    bool tap_candidate;
    uint32_t tap_motion;
};

static struct sosho_long_press_state states[SOSHO_LONG_PRESS_STATE_COUNT];

static struct sosho_long_press_state *state_for(const struct zmk_input_processor_state *state) {
    uint8_t index = state->input_device_index;

    if (index >= SOSHO_LONG_PRESS_STATE_COUNT) {
        index = 0;
    }

    return &states[index];
}

static void emit_button(const struct device *source, uint16_t code, int32_t value, bool sync) {
    if (source != NULL) {
        (void)input_report_key(source, code, value, sync, K_FOREVER);
    }
}

static void record_motion(const struct sosho_long_press_config *config,
                          struct sosho_long_press_state *state, int32_t value) {
    uint32_t magnitude = value < 0 ? (uint32_t)(-(int64_t)value) : (uint32_t)value;

    if (!state->tap_candidate || config->tap_move_threshold == 0) {
        return;
    }

    if (magnitude > config->tap_move_threshold - MIN(state->tap_motion,
                                                      config->tap_move_threshold)) {
        state->tap_motion = config->tap_move_threshold + 1;
        state->tap_candidate = false;
        return;
    }

    state->tap_motion += magnitude;
    if (state->tap_motion > config->tap_move_threshold) {
        state->tap_candidate = false;
    }
}

static void long_press_timer_handler(struct k_timer *timer) {
    struct sosho_long_press_state *state = CONTAINER_OF(timer, struct sosho_long_press_state, timer);

    if (!state->pressed || state->right_pressed || state->source == NULL) {
        return;
    }

    state->right_pressed = true;
    emit_button(state->source, INPUT_BTN_1, 1, true);
}

static int long_press_handle_event(const struct device *dev, struct input_event *event,
                                   uint32_t param1, uint32_t param2,
                                   struct zmk_input_processor_state *processor_state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    struct sosho_long_press_state *state = state_for(processor_state);
    const struct sosho_long_press_config *config = dev->config;

    /* The first contact sample can contain a small amount of motion.  Keep
     * the tap candidate while the accumulated post-contact motion is within
     * the configured tolerance, so a slightly shifted fingertip still clicks.
     * A real drag cancels the click without affecting cursor movement. */
    if (event->type == INPUT_EV_REL && state->pressed &&
        (event->code == INPUT_REL_X || event->code == INPUT_REL_Y)) {
        record_motion(config, state, event->value);
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (event->type != INPUT_EV_KEY || event->code != INPUT_BTN_TOUCH) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    const bool pressed = event->value > 0;

    /* Consume the driver's immediate touch event. A short tap is emitted at
     * release, while a long contact is emitted by the timer as a right click. */
    event->value = 0;

    if (pressed) {
        if (state->pressed) {
            return ZMK_INPUT_PROC_CONTINUE;
        }

        state->source = event->dev;
        state->pressed = true;
        state->right_pressed = false;
        state->tap_candidate = true;
        state->tap_motion = 0;
        k_timer_start(&state->timer, K_MSEC(config->long_press_ms), K_NO_WAIT);
    } else {
        if (!state->pressed) {
            return ZMK_INPUT_PROC_CONTINUE;
        }

        k_timer_stop(&state->timer);
        if (state->right_pressed) {
            emit_button(state->source, INPUT_BTN_1, 0, true);
        } else if (state->tap_candidate) {
            /* Both edges must be synchronized.  If the press is emitted with
             * sync=false and the release with sync=true, the ZMK input
             * listener can send only the release report and the host sees no
             * click. */
            emit_button(state->source, INPUT_BTN_0, 1, true);
            emit_button(state->source, INPUT_BTN_0, 0, true);
        }
        state->source = NULL;
        state->pressed = false;
        state->right_pressed = false;
        state->tap_candidate = false;
        state->tap_motion = 0;
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api long_press_api = {
    .handle_event = long_press_handle_event,
};

static int long_press_init(const struct device *dev) {
    ARG_UNUSED(dev);

    for (size_t i = 0; i < ARRAY_SIZE(states); i++) {
        k_timer_init(&states[i].timer, long_press_timer_handler, NULL);
        states[i].source = NULL;
        states[i].pressed = false;
        states[i].right_pressed = false;
        states[i].tap_candidate = false;
        states[i].tap_motion = 0;
    }

    return 0;
}

#define SOSHO_LONG_PRESS_INST(n)                                                                  \
    static const struct sosho_long_press_config sosho_long_press_config_##n = {                   \
        .long_press_ms = DT_PROP_OR(DT_DRV_INST(n), long_press_ms, 500),                         \
        .tap_move_threshold = DT_PROP_OR(DT_DRV_INST(n), tap_move_threshold, 24),                \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, long_press_init, NULL, NULL, &sosho_long_press_config_##n,          \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &long_press_api);

DT_INST_FOREACH_STATUS_OKAY(SOSHO_LONG_PRESS_INST)
