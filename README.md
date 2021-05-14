# Pi1 System Timer IRQ GUI Clock

Because I have 2 old Pi1, I try to migrate my new Pi4 stuff back to Pi1.

- AArch32
- ARMv6 architecture
- BCM2835 = ARM1176JZF-S

## Specials

- LIC instead of GIC
- other BCM2835 toolchain
- different GPIOs for UART: 14,15
- other SD-Card files
- other peripherial start address
- other boot start address
- Assemblercode complete different
- completly different exception handling / vector

## this branch

- Pi1 on 1024x768
- Set hours and mintutes with GPIO 17,18 (to ground)
- float numbers in code ?! not working
