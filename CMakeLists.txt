zephyr_sources(src/boot_delay_hack.c src/sosho_axis_lock.c)

# HID reports and the active-layer state only exist on the split central.  The
# peripheral forwards its sensor events to the central, where this work item
# must run so the inertial tail is emitted through the host HID endpoint.
if (NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  zephyr_sources(src/sosho_scroll_inertia.c)
endif()
