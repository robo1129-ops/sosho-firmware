/*
 * SO-SHO absolute Cirque Pinnacle processor.
 *
 * The Pinnacle driver can report absolute X/Y/Z samples.  This processor
 * turns them back into the relative mouse events ZMK's pointing listener
 * consumes, while reserving the outer ring of the round pad for vertical
 * wheel scrolling.  The Z (pressure) sample is also converted to
 * BTN_TOUCH, so the existing tap/long-press processor keeps working.
 */

#define DT_DRV_COMPAT sosho_input_processor_trackpad_edge_scroll

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/sys/util.h>

#include <drivers/input_processor.h>

#define SOSHO_EDGE_STATE_COUNT 8
/* Cirque's default active window is rectangular (128..1920, 64..1472),
 * while the physical SO-SHO pad is round.  Normalize each axis separately
 * before testing the radius; using one raw-coordinate radius makes the top,
 * bottom, left and right edges disagree and causes jumpy corner behavior. */
#define SOSHO_PAD_CENTER_X 1024
#define SOSHO_PAD_CENTER_Y 768
#define SOSHO_PAD_RADIUS_X 896
#define SOSHO_PAD_RADIUS_Y 704
#define SOSHO_RADIUS_SCALE 100

struct sosho_edge_config {
    uint8_t edge_band_percent;
    uint16_t scroll_divider;
};

struct sosho_edge_state {
    const struct device *source;
    int32_t x;
    int32_t y;
    int32_t pending_dx;
    int32_t scroll_remainder;
    bool have_position;
    bool touch_active;
    bool edge_active;
};

static struct sosho_edge_state states[SOSHO_EDGE_STATE_COUNT];

static struct sosho_edge_state *state_for(const struct device *dev,
                                          const struct input_event *event,
                                          const struct zmk_input_processor_state *processor_state) {
    uint8_t index = processor_state ? processor_state->input_device_index : 0;

    if (index >= ARRAY_SIZE(states)) {
        index = 0;
    }

    struct sosho_edge_state *state = &states[index];
    if (state->source != event->dev && state->source != NULL) {
        state->x = 0;
        state->y = 0;
        state->pending_dx = 0;
        state->scroll_remainder = 0;
        state->have_position = false;
        state->touch_active = false;
        state->edge_active = false;
    }
    state->source = event->dev ? event->dev : dev;
    return state;
}

static bool on_edge(const struct sosho_edge_config *config, int32_t x, int32_t y) {
    int64_t dx = ((int64_t)x - SOSHO_PAD_CENTER_X) * SOSHO_RADIUS_SCALE /
                 SOSHO_PAD_RADIUS_X;
    int64_t dy = ((int64_t)y - SOSHO_PAD_CENTER_Y) * SOSHO_RADIUS_SCALE /
                 SOSHO_PAD_RADIUS_Y;
    int64_t distance_squared = (dx * dx) + (dy * dy);
    int64_t radius = config->edge_band_percent;
    int64_t radius_squared = radius * radius;

    return distance_squared >= radius_squared;
}

static int32_t divide_with_remainder(int32_t value, int32_t divider, int32_t *remainder) {
    int32_t total = value + *remainder;
    int32_t result = total / divider;
    *remainder = total - (result * divider);
    return result;
}

static void emit_relative(struct input_event *event, uint16_t code, int32_t value, bool sync) {
    event->type = INPUT_EV_REL;
    event->code = code;
    event->value = value;
    event->sync = sync;
}

static int edge_scroll_handle_event(const struct device *dev, struct input_event *event,
                                    uint32_t param1, uint32_t param2,
                                    struct zmk_input_processor_state *processor_state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    if (event->type != INPUT_EV_ABS && event->type != INPUT_EV_REL &&
        event->type != INPUT_EV_KEY) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    struct sosho_edge_state *state = state_for(dev, event, processor_state);
    const struct sosho_edge_config *config = dev->config;

    if (event->type == INPUT_EV_REL || event->type == INPUT_EV_KEY) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    switch (event->code) {
    case INPUT_ABS_X: {
        int32_t next_x = event->value;
        int32_t delta_x = state->have_position ? next_x - state->x : 0;
        state->x = next_x;
        state->have_position = true;
        state->pending_dx = delta_x;
        /* The listener cannot consume ABS events, so turn even a suppressed
         * first contact into a zero relative event. */
        emit_relative(event, INPUT_REL_X,
                      state->touch_active && !state->edge_active ? delta_x : 0, false);
        break;
    }
    case INPUT_ABS_Y: {
        int32_t next_y = event->value;
        int32_t delta_y = state->have_position ? next_y - state->y : 0;
        state->y = next_y;
        state->have_position = true;
        if (state->touch_active && state->edge_active) {
            /* Tangential motion around the circumference becomes vertical
             * wheel motion.  Radial motion contributes zero, avoiding
             * diagonal scrolling at the edge. */
            int32_t radial_x = state->x - SOSHO_PAD_CENTER_X;
            int32_t radial_y = state->y - SOSHO_PAD_CENTER_Y;
            int32_t tangent = (-state->pending_dx * radial_y) + (delta_y * radial_x);
            int32_t scroll = divide_with_remainder(tangent, config->scroll_divider,
                                                    &state->scroll_remainder);
            emit_relative(event, INPUT_REL_WHEEL, scroll, true);
        } else {
            emit_relative(event, INPUT_REL_Y,
                          state->touch_active ? delta_y : 0, true);
        }
        state->pending_dx = 0;
        break;
    }
    case INPUT_ABS_Z: {
        bool pressed = event->value > 0;
        if (pressed && !state->touch_active) {
            state->edge_active = state->have_position && on_edge(config, state->x, state->y);
            state->scroll_remainder = 0;
        } else if (!pressed) {
            state->edge_active = false;
            state->scroll_remainder = 0;
        }
        state->touch_active = pressed;
        event->type = INPUT_EV_KEY;
        event->code = INPUT_BTN_TOUCH;
        event->value = pressed ? 1 : 0;
        event->sync = true;
        break;
    }
    default:
        break;
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api edge_scroll_api = {
    .handle_event = edge_scroll_handle_event,
};

#define SOSHO_EDGE_SCROLL_INST(n)                                                                \
    static const struct sosho_edge_config sosho_edge_config_##n = {                              \
        .edge_band_percent = DT_PROP_OR(DT_DRV_INST(n), edge_band_percent, 82),                  \
        .scroll_divider = DT_PROP_OR(DT_DRV_INST(n), scroll_divider, 4),                        \
    };                                                                                           \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &sosho_edge_config_##n, POST_KERNEL,              \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &edge_scroll_api);

DT_INST_FOREACH_STATUS_OKAY(SOSHO_EDGE_SCROLL_INST)
