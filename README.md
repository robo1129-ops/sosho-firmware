# SO-SHO firmware

This private configuration is based on the official SO-SHO unified ZMK template (https://github.com/elekit-official/unified-zmk-config-template) and keeps its GitHub Actions build workflow and hardware definitions. The official build guide (https://www.elekit.co.jp/pages/TH-601-guide.php) identifies the right side as Central and the left side as Peripheral.

## Daily hardware variant

The explicit daily build is:

- Right side: Central, PMW3610 trackball, artifact sosho_R_trackball.
- Left side: Peripheral, Cirque Pinnacle/GlidePoint trackpad, artifact sosho_L_trackpad.

The side roles must not be swapped: the right-central build carries the Studio USB/BLE transport, while the left-peripheral build does not. Alternate right-trackpad and left-trackball combinations, plus settings_reset, remain in build.yaml.

## Build-time sensitivity controls

Edit the named values at the top of boards/shields/sosho/sosho.dtsi, then push a commit and install the newly built UF2 files. These are build-time settings; there is no safe runtime sensitivity control exposed by the current drivers.

- SOSHO_TRACKBALL_CPI (default 600) sets the PMW3610 CPI through its documented cpi devicetree property.
- SOSHO_TRACKBALL_XY_SCALE_NUM / SOSHO_TRACKBALL_XY_SCALE_DEN (default 1/1) set the rational X/Y multiplier through ZMK's zip_xy_scaler.
- SOSHO_TRACKPAD_XY_SCALE_NUM / SOSHO_TRACKPAD_XY_SCALE_DEN (default 1/1) set the trackpad movement multiplier through the existing ZMK pointing listener path.
- The Pinnacle sensitivity property is currently 1x. The upstream binding defines 1x as most sensitive and 4x as least sensitive; values 1x through 4x require a rebuild.

For example, changing a scale to 2/1 doubles movement and 1/2 halves it. Keep denominators non-zero.

## ZMK Studio and BLE

The right-central variants include the upstream studio-rpc-usb-uart snippet, CONFIG_ZMK_STUDIO=y, CONFIG_ZMK_STUDIO_TRANSPORT_BLE=y, and CONFIG_ZMK_BLE=y. The BLE GATT Studio transport is therefore enabled in the central firmware configuration, while the peripheral configs only enable the Studio behavior needed by the shared keymap.

A one-time manual USB installation is still required: download the correct central UF2 from GitHub Actions, put the right XIAO board into its bootloader, and copy the UF2 to that board. This task did not flash or modify any physical keyboard. After that initial install, unlock Studio using the layer-3 top-left &studio_unlock binding: hold the existing &mo 3 layer key and press the top-left key. BLE Studio runtime still needs to be confirmed by the user after a real flash and connection; this repository alone cannot prove radio/runtime behavior.

## Trackpad taps and gestures

The current Zephyr Cirque Pinnacle driver exposes enough information for one-finger primary tap/click, so primary-tap-enable is enabled in sosho.dtsi. The driver reports that as the normal touch button path; it has not been tested on the physical keyboard here.

Two-finger tap/double-click is not implemented. Two-finger scrolling is not implemented. Source inspection shows the current driver exposes relative X/Y and a primary-touch button, does not expose finger count or raw multi-contact data, and configures scrolling and secondary taps off. No timing trick or ordinary key press is used to simulate these gestures. A future implementation would require a driver/input processor that exposes finger count or raw contacts plus a documented gesture-to-button/scroll event path.

The encoder's existing scroll behaviors remain separate from trackpad gesture scrolling.

## Verification

GitHub Actions is the source of truth for the build. The expected daily artifacts are sosho_R_trackball and sosho_L_trackpad; the other matrix entries are alternate combinations or settings reset. Runtime BLE, tap behavior, and gesture behavior remain unverified until a user performs the one-time USB flash and tests the hardware.

Official references: SO-SHO guide (https://www.elekit.co.jp/pages/TH-601-guide.php), official source/template (https://github.com/elekit-official/unified-zmk-config-template), ZMK Studio (https://zmk.dev/docs/features/studio), Studio unlock behavior (https://zmk.dev/docs/keymaps/behaviors/studio-unlock), and ZMK input processors (https://zmk.dev/docs/keymaps/input-processors).
