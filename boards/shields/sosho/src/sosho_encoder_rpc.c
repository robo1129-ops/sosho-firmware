#include <errno.h>

#include <proto/zmk/sosho.pb.h>

#include <zephyr/sys/util.h>

#include <zmk/studio/rpc.h>

#include "sosho_encoder_runtime.h"

#define SO_SHO_RESPONSE(type, ...) ZMK_RPC_RESPONSE(sosho, type, __VA_ARGS__)

static zmk_studio_Response encoder_error(zmk_sosho_ErrorCode code) {
    return SO_SHO_RESPONSE(error, code);
}

static zmk_sosho_EncoderSettings current_encoder_settings(void) {
    zmk_sosho_EncoderSettings settings = zmk_sosho_EncoderSettings_init_zero;
    struct sosho_encoder_rpc_binding bindings[SOSHO_ENCODER_MAX_BINDINGS];
    size_t count = 0U;
    if (sosho_encoder_get(bindings, ARRAY_SIZE(bindings), &count) < 0) {
        return settings;
    }

    settings.runtime_control = true;
    settings.bindings_count = count;
    for (size_t i = 0U; i < count; i++) {
        settings.bindings[i].layer_id = bindings[i].layer_id;
        settings.bindings[i].sensor = bindings[i].sensor;
        settings.bindings[i].direction = bindings[i].direction;
        settings.bindings[i].behavior_id = bindings[i].behavior_id;
        settings.bindings[i].param1 = bindings[i].param1;
        settings.bindings[i].param2 = bindings[i].param2;
    }
    return settings;
}

zmk_studio_Response get_encoders(const zmk_studio_Request *req) {
    ARG_UNUSED(req);
    return SO_SHO_RESPONSE(get_encoders, current_encoder_settings());
}

zmk_studio_Response set_encoders(const zmk_studio_Request *req) {
    const zmk_sosho_EncoderSettings *settings =
        &req->subsystem.sosho.request_type.set_encoders;
    if (settings->bindings_count > SOSHO_ENCODER_MAX_BINDINGS) {
        return encoder_error(zmk_sosho_ErrorCode_ERROR_BAD_ARGUMENT);
    }

    struct sosho_encoder_rpc_binding bindings[SOSHO_ENCODER_MAX_BINDINGS];
    for (size_t i = 0U; i < settings->bindings_count; i++) {
        bindings[i].layer_id = settings->bindings[i].layer_id;
        bindings[i].sensor = settings->bindings[i].sensor;
        bindings[i].direction = settings->bindings[i].direction;
        bindings[i].behavior_id = settings->bindings[i].behavior_id;
        bindings[i].param1 = settings->bindings[i].param1;
        bindings[i].param2 = settings->bindings[i].param2;
    }

    int err = sosho_encoder_set(bindings, settings->bindings_count);
    if (err < 0) {
        return encoder_error(err == -ENOTSUP ? zmk_sosho_ErrorCode_ERROR_NOT_SUPPORTED
                                             : zmk_sosho_ErrorCode_ERROR_BAD_ARGUMENT);
    }
    return SO_SHO_RESPONSE(set_encoders, current_encoder_settings());
}

