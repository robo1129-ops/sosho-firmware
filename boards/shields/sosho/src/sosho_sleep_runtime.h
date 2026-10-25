#pragma once

#include <stdbool.h>
#include <stdint.h>

uint32_t sosho_sleep_timeout_ms(void);
int sosho_sleep_set_timeout_ms(uint32_t timeout_ms);
bool sosho_sleep_runtime_supported(void);
