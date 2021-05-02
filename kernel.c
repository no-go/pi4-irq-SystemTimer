#include "kernel.h"

// a properly aligned message buffer with: 9x4 byte long mes-
// sage setting feature PL011 UART Clock to 3MHz (and some TAGs and
// sizes of parameter in message)
volatile unsigned int __attribute__((aligned(16))) mbox[9] = { 9*4, 0, 0x38002, 12, 8, 2, 3000000, 0, 0 };

// better: reuse mbox
volatile unsigned int __attribute__((aligned(16))) graphics_message[31] = {
    31*4, 0,
    0x48003, 8,8, 640,480,
    0x48004, 8,8, 640,480,
    0x48009, 8,8, 0,0,
    0x48005, 4,4, 32, // 32bits per pixel
    0x48006, 4,4, 1, // 1 = ARGB
    0x40001, 8,8, 4096,0, // get pointer addr
    0
};

unsigned long _regs[37];
unsigned char * graphics_lfb;
int graphics_width;
int graphics_height;

int hours;
int minutes;
int tenthsec;

void uart_send (unsigned int c) {
    /* wait until we can send */
    do{asm volatile("nop");}while(GET32(UART3_FR)&0x20);
    /* write the character to the buffer */
    PUT32(UART3_DR, c);
}

void graphics_set (int x, int y, int white) {
    if (x<0 || x >= graphics_width || y<0 || y >= graphics_height) return;

    int offs = (y * 4 * graphics_width) + (x * 4); // 4 byte = 32bit
    *((unsigned int*)(graphics_lfb + offs)) = (white==1 ? 0xFFFFFFFF : 0xFF000000);
}

void dispatch (void) {
    // get one of the pending "Shared Peripheral Interrupt"
    unsigned int spi = GET32(GICC_ACK);
    
    while (spi != GIC_SPURIOUS) { // loop until no SPIs are pending on GIC
        if (spi == PIT_SPI) {
            uart_send('t');
            tenthsec++;
            PUT32(PIT_Compare3, GET32(PIT_LOW) + 100000); // next in 0.1sec
            PUT32(PIT_STATUS, 1 << PIT_MASKBIT); // clear IRQ in System Timer chip
        }
        // clear the pending
        PUT32(GICC_EOI, spi);
        // get the next
        spi = GET32(GICC_ACK);
    }
}

void graphics_init (void) {
    // 28-bit address (MSB) and 4-bit value (LSB)
    unsigned int req = (((unsigned int)((long)&graphics_message)& ~0xF) | 0x8);

    // wait until we can write to the mailbox
    while(GET32(MBOX_STATUS) & MBOX_FULL);
    
    // write the address of our message to the mailbox with channel identifier
    PUT32(MBOX_WRITE, req);
    
    // Is there a reply?
    while (GET32(MBOX_STATUS) & MBOX_EMPTY);

    // Is it a reply to our message?
    if (req == GET32(MBOX_READ)) {
        // Is it successful?
        if (graphics_message[1] == MBOX_RESPONSE) {
            graphics_message[28] &= 0x3FFFFFFF;
            graphics_width = graphics_message[5];
            graphics_height = graphics_message[6];
            graphics_lfb = (unsigned char *) ( (long) graphics_message[28] );
            uart_send('g');
        }
    }
}

