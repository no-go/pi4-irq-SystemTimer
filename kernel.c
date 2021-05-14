#include "kernel.h"

#define FONTPADDING 40
#define FONTSIZE    300

// a properly aligned message buffer with: 9x4 byte long mes-
// sage setting feature PL011 UART Clock to 3MHz (and some TAGs and
// sizes of parameter in message)
volatile unsigned int __attribute__((aligned(16))) mbox[9] = { 9*4, 0, 0x38002, 12, 8, 2, 3000000, 0, 0 };

// better: reuse mbox
volatile unsigned int __attribute__((aligned(16))) graphics_message[31] = {
    31*4, 0,
    0x48003, 8,8, 1920,1080,
    0x48004, 8,8, 1920,1080,
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
int old_tenthsec;

void uart_send (unsigned int c) {
    while(GET32(UART3_FR)&0x20);
    PUT32(UART3_DR, c);
}

char uart_get (void) {
    char r;
    while(GET32(UART3_FR)&0x10);
    r=(char)GET32(UART3_DR);
    return r=='\r'?'\n':r;
}

void pixel (int x, int y, int col) {
    if (x<0 || x >= graphics_width || y<0 || y >= graphics_height) return;

    int offs = (y * 4 * graphics_width) + (x * 4); // 4 byte = 32bit
    *((unsigned int*)(graphics_lfb + offs)) = col;
}

void line (int x,  int y, int tox, int toy, int col) {
    int sx = x<tox ? 1 : -1;
    int sy = y<toy ? 1 : -1;

    int dx =  sx * (tox-x);
    int dy =  sy * (y-toy);
    int err = dx+dy, e2;

    while(1) {
        pixel(x, y, col);
        
        if (x==tox && y==toy) break;
        e2 = 2*err;
        if (e2 > dy) { err += dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

// if fillCol == 0 not filled
void rect (int x,  int y, int tox,  int toy,  int col, int fillCol) {
    int i,j;
    if (fillCol != 0) {
        if (tox < x) { i = x; x = tox; tox = i; }
        if (toy < y) { i = y; y = toy; toy = i; }

        for (i=x; i<=tox; ++i) {
            for (j=y; j<=toy; ++j) {
                pixel(i, j, fillCol);
            }
        }
    }
    line(x,y,     tox,y,   col);
    line(x,y,       x,toy, col);
    line(x,toy,   tox,toy, col);
    line(tox,toy, tox,y,   col);
}

// if fillCol == 0 not filled
void circle (int x0, int y0, int radius, int col, int fillCol) {
    int x = radius;
    int y = 0;
    int err = 0;
 
    while (x >= y) {
        if (fillCol != 0) {
           line(x0 - y, y0 + x, x0 + y, y0 + x, fillCol);
           line(x0 - x, y0 + y, x0 + x, y0 + y, fillCol);
           line(x0 - x, y0 - y, x0 + x, y0 - y, fillCol);
           line(x0 - y, y0 - x, x0 + y, y0 - x, fillCol);
        }
        pixel(x0 - y, y0 + x, col);
        pixel(x0 + y, y0 + x, col);
        pixel(x0 - x, y0 + y, col);
        pixel(x0 + x, y0 + y, col);
        pixel(x0 - x, y0 - y, col);
        pixel(x0 + x, y0 - y, col);
        pixel(x0 - y, y0 - x, col);
        pixel(x0 + y, y0 - x, col);

        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }

        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

/*
 * print a number >=0 at position x,y with height (=he).
 * the number is a line/circle construct. you get the next pos
 * as return value.
 */
int myFont (int x, int y, int b, int he, int col, int fillCol) {    
    if (b == 0) {
      circle(x+he/4, y+3*he/4, he/4, col, fillCol);
      return x+2+he/2;
    } else if (b == 1) {
      line(x,y+5,x+5,y, col);
      line(x+5,y,x+5,y+he, col);
      return x+8;
    } else if (b == 2) {
      circle(x+he/4, y-3+3*he/4, he/4, col, fillCol);
      rect(x,y,x+he/4,y+he, fillCol, fillCol);
      line(x+he/4,y+he-3,x+he/2,y+he, col);
      return x+2+he/2;
    } else if (b == 3) {
      circle(x+he/4, y+1*he/4, he/4, col, fillCol);
      circle(x+he/4, y+3*he/4, he/4, col, fillCol);
      rect(x,y,x+he/4,y+he, fillCol, fillCol);
      return x+2+he/2;
    } else if (b == 4) {
      line(x,y+he/2,x+he/2,y, col);
      line(x,y+he/2,x+he/2,y+he/2, col);
      line(x+he/2,y,x+he/2,y+he, col);
      return x+2+he/2;   
    } else if (b == 5) {
      line(x+he/4,y,x+he/2,y, col);
      line(x+he/4,y,x+he/4,y+he/2, col);
      circle(x+he/4, y+3*he/4, he/4, col, fillCol);
      rect(x,y,x-1+he/4,y+he, fillCol, fillCol);
      return x+2+he/2;   
    } else if (b == 6) {
      line(x,y-2+3*he/4,x+he/2,y, col);
      circle(x+he/4, y+3*he/4, he/4, col, fillCol);
      return x+2+he/2;   
    } else if (b == 7) {
      line(x,y+3,x+he/2,y, col);
      line(x+he/2,y,x,y+he, col);
      line(x+3,y+he/2,x+he/2-1,y+he/2, col);
      return x+2+5;  
    } else if (b == 8) {
      circle(x+he/4, y+1*he/4, he/4, col, fillCol);
      circle(x+he/4, y+3*he/4, he/4, col, fillCol);
      return x+2+he/2;
    } else if (b == 9) {
      circle(x+he/4, y+1*he/4, he/4, col, fillCol);
      line (x+he/2, y+1*he/4, x+he/2, y+he, col);
      return x+2+he/2;
    }
    
    int t = b/10;
    int pos = myFont(x, y, t, he, col, fillCol);
    t *= 10;
    b = b - t;
    return myFont(pos+FONTPADDING, y, b, he, col, fillCol);
}
  
void dispatch (void) {
    // get one of the pending "Shared Peripheral Interrupt"
    unsigned int spi = GET32(GICC_ACK);
    
    while (spi != GIC_SPURIOUS) { // loop until no SPIs are pending on GIC
        if (spi == PIT_SPI) {
            //uart_send('t');
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
        }
    }
}

// initialize PL011 UART3 on GPIO4 and 5
void uart_init (void) {
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
    r &= ~((7<<12)|(7<<15));    // 111 to gpio4, gpio5 (mask/clear)
    r |= (3<<12)|(3<<15);       // 011 = alt4
    PUT32(GPFSEL0, r);

    // remove pullup or pulldown
    r = GET32(GPPUPPDN0);
    r &= ~((3<<8)|(3<<10));     // 11 to gpio4, gpio5 (mask/clear)
    r |= (0<<8)|(0<<10);        // 00 = no pullup or down
    PUT32(GPPUPPDN0, r);

    // clear interrupts in PL011 Chip
    PUT32(UART3_ICR, 0x7FF);
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

void setHHMM (int d1, int d2) {
    hours = d1;
    minutes = d2;
    
    rect(0,FONTSIZE-2,graphics_width,FONTSIZE*2+4, C_black, C_black);
    
    if (d1<10) {
        myFont(2*FONTPADDING+FONTSIZE, FONTSIZE, d1, FONTSIZE, C_white, C_black);
    } else {
        myFont(FONTPADDING, FONTSIZE, d1, FONTSIZE, C_white, C_black);
    }
    if (d2<10) {
        myFont(graphics_width/2 - FONTPADDING/2 - FONTSIZE/2, FONTSIZE, 0, FONTSIZE, C_white, C_black);
        myFont(graphics_width/2 + FONTPADDING/2, FONTSIZE, d2, FONTSIZE, C_white, C_black);
    } else {
        myFont(graphics_width/2 - FONTPADDING/2 - FONTSIZE/2, FONTSIZE, d2, FONTSIZE, C_white, C_black);
    }
    tenthsec = 0;
    old_tenthsec = 0;
}

void main () {    
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
    uart_send('s');
    uart_send('e');
    uart_send('t');
    uart_send(' ');
    uart_send('v');
    uart_send('i');
    uart_send('a');
    uart_send(' ');
    uart_send('H');
    uart_send('H');
    uart_send(':');
    uart_send('M');
    uart_send('M');
    uart_send('\r');
    uart_send('\n');

    graphics_init();
    
    // the main loop ---------------------------------
    
    setHHMM(23,58);

    int colorbar = C_red;
    int pos = 0;
    char cmdline[10] = "         ";
    int cmdi = 0;
    
    while (1) {
        asm ("wfi"); // cool. core0 just wait until interrupt comes
        
        if ((GET32(UART3_FR)&0x10) == 0) {
            cmdline[cmdi] = uart_get();
            uart_send(cmdline[cmdi]);
            if (cmdline[cmdi] == '\n') {
                hours = cmdline[0] - '0';
                hours *= 10;
                hours += cmdline[1] - '0';
                
                minutes = cmdline[3] - '0';
                minutes *= 10;
                minutes += cmdline[4] - '0';
                
                setHHMM(hours, minutes);
                cmdi = 0;
                uart_send('\r');
            } else {
                cmdi = (cmdi+1)%10;
            }
            
        }

        if (tenthsec >= 600) {
            if (minutes < 10) {
                myFont(graphics_width/2 - FONTPADDING/2 - FONTSIZE/2, FONTSIZE, 0, FONTSIZE, C_black, C_black);
                myFont(graphics_width/2 + FONTPADDING/2, FONTSIZE, minutes, FONTSIZE, C_black, C_black);
            } else {
                myFont(graphics_width/2 - FONTPADDING/2 - FONTSIZE/2, FONTSIZE, minutes, FONTSIZE, C_black, C_black);
            }

            minutes++;
            tenthsec = 0;
            
            if (minutes < 10) {
                myFont(graphics_width/2 - FONTPADDING/2 - FONTSIZE/2, FONTSIZE, 0, FONTSIZE, C_white, C_black);
                myFont(graphics_width/2 + FONTPADDING/2, FONTSIZE, minutes, FONTSIZE, C_white, C_black);
            } else {
                if (minutes < 60) myFont(graphics_width/2 - FONTPADDING/2 - FONTSIZE/2, FONTSIZE, minutes, FONTSIZE, C_white, C_black);
            }
        }
        
        if (minutes == 60) {
            colorbar = C_black;
            if (hours < 10) {
                myFont(2*FONTPADDING+FONTSIZE, FONTSIZE, hours, FONTSIZE, C_black, C_black);
            } else {
                myFont(FONTPADDING, FONTSIZE, hours, FONTSIZE, C_black, C_black);
            }
            myFont(graphics_width/2 - FONTPADDING/2 - FONTSIZE/2, FONTSIZE, 59, FONTSIZE, C_black, C_black);
            hours++;
            minutes = 0;
            myFont(graphics_width/2 - FONTPADDING/2 - FONTSIZE/2, FONTSIZE, 0, FONTSIZE, C_white, C_black);
            myFont(graphics_width/2 + FONTPADDING/2, FONTSIZE, 0, FONTSIZE, C_white, C_black);

            if (hours == 24) {
                myFont(FONTPADDING, FONTSIZE, 23, FONTSIZE, C_black, C_black);
                hours = 0;
                myFont(2*FONTPADDING+FONTSIZE, FONTSIZE, hours, FONTSIZE, C_white, C_black);
            } else {
                if (hours < 10) {
                    myFont(2*FONTPADDING+FONTSIZE, FONTSIZE, hours, FONTSIZE, C_white, C_black);
                } else {
                    myFont(FONTPADDING, FONTSIZE, hours, FONTSIZE, C_white, C_black);
                }
            }
        }
        
        if (tenthsec/10 != old_tenthsec) {
            pos = graphics_width - 2*FONTPADDING - FONTSIZE;
            if (tenthsec/10 == 0) {
                myFont(pos, FONTSIZE, 59, FONTSIZE, C_black, C_black);
            } else if (tenthsec/10 < 10) {
                pos = myFont(pos, FONTSIZE, 0, FONTSIZE, C_white, C_black);
                pos += FONTPADDING;
                myFont(pos, FONTSIZE, old_tenthsec, FONTSIZE, C_black, C_black);
                myFont(pos, FONTSIZE, tenthsec/10, FONTSIZE, C_white, C_black);
            } else if (tenthsec/10 == 10) {
                pos = myFont(pos, FONTSIZE, 0, FONTSIZE, C_black, C_black);
                pos += FONTPADDING;
                myFont(pos, FONTSIZE, old_tenthsec, FONTSIZE, C_black, C_black);
                myFont(graphics_width - 2*FONTPADDING - FONTSIZE, FONTSIZE, tenthsec/10, FONTSIZE, C_white, C_black);
            } else {
                myFont(pos, FONTSIZE, old_tenthsec, FONTSIZE, C_black, C_black);
                myFont(pos, FONTSIZE, tenthsec/10, FONTSIZE, C_white, C_black);
            }
            if (old_tenthsec%20 == 0) {
                colorbar += 0x23261;
                colorbar |= 0xFF000000;
            }
            old_tenthsec = tenthsec/10;
        }
        // start, end and scaling are hardcode on width
        line(60, FONTSIZE*2 +2*FONTPADDING,   60+3*tenthsec, FONTSIZE*2 +2*FONTPADDING, colorbar);
        line(60, FONTSIZE*2 +2*FONTPADDING+1, 60+3*tenthsec, FONTSIZE*2 +2*FONTPADDING+1, colorbar);
        line(60, FONTSIZE*2 +2*FONTPADDING+2, 60+3*tenthsec, FONTSIZE*2 +2*FONTPADDING+2, colorbar);
    }
}
