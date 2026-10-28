SO-SHO locally verified firmware

This ZIP contains six UF2 profiles built locally on 2026-10-28.
The normal pair is sosho_R_trackball.uf2 for the right Central and
sosho_L_trackpad.uf2 for the left Peripheral. Flash a matching pair.

For USB flashing, put only the right side into bootloader mode. The initial
right-Central keymap contains a bootloader action at the bottom-right position,
but it is a normal editable binding in the Web editor. The same position on the
left side is not a bootloader action.

Runtime sleep, keymap and macro writes require the matching SO-SHO extension RPC
firmware. These local builds and UF2 generation were verified; physical runtime
save is not claimed until the Web editor shows a read-back match.
