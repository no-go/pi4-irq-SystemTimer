#ifndef __THEKERNEL
#define __THEKERNEL 1

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

// Pi1, BCM2835, Base Addresses
#define MMIO_BASE       0x20000000

// System Timer (and Comparator 3)
#define PIT_STATUS      (MMIO_BASE+0x00003000)
#define PIT_LOW         (MMIO_BASE+0x00003004)
#define PIT_Compare3    (MMIO_BASE+0x00003018)
#define PIT_IRQ         3
#define PIT_MASKBIT     3

// https://xinu.cs.mu.edu/index.php/BCM2835_Interrupt_Controller
// shared IRQ (Legacy Interrupt Controller)
#define IRQ1_PENDING_31_00  (MMIO_BASE+0x0000B204)
#define IRQ2_PENDING_31_00  (MMIO_BASE+0x0000B208)
#define IRQ0_PENDING_31_00  (MMIO_BASE+0x0000B200)
// Register to set and see enabled non Basic IRQs 31..0
#define IRQ1_SET_31_00      (MMIO_BASE+0x0000B210)  
// Register to Disable = Clear = Mask IRQs 31..0
#define IRQ1_CLEAR_31_00    (MMIO_BASE+0x0000B21C)

// GPIOs
#define GPFSEL1         (MMIO_BASE+0x00200004)
#define GPLEV0          (MMIO_BASE+0x00200034)
#define GPPUD           (MMIO_BASE+0x00200094)
#define GPPUDCLK0       (MMIO_BASE+0x00200098)

// PL011 UART
#define UART0_DR        (MMIO_BASE+0x00201000)
#define UART0_FR        (MMIO_BASE+0x00201018)
#define UART0_IBRD      (MMIO_BASE+0x00201024)
#define UART0_FBRD      (MMIO_BASE+0x00201028)
#define UART0_LCRH      (MMIO_BASE+0x0020102C)
#define UART0_CR        (MMIO_BASE+0x00201030)
#define UART0_ICR       (MMIO_BASE+0x00201044)

// GPU messagebox
#define MBOX_READ       (MMIO_BASE+0x0000B880)
#define MBOX_STATUS     (MMIO_BASE+0x0000B898)
#define MBOX_WRITE      (MMIO_BASE+0x0000B8A0)
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

#endif
