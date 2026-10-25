/*
 * Copyright (c) 2026 SO-SHO Contributors
 * SPDX-License-Identifier: MIT
 *
 * Encoder-to-scroll behavior. This is kept in the SO-SHO firmware so the
 * encoder behavior is reproducible without depending on a mutable external
 * module revision.
 */

#define DT_DRV_COMPAT sosho_behavior_encoder_mouse_scroll

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/virtual_key_position.h>
#include <dt-bindings/zmk/pointing.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct sosho_encoder_scroll_data {
    struct sensor_value remainder[ZMK_KEYMAP_SENSORS_LEN][ZMK_KEYMAP_LAYERS_LEN];
    int32_t triggers[ZMK_KEYMAP_SENSORS_LEN][ZMK_KEYMAP_LAYERS_LEN];
};

static int sosho_encoder_scroll_accept_data(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event,
    const struct zmk_sensor_config *sensor_config, size_t channel_data_size,
    const struct zmk_sensor_channel_data *channel_data) {
    if (channel_data_size == 0 || channel_data == NULL) {
        return -EINVAL;
    }

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }

    struct sosho_encoder_scroll_data *data = dev->data;
    const struct sensor_value value = channel_data[0].value;
    const int sensor_index = ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(event.position);
    struct sensor_value *remainder = &data->remainder[sensor_index][event.layer];
    int32_t triggers;

    /* EC11 revisions report a tick count in val2 with val1 == 0. */
    if (value.val1 == 0) {
        triggers = value.val2;
    } else {
        if ((value.val1 > 0 && remainder->val1 < 0) ||
            (value.val1 < 0 && remainder->val1 > 0)) {
            remainder->val1 = 0;
            remainder->val2 = 0;
        }

        remainder->val1 += value.val1;
        remainder->val2 += value.val2;

        if (abs(remainder->val2) >= 1000000) {
            remainder->val1 += remainder->val2 / 1000000;
            remainder->val2 %= 1000000;
        }

        const int trigger_degrees = 360 / sensor_config->triggers_per_rotation;
        triggers = remainder->val1 / trigger_degrees;
        remainder->val1 %= trigger_degrees;
    }

    data->triggers[sensor_index][event.layer] = triggers;
    return 0;
}

static int sosho_encoder_scroll_process(
    struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event,
    enum behavior_sensor_binding_process_mode mode) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }

    struct sosho_encoder_scroll_data *data = dev->data;
    const int sensor_index = ZMK_SENSOR_POSITION_FROM_VIRTUAL_KEY_POSITION(event.position);
    const int32_t triggers = data->triggers[sensor_index][event.layer];
    data->triggers[sensor_index][event.layer] = 0;

    if (mode != BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_TRIGGER || triggers == 0) {
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

    const uint32_t param = triggers > 0 ? binding->param1 : binding->param2;
    int16_t horizontal = MOVE_X_DECODE(param);
    int16_t vertical = MOVE_Y_DECODE(param);
    const int abs_triggers = triggers > 0 ? triggers : -triggers;

    horizontal = horizontal > 0 ? 1 : horizontal < 0 ? -1 : 0;
    vertical = vertical > 0 ? 1 : vertical < 0 ? -1 : 0;
    horizontal *= abs_triggers;
    vertical *= abs_triggers;

    zmk_hid_mouse_scroll_set(horizontal, vertical);
    zmk_endpoint_send_mouse_report();
    zmk_hid_mouse_scroll_set(0, 0);
    zmk_endpoint_send_mouse_report();

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api sosho_encoder_scroll_api = {
    .sensor_binding_accept_data = sosho_encoder_scroll_accept_data,
    .sensor_binding_process = sosho_encoder_scroll_process,
};

#define SOSHO_ENCODER_SCROLL_INST(n)                                                        \
    static struct sosho_encoder_scroll_data sosho_encoder_scroll_data_##n = {};             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &sosho_encoder_scroll_data_##n, NULL, POST_KERNEL, \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &sosho_encoder_scroll_api);

DT_INST_FOREACH_STATUS_OKAY(SOSHO_ENCODER_SCROLL_INST)
