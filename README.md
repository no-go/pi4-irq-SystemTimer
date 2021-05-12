# Pi1 System Timer IRQ

THIS BRANCH IS A BIT FREESTYLE AND HAS GRAPHIC SUPPORT to display a clock.

This is a minimal baremetal code to test system timer interrupt during
a main loop on raspberry Pi1

- set clock via uart with `23:45` and return

Because I have 2 old Pi1, I try to migrate my new Pi4 stuff back to Pi1.

## todo

- LIC instead of GIC (I think: done, but untested)
- other BCM2835 toolchain (done, untested)
- different GPIOs for UART: 14,15 (done, untested)
- other SD-Card files (done, untested)
- other peripherial start address (done, untested)
- other boot start address (done, untested)
- Assemblercode complete different
