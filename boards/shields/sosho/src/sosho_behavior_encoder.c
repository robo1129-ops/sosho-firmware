#define DT_DRV_COMPAT sosho_behavior_encoder

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/virtual_key_position.h>

#include "sosho_encoder_runtime.h"

LOG_MODULE_REGISTER(sosho_encoder, CONFIG_ZMK_LOG_LEVEL);

struct sosho_behavior_encoder_data {
    struct sensor_value remainder[ZMK_KEYMAP_SENSORS_LEN][ZMK_KEYMAP_LAYERS_LEN];
    int32_t triggers[ZMK_KEYMAP_SENSORS_LEN][ZMK_KEYMAP_LAYERS_LEN];
};

static int encoder_accept_data(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event,
                               const struct zmk_sensor_config *sensor_config,
                               size_t channel_data_size,
                               const struct zmk_sensor_channel_data *channel_data) {
    ARG_UNUSED(binding);
    if (channel_data_size == 0U || channel_data == NULL || sensor_config == NULL ||
        sensor_config->triggers_per_rotation == 0U) {
        return -EINVAL;
    }

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }
    struct sosho_behavior_encoder_data *data = dev->data;
    int sensor = ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(event.position);
    if (sensor < 0 || sensor >= ZMK_KEYMAP_SENSORS_LEN || event.layer >= ZMK_KEYMAP_LAYERS_LEN) {
        return -EINVAL;
    }

    struct sensor_value *remainder = &data->remainder[sensor][event.layer];
    const struct sensor_value value = channel_data[0].value;
    if ((value.val1 > 0 && remainder->val1 < 0) || (value.val1 < 0 && remainder->val1 > 0)) {
        remainder->val1 = 0;
        remainder->val2 = 0;
    }
    remainder->val1 += value.val1;
    remainder->val2 += value.val2;
    if (abs(remainder->val2) >= 1000000) {
        remainder->val1 += remainder->val2 / 1000000;
        remainder->val2 %= 1000000;
    }

    int trigger_degrees = 360 / sensor_config->triggers_per_rotation;
    data->triggers[sensor][event.layer] = remainder->val1 / trigger_degrees;
    remainder->val1 %= trigger_degrees;
    return 0;
}

static int encoder_process(struct zmk_behavior_binding *binding,
                            struct zmk_behavior_binding_event event,
                            enum behavior_sensor_binding_process_mode mode) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }
    struct sosho_behavior_encoder_data *data = dev->data;
    int sensor = ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(event.position);
    if (sensor < 0 || sensor >= ZMK_KEYMAP_SENSORS_LEN || event.layer >= ZMK_KEYMAP_LAYERS_LEN) {
        return -EINVAL;
    }

    int32_t triggers = data->triggers[sensor][event.layer];
    data->triggers[sensor][event.layer] = 0;
    if (mode != BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_TRIGGER || triggers == 0) {
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

    uint32_t direction = triggers < 0 ? 0U : 1U;
    if (triggers < 0) {
        triggers = -triggers;
    }

    struct zmk_behavior_binding target;
    if (sosho_encoder_binding_for(event.layer, (uint32_t)sensor, direction, &target) < 0) {
        return -EINVAL;
    }
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    event.source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL;
#endif
    for (int32_t i = 0; i < triggers; i++) {
        /* Invoke a complete encoder step immediately.  The generic behavior
         * queue is not linked into Peripheral builds, and a rotary step is a
         * pulse rather than a held key, so a direct press/release also avoids
         * introducing extra latency. */
        zmk_behavior_invoke_binding(&target, event, true);
        zmk_behavior_invoke_binding(&target, event, false);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api encoder_api = {
    .sensor_binding_accept_data = encoder_accept_data,
    .sensor_binding_process = encoder_process,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &encoder_api);
