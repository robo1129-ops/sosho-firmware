#pragma once

#include <stdint.h>

#define SOSHO_MAX_MACROS 16
#define SOSHO_MAX_MACRO_PAYLOAD 224

/* Run a macro stored by the SO-SHO runtime macro service. */
int sosho_macro_run(uint8_t id);

/* Runtime macro storage API used by the USB/BLE Studio RPC subsystem. */
uint32_t sosho_macro_list_ids(void);
int sosho_macro_get(uint8_t id, uint8_t *dst, uint16_t capacity, uint16_t *length);
int sosho_macro_set(uint8_t id, const uint8_t *data, uint16_t length);
int sosho_macro_delete(uint8_t id);
