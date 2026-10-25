/*
 * SO-SHO absolute Cirque Pinnacle processor.
 *
 * The Pinnacle driver can report absolute X/Y/Z samples.  This processor
 * turns them back into the relative mouse events ZMK's pointing listener
 * consumes, while reserving the outer ring of the round pad for vertical
 * wheel scrolling.  The Z (pressure) sample is also converted to
 * BTN_TOUCH, so the existing tap/long-press processor keeps working.
 *
 * The driver reports X, Y, and Z as one sample (X/Y have sync=false and Z
 * has sync=true).  Do not emit the current X/Y delta immediately: the
 * release sample can contain X=0/Y=0, and treating it as movement causes a
 * large pointer jump.  One sample is held until its Z state is known; this
 * also suppresses the initial contact delta.  The resulting one-sample
 * latency is preferable to a release jump and is not visible during normal
 * use.
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
#define SOSHO_MAX_FRAME_DELTA 128

struct sosho_edge_config {
    uint8_t edge_band_percent;
    uint16_t scroll_divider;
};

struct sosho_edge_state {
    const struct device *source;
    int32_t x;
    int32_t y;
    int32_t frame_dx;
    int32_t frame_dy;
    int32_t last_dx;
    int32_t last_dy;
    int32_t last_x;
    int32_t last_y;
    int32_t scroll_remainder;
    bool have_position;
    bool frame_x_seen;
    bool frame_y_seen;
    bool frame_release_suspected;
    bool frame_valid;
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
        state->frame_dx = 0;
        state->frame_dy = 0;
        state->last_dx = 0;
        state->last_dy = 0;
        state->last_x = 0;
        state->last_y = 0;
        state->scroll_remainder = 0;
        state->have_position = false;
        state->frame_x_seen = false;
        state->frame_y_seen = false;
        state->frame_release_suspected = false;
        state->frame_valid = false;
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

static int32_t clamp_frame_delta(int32_t delta) {
    return CLAMP(delta, -SOSHO_MAX_FRAME_DELTA, SOSHO_MAX_FRAME_DELTA);
}

static int32_t normalized_radius_component(int32_t value, int32_t center,
                                           int32_t radius) {
    return ((value - center) * SOSHO_RADIUS_SCALE) / radius;
}

static int32_t previous_frame_scroll(const struct sosho_edge_state *state,
                                     const struct sosho_edge_config *config,
                                     int32_t *remainder) {
    const int32_t radial_x = normalized_radius_component(state->last_x,
                                                         SOSHO_PAD_CENTER_X,
                                                         SOSHO_PAD_RADIUS_X);
    const int32_t radial_y = normalized_radius_component(state->last_y,
                                                         SOSHO_PAD_CENTER_Y,
                                                         SOSHO_PAD_RADIUS_Y);
    const int32_t denominator = config->scroll_divider * SOSHO_RADIUS_SCALE;
    const int64_t cross = (-(int64_t)state->last_dx * radial_y) +
                          ((int64_t)state->last_dy * radial_x);

    /* Normalize the tangential displacement before applying the requested
     * scroll divider.  The old raw-coordinate cross product multiplied each
     * movement by roughly the pad radius, which made scrolling jumpy. */
    return divide_with_remainder((int32_t)cross, denominator, remainder);
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
        state->frame_dx = clamp_frame_delta(delta_x);
        state->frame_x_seen = true;
        /* Cirque's inactive report uses zero coordinates, outside its
         * active window (X starts at 128).  Mark the complete report so the
         * X event cannot replay the previous movement on release. */
        state->frame_release_suspected = state->frame_release_suspected || next_x == 0;

        /* Output the previous complete sample.  The current sample is held
         * until its Z event tells us whether it was a real touch or release. */
        emit_relative(event, INPUT_REL_X,
                      state->frame_valid && state->touch_active && !state->edge_active &&
                              !state->frame_release_suspected
                          ? state->last_dx
                          : 0,
                      false);
        break;
    }
    case INPUT_ABS_Y: {
        int32_t next_y = event->value;
        int32_t delta_y = state->have_position ? next_y - state->y : 0;
        state->y = next_y;
        state->have_position = true;
        state->frame_dy = clamp_frame_delta(delta_y);
        state->frame_y_seen = true;
        state->frame_release_suspected = state->frame_release_suspected || next_y == 0;

        if (state->frame_valid && state->touch_active && state->edge_active &&
            !state->frame_release_suspected) {
            /* Tangential motion around the circumference becomes vertical
             * wheel motion.  Radial motion contributes zero, avoiding
             * diagonal scrolling at the edge. */
            int32_t scroll = previous_frame_scroll(state, config,
                                                    &state->scroll_remainder);
            emit_relative(event, INPUT_REL_WHEEL, scroll, true);
        } else {
            emit_relative(event, INPUT_REL_Y,
                          state->frame_valid && state->touch_active && !state->edge_active &&
                                  !state->frame_release_suspected
                              ? state->last_dy
                              : 0,
                          true);
        }
        break;
    }
    case INPUT_ABS_Z: {
        bool pressed = event->value > 0;
        bool was_pressed = state->touch_active;

        if (pressed && !was_pressed) {
            state->edge_active = state->have_position && on_edge(config, state->x, state->y);
            state->scroll_remainder = 0;
            /* Do not emit the jump from the idle position to the first
             * contact.  The next held sample will become the first valid
             * movement sample. */
            state->frame_valid = false;
        } else if (pressed && was_pressed) {
            state->frame_valid = state->frame_x_seen && state->frame_y_seen;
            state->last_dx = state->frame_dx;
            state->last_dy = state->frame_dy;
            state->last_x = state->x;
            state->last_y = state->y;
        } else if (!pressed) {
            state->edge_active = false;
            state->scroll_remainder = 0;
            /* Never expose the release sample's coordinates as movement. */
            state->frame_valid = false;
        }
        state->touch_active = pressed;
        state->frame_x_seen = false;
        state->frame_y_seen = false;
        state->frame_release_suspected = false;
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
