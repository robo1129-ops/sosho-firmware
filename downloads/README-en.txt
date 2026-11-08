SO-SHO locally verified firmware

This ZIP contains six UF2 profiles built locally on 2026-10-28.
The normal pair is sosho_R_trackball.uf2 for the right Central and
sosho_L_trackpad.uf2 for the left Peripheral. Flash a matching pair.

For USB flashing, put only the right side into bootloader mode. The factory
keymap does not bind a physical bootloader key, which prevents accidental UF2
entry while Layer 3 is active. The bootloader action remains available from the
Web editor's special-action palette.

Runtime sleep, keymap and macro writes require the matching SO-SHO extension RPC
firmware. These local builds and UF2 generation were verified; physical runtime
save is not claimed until the Web editor shows a read-back match.
