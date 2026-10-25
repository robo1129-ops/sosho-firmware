# SO-SHO shield-local sources.
#
# ZMK loads a shield's CMakeLists.txt when the shield is selected. Keeping
# these sources here makes the custom input processors and Studio behaviors
# part of both local west builds and the reusable GitHub build workflow; a
# standalone CMakeLists.txt at the repository root is not consumed by ZMK's
# application build.
set(SOSHO_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../src")

target_sources(app PRIVATE
  "${SOSHO_SOURCE_DIR}/boot_delay_hack.c"
  "${SOSHO_SOURCE_DIR}/sosho_axis_lock.c"
  "${SOSHO_SOURCE_DIR}/sosho_runtime_scale.c"
)

if (NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE
    "${SOSHO_SOURCE_DIR}/sosho_scroll_inertia.c"
    "${SOSHO_SOURCE_DIR}/sosho_macro_runtime.c"
    "${SOSHO_SOURCE_DIR}/sosho_behavior_controls.c"
    "${SOSHO_SOURCE_DIR}/sosho_behavior_macro.c"
  )
endif()
