#include <errno.h>
#include <string.h>

#include <proto/zmk/macro.pb.h>

#include <zmk/studio/rpc.h>

#include "sosho_macro_runtime.h"

#define MACRO_RESPONSE(type, ...) ZMK_RPC_RESPONSE(macro, type, __VA_ARGS__)

static zmk_studio_Response macro_error(zmk_macro_ErrorCode code) {
    return MACRO_RESPONSE(error, code);
}

zmk_studio_Response list_macros(const zmk_studio_Request *req) {
    ARG_UNUSED(req);
    return MACRO_RESPONSE(macro_ids, sosho_macro_list_ids());
}

zmk_studio_Response get_macro(const zmk_studio_Request *req) {
    const uint32_t requested_id = req->subsystem.macro.request_type.get_macro;
    if (requested_id >= SOSHO_MAX_MACROS) {
        return macro_error(zmk_macro_ErrorCode_ERROR_BAD_ARGUMENT);
    }

    zmk_macro_MacroData data = zmk_macro_MacroData_init_zero;
    data.id = requested_id;
    uint16_t length = 0;
    int err = sosho_macro_get((uint8_t)requested_id, data.data.bytes, sizeof(data.data.bytes), &length);
    if (err < 0) {
        return macro_error(err == -ENOSPC ? zmk_macro_ErrorCode_ERROR_NO_SPACE
                                          : zmk_macro_ErrorCode_ERROR_NOT_FOUND);
    }

    data.data.size = length;
    return MACRO_RESPONSE(get_macro, data);
}

zmk_studio_Response set_macro(const zmk_studio_Request *req) {
    const zmk_macro_SetMacroRequest *set_req =
        &req->subsystem.macro.request_type.set_macro;
    if (set_req->id >= SOSHO_MAX_MACROS || set_req->data.size > SOSHO_MAX_MACRO_PAYLOAD) {
        return macro_error(zmk_macro_ErrorCode_ERROR_BAD_ARGUMENT);
    }

    int err = sosho_macro_set((uint8_t)set_req->id, set_req->data.bytes, set_req->data.size);
    if (err < 0) {
        return macro_error(err == -ENOSPC ? zmk_macro_ErrorCode_ERROR_NO_SPACE
                                          : zmk_macro_ErrorCode_ERROR_STORAGE);
    }

    return MACRO_RESPONSE(set_macro, true);
}

zmk_studio_Response delete_macro(const zmk_studio_Request *req) {
    const uint32_t requested_id = req->subsystem.macro.request_type.delete_macro;
    if (requested_id >= SOSHO_MAX_MACROS) {
        return macro_error(zmk_macro_ErrorCode_ERROR_BAD_ARGUMENT);
    }

    int err = sosho_macro_delete((uint8_t)requested_id);
    return err < 0 ? macro_error(zmk_macro_ErrorCode_ERROR_STORAGE)
                   : MACRO_RESPONSE(delete_macro, true);
}

zmk_studio_Response run_macro(const zmk_studio_Request *req) {
    const uint32_t requested_id = req->subsystem.macro.request_type.run_macro;
    if (requested_id >= SOSHO_MAX_MACROS) {
        return macro_error(zmk_macro_ErrorCode_ERROR_BAD_ARGUMENT);
    }

    int err = sosho_macro_run((uint8_t)requested_id);
    if (err < 0) {
        return macro_error(err == -ENOENT ? zmk_macro_ErrorCode_ERROR_NOT_FOUND
                                          : zmk_macro_ErrorCode_ERROR_BUSY);
    }

    return MACRO_RESPONSE(run_macro, true);
}

ZMK_RPC_SUBSYSTEM(macro)
ZMK_RPC_SUBSYSTEM_HANDLER(macro, list_macros, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(macro, get_macro, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(macro, set_macro, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(macro, delete_macro, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(macro, run_macro, ZMK_STUDIO_RPC_HANDLER_SECURED);
