#include <errno.h>

#include <proto/zmk/sosho.pb.h>

#include <zmk/studio/rpc.h>

#include "sosho_sleep_runtime.h"

#define SO_SHO_RESPONSE(type, ...) ZMK_RPC_RESPONSE(sosho, type, __VA_ARGS__)

static zmk_studio_Response sleep_error(zmk_sosho_ErrorCode code) {
    return SO_SHO_RESPONSE(error, code);
}

static zmk_sosho_SleepSettings current_sleep_settings(void) {
    zmk_sosho_SleepSettings settings = zmk_sosho_SleepSettings_init_zero;
    settings.enabled = sosho_sleep_runtime_supported() && sosho_sleep_timeout_ms() > 0U;
    settings.timeout_ms = sosho_sleep_timeout_ms();
    settings.runtime_control = true;
    return settings;
}

zmk_studio_Response get_sleep(const zmk_studio_Request *req) {
    ARG_UNUSED(req);
    return SO_SHO_RESPONSE(get_sleep, current_sleep_settings());
}

zmk_studio_Response set_sleep_timeout_ms(const zmk_studio_Request *req) {
    uint32_t timeout_ms = req->subsystem.sosho.request_type.set_sleep_timeout_ms;
    int err = sosho_sleep_set_timeout_ms(timeout_ms);
    if (err == -ENOTSUP) return sleep_error(zmk_sosho_ErrorCode_ERROR_NOT_SUPPORTED);
    if (err < 0) return sleep_error(zmk_sosho_ErrorCode_ERROR_BAD_ARGUMENT);
    return SO_SHO_RESPONSE(set_sleep_timeout_ms, current_sleep_settings());
}

ZMK_RPC_SUBSYSTEM(sosho)
ZMK_RPC_SUBSYSTEM_HANDLER(sosho, get_sleep, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(sosho, set_sleep_timeout_ms, ZMK_STUDIO_RPC_HANDLER_SECURED);
