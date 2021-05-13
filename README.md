# Pi4 System Timer IRQ

This is a minimal baremetal code to test system timer interrupt during
a main loop on raspberry Pi4 and aarch64.

Many bare metal tutorials and code examples are:

- to complex to understand (many 32/64bit and Pi3/4 switches)
- do polling instead of interrupt for the timer
- uses Exceptions in a kind of bluescreen without saving registers
- did not use System Timer; they use the clockrate depending timer of a core
- did not explain or share a minimal example for GIC-400 (gicv2) on the Pi4

This minimum example has/is:

- Pi4 only
- AArch64 only
- minimum assembler code
- many in code comments
- minimum code tweaks and files

What this code do:

- hint: some firmware `armstub8` code init System Timer to 1MHz before
  switching from EL3 to EL2
- `startup.S` use only core0 (subcores are idle)
- set 2 stacks: 0x80000 for c code, 0x40000 for exception asm/c code
- switch from EL2 to EL1 with an valid exception vector for IRQ
- `kernel.c` mask interrupts
- init the GIC-400 to forward all interrupts to core0 with prio A0
- init the GIC-400 core0 to react on prio lower than F0 (00 is the highest prio)
- say GIC-400 to react on SPI 99 (= system timer comparator 3)
- set comparator3 on system timer to an initial value
- say system timer to make IRQ if comparator3 is reached
- clear interrupt mask
- init uart and send "boo"
- no framebuffer output, just an output on UART 115200 on GPIO Pins 4 and 5
- NOT aux mini uart, not GPIO Pin 14,15
- running a loop and send sometimes "m" for signaling "main loop"
- if interrupt occours, a "i" is sended
- if interrupt is a timer interrupt a "t" is sended and comparator gets an new value

The code maybe not compatible with qemu (April 2020) or [AEMv8-A Base Fixed Virtual Platform](http://www.arm.com/fvp).

## Compile (on linux) kernel8.img

Take a look into `Makefile` and get/set your noeabi gcc crosscompile toolchain.
Then cross your fingers and do `make`.

Take a look into `config.txt` for additional files you need.

## A Graphical Clock and Pi1

Look into the `someGraphics` and `someGraphics_pi1` Branch. There is a cool digital graphical clock
and a good comparison between Pi1 and Pi4 interrupt handling.

## Links and Sources

This code is the best minimal mix from serveral different open source projects:

- minimal uart from [raspbootin](https://github.com/mrvn/raspbootin)
- register save and restore from [aarch64 disasm mini debug](https://gitlab.com/bztsrc/minidbg)
- some inital boot code from [raspi3-tutorial](https://github.com/bztsrc/raspi3-tutorial/blob/master/11_exceptions/start.S)
- the gic-400 stuff and system timer from [circle library](https://github.com/rsta2/circle)
- some pi4 specific from [Raspberry Pi4 osdev](https://github.com/isometimes/rpi4-osdev)
- some other minimalistic tricks and UART3 from [David Welch pi3 baremetal tutorial](https://github.com/dwelch67/raspberrypi/tree/master/boards/pi3/aarch64/)
- some hints about the system timer and exceptions from [linux os explain on the Pi](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson02/rpi-os.md)

