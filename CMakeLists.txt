zephyr_sources(boards/shields/sosho/src/boot_delay_hack.c boards/shields/sosho/src/sosho_axis_lock.c)
zephyr_sources(boards/shields/sosho/src/sosho_runtime_scale.c)

# HID reports and the active-layer state only exist on the split central.  The
# peripheral forwards its sensor events to the central, where this work item
# must run so the inertial tail is emitted through the host HID endpoint.
if (NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  zephyr_sources(boards/shields/sosho/src/sosho_scroll_inertia.c)
  # Runtime macro edits use a separate encrypted SO-SHO BLE GATT service.
  zephyr_sources(boards/shields/sosho/src/sosho_macro_runtime.c)
  zephyr_sources(boards/shields/sosho/src/sosho_behavior_controls.c boards/shields/sosho/src/sosho_behavior_macro.c)
endif()
