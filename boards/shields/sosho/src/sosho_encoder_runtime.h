#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zmk/behavior.h>

#define SOSHO_ENCODER_SENSOR_COUNT 2U
#define SOSHO_ENCODER_DIRECTION_COUNT 2U
#define SOSHO_ENCODER_MAX_BINDINGS 32U

struct sosho_encoder_rpc_binding {
    uint32_t layer_id;
    uint32_t sensor;
    uint32_t direction;
    uint32_t behavior_id;
    uint32_t param1;
    uint32_t param2;
};

int sosho_encoder_get(struct sosho_encoder_rpc_binding *bindings, size_t capacity,
                      size_t *count);
int sosho_encoder_set(const struct sosho_encoder_rpc_binding *bindings, size_t count);

/* Resolve the binding for one sensor and direction at runtime. */
int sosho_encoder_binding_for(uint32_t layer_id, uint32_t sensor, uint32_t direction,
                              struct zmk_behavior_binding *binding);

bool sosho_encoder_runtime_supported(void);
