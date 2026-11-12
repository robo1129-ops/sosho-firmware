#include "sosho_encoder_runtime.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include <zmk/behavior.h>
#include <zmk/keymap.h>

#define SOSHO_ENCODER_PERSISTED_COUNT \
    (ZMK_KEYMAP_LAYERS_LEN * SOSHO_ENCODER_SENSOR_COUNT * SOSHO_ENCODER_DIRECTION_COUNT)

struct sosho_encoder_persisted_binding {
    uint16_t behavior_id;
    uint16_t reserved;
    uint32_t param1;
    uint32_t param2;
    uint8_t valid;
    uint8_t padding[3];
};

static struct sosho_encoder_persisted_binding persisted[SOSHO_ENCODER_PERSISTED_COUNT];
static bool defaults_initialized;

static size_t binding_index(uint32_t layer_id, uint32_t sensor, uint32_t direction) {
    return ((size_t)layer_id * SOSHO_ENCODER_SENSOR_COUNT + sensor) *
           SOSHO_ENCODER_DIRECTION_COUNT + direction;
}

static struct zmk_behavior_binding default_binding(uint32_t sensor, uint32_t direction) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = "sosho_wheel",
        .param1 = 0,
        .param2 = 0,
    };

    /* The physical SO-SHO layout is left encoder = horizontal and right
     * encoder = vertical.  Direction 0 is counter-clockwise, direction 1 is
     * clockwise.  These defaults intentionally match the original DTS map,
     * but are now stored as editable runtime bindings. */
    if (sensor == 0U) {
        binding.param1 = direction == 0U ? 2U : 3U; /* left/right */
    } else {
        binding.param1 = direction == 0U ? 1U : 0U; /* down/up */
    }
    return binding;
}

static void initialize_defaults(void) {
    if (defaults_initialized) {
        return;
    }

    memset(persisted, 0, sizeof(persisted));
    defaults_initialized = true;
}

static int settings_set(const char *name, size_t len_rd, settings_read_cb read_cb, void *cb_arg) {
    initialize_defaults();
    if (strcmp(name, "encoder_bindings") != 0 || len_rd != sizeof(persisted)) {
        return -ENOENT;
    }

    return read_cb(cb_arg, persisted, sizeof(persisted)) < 0 ? -EINVAL : 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(sosho_encoder_runtime, "sosho_runtime", NULL, settings_set, NULL,
                               NULL);

static int resolve_persisted(uint32_t layer_id, uint32_t sensor, uint32_t direction,
                             struct zmk_behavior_binding *binding) {
    if (layer_id >= ZMK_KEYMAP_LAYERS_LEN || sensor >= SOSHO_ENCODER_SENSOR_COUNT ||
        direction >= SOSHO_ENCODER_DIRECTION_COUNT || binding == NULL) {
        return -EINVAL;
    }

    initialize_defaults();
    size_t index = binding_index(layer_id, sensor, direction);
    if (!persisted[index].valid) {
        *binding = default_binding(sensor, direction);
        return 0;
    }

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    const char *name = zmk_behavior_find_behavior_name_from_local_id(persisted[index].behavior_id);
    if (name == NULL) {
        /* A firmware update can remove a behavior that was previously saved.
         * Falling back to the safe wheel action keeps the encoder usable. */
        *binding = default_binding(sensor, direction);
        return 0;
    }

    binding->behavior_dev = name;
    binding->param1 = persisted[index].param1;
    binding->param2 = persisted[index].param2;
    return 0;
#else
    /* The peripheral does not own the Studio RPC or the persistent local-ID
     * table. Its encoder inputs, when present, must remain functional with
     * the safe physical defaults. */
    *binding = default_binding(sensor, direction);
    return 0;
#endif
}

int sosho_encoder_binding_for(uint32_t layer_id, uint32_t sensor, uint32_t direction,
                              struct zmk_behavior_binding *binding) {
    return resolve_persisted(layer_id, sensor, direction, binding);
}

int sosho_encoder_get(struct sosho_encoder_rpc_binding *bindings, size_t capacity,
                      size_t *count) {
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    ARG_UNUSED(bindings);
    ARG_UNUSED(capacity);
    ARG_UNUSED(count);
    return -ENOTSUP;
#else
    if (bindings == NULL || count == NULL || capacity < SOSHO_ENCODER_PERSISTED_COUNT) {
        return -EINVAL;
    }

    initialize_defaults();
    size_t output = 0U;
    for (uint32_t layer = 0U; layer < ZMK_KEYMAP_LAYERS_LEN; layer++) {
        for (uint32_t sensor = 0U; sensor < SOSHO_ENCODER_SENSOR_COUNT; sensor++) {
            for (uint32_t direction = 0U; direction < SOSHO_ENCODER_DIRECTION_COUNT; direction++) {
                struct zmk_behavior_binding binding;
                int err = resolve_persisted(layer, sensor, direction, &binding);
                if (err < 0) {
                    return err;
                }
                bindings[output].layer_id = layer;
                bindings[output].sensor = sensor;
                bindings[output].direction = direction;
                bindings[output].behavior_id = zmk_behavior_get_local_id(binding.behavior_dev);
                bindings[output].param1 = binding.param1;
                bindings[output].param2 = binding.param2;
                if (bindings[output].behavior_id == UINT16_MAX) {
                    return -ENODEV;
                }
                output++;
            }
        }
    }
    *count = output;
    return 0;
#endif
}

int sosho_encoder_set(const struct sosho_encoder_rpc_binding *bindings, size_t count) {
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    ARG_UNUSED(bindings);
    ARG_UNUSED(count);
    return -ENOTSUP;
#else
    if (bindings == NULL || count > SOSHO_ENCODER_MAX_BINDINGS) {
        return -EINVAL;
    }

    initialize_defaults();
    for (size_t i = 0U; i < count; i++) {
        const struct sosho_encoder_rpc_binding *input = &bindings[i];
        if (input->layer_id >= ZMK_KEYMAP_LAYERS_LEN || input->sensor >= SOSHO_ENCODER_SENSOR_COUNT ||
            input->direction >= SOSHO_ENCODER_DIRECTION_COUNT) {
            return -EINVAL;
        }
        if (input->behavior_id > UINT16_MAX ||
            zmk_behavior_find_behavior_name_from_local_id((uint16_t)input->behavior_id) == NULL) {
            return -EINVAL;
        }
        if (input->behavior_id == zmk_behavior_get_local_id("sosho_encoder")) {
            return -EINVAL;
        }

        struct sosho_encoder_persisted_binding *target =
            &persisted[binding_index(input->layer_id, input->sensor, input->direction)];
        target->behavior_id = (uint16_t)input->behavior_id;
        target->param1 = input->param1;
        target->param2 = input->param2;
        target->valid = 1U;
    }

    return settings_save_one("sosho_runtime/encoder_bindings", persisted, sizeof(persisted));
#endif
}

bool sosho_encoder_runtime_supported(void) {
    return IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL);
}
