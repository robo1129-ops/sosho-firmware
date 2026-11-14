# USB / BLE RPC stack correction

The right Central previously configured a 4096-byte Studio RPC stack.
Inspection of the locally generated ARM ELF showed these stack allocations:

| Function | Frame allocation, excluding saved registers |
| --- | ---: |
| rpc_main | 2384 bytes |
| get_encoders | 776 bytes |
| current_encoder_settings | 1552 bytes |

These nested frames alone total 4712 bytes, exceeding 4096 before saved
registers and other calls. This explains the observed stop after the keymap
and behavior metadata were received and encoder settings were requested.

The encoder settings helper now fills the response in place, removing a
776-byte temporary. All three right Central configurations use an 8192-byte
RPC stack. Left Peripheral configurations are unchanged.

Local right-trackball build: FLASH 295228 bytes (36.59%), RAM 101372 bytes
(38.67%). UF2 size 590848 bytes. SHA-256:
`1c1de79e42b39472623894e63359c5c82128f5bd8546502af0d760baa0f2c862`.

Chrome USB test after flashing: keymap and encoder settings loaded; Layer 3
left row 1 column 4 was changed from Up to F8; save acknowledgment and device
readback matched. The test key was restored to Up. BLE testing is separate;
USB success is not evidence of BLE success.
