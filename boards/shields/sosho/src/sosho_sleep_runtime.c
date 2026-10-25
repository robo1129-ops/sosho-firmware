#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/settings/settings.h>
#include <zephyr/toolchain.h>

#include <zmk/activity.h>

#include "sosho_sleep_runtime.h"

#if IS_ENABLED(CONFIG_ZMK_SLEEP)
#define SOSHO_DEFAULT_SLEEP_TIMEOUT_MS CONFIG_ZMK_IDLE_SLEEP_TIMEOUT
#else
#define SOSHO_DEFAULT_SLEEP_TIMEOUT_MS 0U
#endif

#define SOSHO_SLEEP_TIMEOUT_MIN_MS 60000U
#define SOSHO_SLEEP_TIMEOUT_MAX_MS 86400000U

static uint32_t sleep_timeout_ms = SOSHO_DEFAULT_SLEEP_TIMEOUT_MS;

static int settings_set(const char *name, size_t len_rd, settings_read_cb read_cb, void *cb_arg) {
    if (strcmp(name, "sleep_timeout_ms") != 0 || len_rd != sizeof(sleep_timeout_ms)) {
        return -ENOENT;
    }

    return read_cb(cb_arg, &sleep_timeout_ms, sizeof(sleep_timeout_ms)) < 0 ? -EINVAL : 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(sosho_runtime, "sosho_runtime", NULL, settings_set, NULL, NULL);

uint32_t sosho_sleep_timeout_ms(void) { return sleep_timeout_ms; }

bool sosho_sleep_runtime_supported(void) { return IS_ENABLED(CONFIG_ZMK_SLEEP); }

int sosho_sleep_set_timeout_ms(uint32_t timeout_ms) {
    if (timeout_ms != 0U &&
        (timeout_ms < SOSHO_SLEEP_TIMEOUT_MIN_MS || timeout_ms > SOSHO_SLEEP_TIMEOUT_MAX_MS ||
         timeout_ms % 60000U != 0U)) {
        return -EINVAL;
    }

    if (!sosho_sleep_runtime_supported()) {
        return -ENOTSUP;
    }

    sleep_timeout_ms = timeout_ms;
    return settings_save_one("sosho_runtime/sleep_timeout_ms", &sleep_timeout_ms,
                             sizeof(sleep_timeout_ms));
}

/* Override the build-time default in ZMK's activity manager. */
uint32_t zmk_activity_get_sleep_timeout_ms(void) { return sleep_timeout_ms; }
