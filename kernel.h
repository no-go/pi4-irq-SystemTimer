#ifndef __THEKERNEL
#define __THEKERNEL 1

extern unsigned long _regs[37];
extern void dispatch (void);

// implemented in startup.S 
extern void PUT32 (unsigned int, unsigned int);
extern unsigned int GET32 (unsigned int);

enum {
    C_black     = 0xFF000000,
    C_blue      = 0xFF0000AA,
    C_green     = 0xFF00AA00,
    C_cyan      = 0xFF00AAAA,
    C_red       = 0xFFAA0000,
    C_violett   = 0xFFAA00AA,
    C_brown     = 0xFFAA5500,
    C_lightgray = 0xFFAAAAAA,
    
    C_gray      = 0xFF555555,
    C_lightblue = 0xFFABABFF,
    C_lightgreen= 0xFFABFFAB,
    C_lightcyan = 0xFF55FFFF,
    C_lightred  = 0xFFFF5555,
    C_pink      = 0xFFFF55FF,
    C_yellow    = 0xFFFFFF55,
    C_white     = 0xFFFFFFFF
};

// Pi4 B, BCM2711, 4MB Base Addresses
#define MMIO_BASE       0xFE000000

// System Timer (and Comparator 3)
#define PIT_STATUS      (MMIO_BASE+0x00003000)
#define PIT_LOW         (MMIO_BASE+0x00003004)
#define PIT_Compare3    (MMIO_BASE+0x00003018)
#define PIT_SPI         99
#define PIT_MASKBIT     3

// IRQ and GIC-400
#define GIC_IRQS        192
#define GIC_SPURIOUS    1023
#define GICD_BASE       0xFF841000
#define GICD_CTLR       (GICD_BASE + 0x000)
#define GICD_ENABLE     (GICD_BASE + 0x100)
#define GICD_DISABLE    (GICD_BASE + 0x180)
#define GICD_PEND_CLR   (GICD_BASE + 0x280)
#define GICD_ACTIVE_CLR (GICD_BASE + 0x380)
#define GICD_PRIO       (GICD_BASE + 0x400)
#define GICD_TARGET     (GICD_BASE + 0x800)
#define GICD_CFG        (GICD_BASE + 0xc00)
// GIC-400 CPU Interface Controller
#define GICC_BASE       0xFF842000
#define GICC_CTLR       (GICC_BASE + 0x000)
#define GICC_PRIO       (GICC_BASE + 0x004)
#define GICC_ACK        (GICC_BASE + 0x00c)
#define GICC_EOI        (GICC_BASE + 0x010)

// GPIOs
#define GPFSEL0         (MMIO_BASE+0x00200000)
#define GPPUD           (MMIO_BASE+0x00200094)
#define GPPUDCLK0       (MMIO_BASE+0x00200098)

// PL011 UART
#define UART3_DR        (MMIO_BASE+0x00201600)
#define UART3_FR        (MMIO_BASE+0x00201618)
#define UART3_IBRD      (MMIO_BASE+0x00201624)
#define UART3_FBRD      (MMIO_BASE+0x00201628)
#define UART3_LCRH      (MMIO_BASE+0x0020162C)
#define UART3_CR        (MMIO_BASE+0x00201630)
#define UART3_ICR       (MMIO_BASE+0x00201644)

// GPU messagebox
#define MBOX_READ       (MMIO_BASE+0x0000B880)
#define MBOX_STATUS     (MMIO_BASE+0x0000B898)
#define MBOX_WRITE      (MMIO_BASE+0x0000B8A0)
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

#endif
