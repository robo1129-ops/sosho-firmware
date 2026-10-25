#pragma once

/* Adjust the runtime multiplier used by the SO-SHO trackpad input processor. */
void sosho_runtime_scale_adjust(int delta_percent);
int sosho_runtime_scale_percent(void);

void sosho_scroll_scale_adjust(int delta_percent);
int sosho_scroll_scale_percent(void);
