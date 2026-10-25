#define DT_DRV_COMPAT sosho_input_processor_runtime_scale

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <drivers/input_processor.h>

#include "sosho_runtime_scale.h"

/* Start at unity so the existing calibrated movement is unchanged. */
static volatile int scale_percent = 100;

void sosho_runtime_scale_adjust(int delta_percent) {
    int next = scale_percent + delta_percent;
    if (next < 25) {
        next = 25;
    } else if (next > 200) {
        next = 200;
    }
    scale_percent = next;
}

int sosho_runtime_scale_percent(void) { return scale_percent; }

static int runtime_scale_handle_event(const struct device *dev, struct input_event *event,
                                      uint32_t param1, uint32_t param2,
                                      struct zmk_input_processor_state *state) {
    ARG_UNUSED(dev);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    if (event->type != INPUT_EV_REL ||
        (event->code != INPUT_REL_X && event->code != INPUT_REL_Y)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    int32_t scaled = (int32_t)event->value * scale_percent;
    if (state != NULL && state->remainder != NULL) {
        scaled += *state->remainder;
    }
    int32_t result = scaled / 100;
    if (state != NULL && state->remainder != NULL) {
        *state->remainder = (int16_t)(scaled - (result * 100));
    }
    event->value = (int16_t)result;
    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api runtime_scale_api = {
    .handle_event = runtime_scale_handle_event,
};

#define SOSHO_RUNTIME_SCALE_INST(n)                                                               \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                 \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &runtime_scale_api);

DT_INST_FOREACH_STATUS_OKAY(SOSHO_RUNTIME_SCALE_INST)
