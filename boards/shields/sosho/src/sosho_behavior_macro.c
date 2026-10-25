#define DT_DRV_COMPAT sosho_behavior_runtime_macro

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include "sosho_macro_runtime.h"

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static const struct behavior_parameter_value_metadata macro_values[] = {
    {.display_name = "Macro 0", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 0},
    {.display_name = "Macro 1", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 1},
    {.display_name = "Macro 2", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 2},
    {.display_name = "Macro 3", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 3},
    {.display_name = "Macro 4", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 4},
    {.display_name = "Macro 5", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 5},
    {.display_name = "Macro 6", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 6},
    {.display_name = "Macro 7", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 7},
    {.display_name = "Macro 8", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 8},
    {.display_name = "Macro 9", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 9},
    {.display_name = "Macro 10", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 10},
    {.display_name = "Macro 11", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 11},
    {.display_name = "Macro 12", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 12},
    {.display_name = "Macro 13", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 13},
    {.display_name = "Macro 14", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 14},
    {.display_name = "Macro 15", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 15},
};

static const struct behavior_parameter_metadata_set macro_set = {
    .param1_values = macro_values,
    .param1_values_len = ARRAY_SIZE(macro_values),
};

static const struct behavior_parameter_metadata macro_metadata = {
    .sets_len = 1,
    .sets = &macro_set,
};
#endif

static int macro_pressed(struct zmk_behavior_binding *binding,
                         struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);
    return sosho_macro_run((uint8_t)binding->param1);
}

static int macro_released(struct zmk_behavior_binding *binding,
                          struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api macro_api = {
    .binding_pressed = macro_pressed,
    .binding_released = macro_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &macro_metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &macro_api);
