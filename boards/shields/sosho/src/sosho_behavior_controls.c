#define DT_DRV_COMPAT sosho_behavior_controls

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>

#include "sosho_runtime_scale.h"

LOG_MODULE_REGISTER(sosho_controls, CONFIG_ZMK_LOG_LEVEL);

enum sosho_control_action {
    SOSHO_CPI_DOWN,
    SOSHO_CPI_UP,
    SOSHO_TRACKPAD_DOWN,
    SOSHO_TRACKPAD_UP,
    SOSHO_AUTO_MOUSE_TOGGLE,
};

static int current_cpi = 200;
static bool auto_mouse_enabled;

#if DT_HAS_COMPAT_STATUS_OKAY(pixart_pmw3610_alt)
static const struct device *trackball_device(void) {
    const struct device *dev = DEVICE_DT_GET(DT_INST(0, pixart_pmw3610_alt));
    return device_is_ready(dev) ? dev : NULL;
}

static int set_cpi(int next) {
    struct sensor_value value = {.val1 = next, .val2 = 0};
    const struct device *dev = trackball_device();
    if (dev == NULL) {
        return -ENODEV;
    }
    int ret = sensor_attr_set(dev, SENSOR_CHAN_ALL, (enum sensor_attribute)0, &value);
    if (ret == 0) {
        current_cpi = next;
    }
    return ret;
}
#else
static int set_cpi(int next) {
    ARG_UNUSED(next);
    return -ENODEV;
}
#endif

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static const struct behavior_parameter_value_metadata values[] = {
    {.display_name = "CPI -", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = SOSHO_CPI_DOWN},
    {.display_name = "CPI +", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = SOSHO_CPI_UP},
    {.display_name = "Trackpad -", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = SOSHO_TRACKPAD_DOWN},
    {.display_name = "Trackpad +", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = SOSHO_TRACKPAD_UP},
    {.display_name = "Auto Mouse ON/OFF", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = SOSHO_AUTO_MOUSE_TOGGLE},
};

static const struct behavior_parameter_metadata_set metadata_set = {
    .param1_values = values,
    .param1_values_len = ARRAY_SIZE(values),
};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = 1,
    .sets = &metadata_set,
};
#endif

static int controls_pressed(struct zmk_behavior_binding *binding,
                            struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);
    switch (binding->param1) {
    case SOSHO_CPI_DOWN:
        return set_cpi(MAX(200, current_cpi - 200));
    case SOSHO_CPI_UP:
        return set_cpi(MIN(3200, current_cpi + 200));
    case SOSHO_TRACKPAD_DOWN:
        sosho_runtime_scale_adjust(-10);
        return 0;
    case SOSHO_TRACKPAD_UP:
        sosho_runtime_scale_adjust(10);
        return 0;
    case SOSHO_AUTO_MOUSE_TOGGLE:
        auto_mouse_enabled = !auto_mouse_enabled;
        LOG_INF("auto mouse %s", auto_mouse_enabled ? "enabled" : "disabled");
        return 0;
    default:
        return -EINVAL;
    }
}

static int controls_released(struct zmk_behavior_binding *binding,
                             struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api controls_api = {
    .binding_pressed = controls_pressed,
    .binding_released = controls_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &controls_api);

#define DT_DRV_COMPAT sosho_behavior_wheel

enum sosho_wheel_action { SOSHO_WHEEL_UP, SOSHO_WHEEL_DOWN, SOSHO_WHEEL_LEFT, SOSHO_WHEEL_RIGHT };

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static const struct behavior_parameter_value_metadata wheel_values[] = {
    {.display_name = "Wheel Up", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = SOSHO_WHEEL_UP},
    {.display_name = "Wheel Down", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = SOSHO_WHEEL_DOWN},
    {.display_name = "Wheel Left", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = SOSHO_WHEEL_LEFT},
    {.display_name = "Wheel Right", .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = SOSHO_WHEEL_RIGHT},
};
static const struct behavior_parameter_metadata_set wheel_set = {
    .param1_values = wheel_values,
    .param1_values_len = ARRAY_SIZE(wheel_values),
};
static const struct behavior_parameter_metadata wheel_metadata = {
    .sets_len = 1,
    .sets = &wheel_set,
};
#endif

static int wheel_pressed(struct zmk_behavior_binding *binding,
                         struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);
    int16_t x = 0;
    int16_t y = 0;
    switch (binding->param1) {
    case SOSHO_WHEEL_UP:
        y = 1;
        break;
    case SOSHO_WHEEL_DOWN:
        y = -1;
        break;
    case SOSHO_WHEEL_LEFT:
        x = -1;
        break;
    case SOSHO_WHEEL_RIGHT:
        x = 1;
        break;
    default:
        return -EINVAL;
    }
    zmk_hid_mouse_scroll_set(x, y);
    int ret = zmk_endpoint_send_mouse_report();
    zmk_hid_mouse_scroll_set(0, 0);
    return ret;
}

static int wheel_released(struct zmk_behavior_binding *binding,
                          struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api wheel_api = {
    .binding_pressed = wheel_pressed,
    .binding_released = wheel_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &wheel_metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wheel_api);