// initialize PL011 UART3 on GPIO4 and 5
void uart_init (void) {
    unsigned int i;

    PUT32(UART3_CR, 0); // turn off UART3
    
    // set up clock for consistent divisor values
    unsigned int req = (((unsigned int)((long)&mbox)& ~0xF) | 0x8);
    // wait until we can write to the mailbox
    do{asm volatile("nop");}while(GET32(MBOX_STATUS) & MBOX_FULL);
    // write the address of our message to the mailbox with channel identifier
    PUT32(MBOX_WRITE, req);
    // now wait for the response
    while (
        ( GET32(MBOX_STATUS) & MBOX_EMPTY ) || ( GET32(MBOX_READ) != req )
    ) {}

    // map UART3 to GPIO pins
    register unsigned int r = GET32(GPFSEL0);
    r &= ~((7<<12)|(7<<15));    // 000 to gpio4, gpio5
    r |= (3<<12)|(3<<15);       // 011 = alt4
    PUT32(GPFSEL0, r);
    PUT32(GPPUD, 0);                 // enable pins 4 and 5
    
    i=350; while(i--) { asm volatile("nop"); } // a short "wait"
    PUT32(GPPUDCLK0, (1<<14)|(1<<15));
    i=350; while(i--) { asm volatile("nop"); } // a short "wait"
    PUT32(GPPUDCLK0, 0);        // flush GPIO setup

    PUT32(UART3_ICR, 0x7FF);    // clear interrupts in PL011 Chip
    
    // Divider = 3000000MHz/(16 * 115200) = 1.627 = ~1.
    // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
    PUT32(UART3_IBRD, 1);       // 115200 baud = 1 40
    PUT32(UART3_FBRD, 40);
    
    // Enable FIFO and 8 bit data transmission (1 stop bit, no parity)
    PUT32(UART3_LCRH, (1 << 6) | (1 << 5) | (1 << 4));
    // enable Tx, Rx, FIFO 
    PUT32(UART3_CR, (1 << 9) | (1 << 8) | (1 << 0));
}

// initialize GIC-400
void gic_init (void) {
    unsigned int i;
    
    // disable Distributor and CPU interface
    PUT32(GICD_CTLR, 0);
    PUT32(GICC_CTLR, 0);

    // disable, acknowledge and deactivate all interrupts
    for (i = 0; i < (GIC_IRQS/32); ++i) {
        PUT32(GICD_DISABLE     + 4*i, ~0);
        PUT32(GICD_PEND_CLR    + 4*i, ~0);
        PUT32(GICD_ACTIVE_CLR  + 4*i, ~0);
    }

    // direct all interrupts to core 0 (=01) with default priority a0
    for (i = 0; i < (GIC_IRQS/4); ++i) {
        PUT32(GICD_TARGET + 4*i, 0x01010101);
        PUT32(GICD_PRIO   + 4*i, 0xa0a0a0a0);
    }

    // config all interrupts to level triggered
    for (i = 0; i < (GIC_IRQS/16); ++i) {
        PUT32(GICD_CFG + 4*i, 0);
    }

    // enable Distributor
    PUT32(GICD_CTLR, 1);

    // set Core0 interrupts mask theshold prio to F0, to react on higher prio a0
    PUT32(GICC_PRIO, 0xF0);
    // enable CPU interface
    PUT32(GICC_CTLR, 1);
}

void main () {    
    hours = 0;
    minutes = 0;
    tenthsec = 0;

    // interrupts off/mask
    asm ("msr daifset, #2");

    gic_init();

    // PIT plugin (System Timer)
    PUT32(GICD_ENABLE + 4 * (PIT_SPI / 32), 1 << (PIT_SPI % 32));
    PUT32(PIT_Compare3, 12000000); // inital first IRQ in 12sec
    PUT32(PIT_STATUS, 1 << PIT_MASKBIT);

    // IRQs on
    asm ("msr daifclr, #2"); 

    uart_init();
    
    // welcome to uart !!!
    uart_send('b');
    uart_send('o');
    uart_send('o');
    uart_send('\r');
    uart_send('\n');

    graphics_init();
    
    // the main loop ---------------------------------
    int colorbar = 1;
    while (1) {
        asm ("wfi"); // cool. core0 just wait until interrupt comes

        if (tenthsec >= 600) {
            minutes++;
            tenthsec = 0;
            colorbar = (colorbar==1? 0 : 1);
        }
        
        if (minutes == 60) {
            hours++;
            minutes = 0;
        }
        
        if (hours == 24) {
            hours = 0;
        }
        
        graphics_set(20+tenthsec, 400, colorbar);
        graphics_set(20+tenthsec, 401, colorbar);
        graphics_set(20+tenthsec, 402, colorbar);
        
        //uart_send('m');
    }
}
