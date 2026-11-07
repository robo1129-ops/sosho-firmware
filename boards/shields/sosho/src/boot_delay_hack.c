#include <zephyr/kernel.h>
#include <zephyr/init.h>

static int force_boot_delay(void) {
    /* Keep only the short rail-settle margin needed by the XIAO sensors.
     * The previous unconditional 1000 ms delay made every pointing device
     * appear frozen after a wireless wake. */
    k_busy_wait(40 * 1000);
    return 0;
}

// POST_KERNELより早い PRE_KERNEL_2 フェーズで実行
SYS_INIT(force_boot_delay, POST_KERNEL, 10);
