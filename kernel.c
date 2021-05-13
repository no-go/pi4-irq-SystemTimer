#include "kernel.h"

#define FONTPADDING 10

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

unsigned char * graphics_lfb;
int graphics_width;
int graphics_height;

int hours;
int minutes;
int tenthsec;
int old_tenthsec;

void uart_send (unsigned int c) {
    while(GET32(UART0_FR)&0x20);
    PUT32(UART0_DR, c);
}

char uart_get (void) {
    char r;
    while(GET32(UART0_FR)&0x10);
    r=(char)GET32(UART0_DR);
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

// some parts from the circle lib
void dispatch (void) {
    unsigned nReg = 0;
    unsigned Pending[3];
    Pending[0] = GET32(IRQ1_PENDING_31_00);
    Pending[1] = GET32(IRQ2_PENDING_31_00);
    Pending[2] = GET32(IRQ0_PENDING_31_00) & 0xFF;

    for (nReg = 0; nReg < 3; ++nReg) {
        unsigned nPending = Pending[nReg];
        if (nPending != 0) {
            unsigned nIRQ = nReg * 32;

            do {
                if (nPending & 1) {
                    if (nIRQ == PIT_IRQ) {
                        //uart_send('t');
                        tenthsec++;
                        PUT32(PIT_Compare3, GET32(PIT_LOW) + 100000); // next in 0.1sec
                        PUT32(PIT_STATUS, 1 << PIT_MASKBIT); // clear IRQ in System Timer chip
                        // clear the pending ?
                        return;
                    }
                }

                nPending >>= 1;
                nIRQ++;
            } while (nPending != 0);
        }
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
    PUT32(UART0_CR, 0); // turn off UART3
    
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

    // map UART0 to GPIO pins
    register unsigned int r = GET32(GPFSEL1);
    r &= ~((7<<12)|(7<<15));    // 111 to gpio14, gpio15 (mask/clear)
    r |= (4<<12)|(4<<15);       // 100 = alt0
    PUT32(GPFSEL1, r);

    // remove pullup or pulldown
    PUT32(GPPUD,0);
    for(r=0;r<150;r++) asm volatile("nop");
    PUT32(GPPUDCLK0,(1<<14)|(1<<15));
    for(r=0;r<150;r++) asm volatile("nop");
    PUT32(GPPUDCLK0,0);

    // clear interrupts in PL011 Chip
    PUT32(UART0_ICR, 0x7FF);
    // Divider = 3000000MHz/(16 * 115200) = 1.627 = ~1.
    // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
    PUT32(UART0_IBRD, 1);       // 115200 baud = 1 40
    PUT32(UART0_FBRD, 40);
    
    // Enable FIFO and 8 bit data transmission (1 stop bit, no parity)
    PUT32(UART0_LCRH, (1 << 6) | (1 << 5) | (1 << 4));
    // enable Tx, Rx, FIFO 
    PUT32(UART0_CR, (1 << 9) | (1 << 8) | (1 << 0));
}

void setHHMM (int d1, int d2) {
    hours = d1;
    minutes = d2;
    
    rect(0,100,640,300, C_black, C_black);
    
    if (d1<10) {
        myFont(58, 120, d1, 140, C_white, C_black);
    } else {
        myFont(10, 120, d1, 140, C_white, C_black);
    }
    if (d2<10) {
        myFont(245, 120, 0, 140, C_white, C_black);
        myFont(325, 120, d2, 140, C_white, C_black);
    } else {
        myFont(245, 120, d2, 140, C_white, C_black);
    }
    tenthsec = 0;
    old_tenthsec = 0;
}

void main () {    
    // interrupts disable
    asm ("cpsid i");

    // PIT plugin (System Timer) in Interrupt Controller
    unsigned int dummy = GET32(IRQ1_SET_31_00);
    dummy |= 1 << PIT_IRQ;
    PUT32(IRQ1_SET_31_00, dummy);
    
    // inital first IRQ in 5sec
    PUT32(PIT_Compare3, 5000000);
    PUT32(PIT_STATUS, 1 << PIT_MASKBIT);

    // IRQs enable
    asm ("cpsie i"); 

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
        
        if ((GET32(UART0_FR)&0x10) == 0) {
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
                myFont(245, 120, 0, 140, C_black, C_black);
                myFont(325, 120, minutes, 140, C_black, C_black);
            } else {
                myFont(245, 120, minutes, 140, C_black, C_black);
            }

            minutes++;
            tenthsec = 0;
            
            if (minutes < 10) {
                myFont(245, 120, 0, 140, C_white, C_black);
                myFont(325, 120, minutes, 140, C_white, C_black);
            } else {
                if (minutes < 60) myFont(245, 120, minutes, 140, C_white, C_black);
            }
        }
        
        if (minutes == 60) {
            colorbar = C_black;
            if (hours < 10) {
                myFont(85, 120, hours, 140, C_black, C_black);
            } else {
                myFont(10, 120, hours, 140, C_black, C_black);
            }
            myFont(245, 120, 59, 140, C_black, C_black);
            hours++;
            minutes = 0;
            myFont(245, 120, 0, 140, C_white, C_black);
            myFont(325, 120, 0, 140, C_white, C_black);

            if (hours == 24) {
                myFont(10, 120, 23, 140, C_black, C_black);
                hours = 0;
                myFont(85, 120, hours, 140, C_white, C_black);
            } else {
                if (hours < 10) {
                    myFont(85, 120, hours, 140, C_white, C_black);
                } else {
                    myFont(10, 120, hours, 140, C_white, C_black);
                }
            }
        }
        
        if (tenthsec/10 != old_tenthsec) {
            pos = 480;
            if (tenthsec/10 == 0) {
                myFont(pos, 120, 59, 140, C_black, C_black);
            } else if (tenthsec/10 < 10) {
                pos = myFont(pos, 120, 0, 140, C_white, C_black);
                pos += FONTPADDING;
                myFont(pos, 120, old_tenthsec, 140, C_black, C_black);
                myFont(pos, 120, tenthsec/10, 140, C_white, C_black);
            } else if (tenthsec/10 == 10) {
                pos = myFont(pos, 120, 0, 140, C_black, C_black);
                pos += FONTPADDING;
                myFont(pos, 120, old_tenthsec, 140, C_black, C_black);
                myFont(480, 120, tenthsec/10, 140, C_white, C_black);
            } else {
                myFont(pos, 120, old_tenthsec, 140, C_black, C_black);
                myFont(pos, 120, tenthsec/10, 140, C_white, C_black);
            }
            if (old_tenthsec%30 == 0) {
                colorbar += 0x55;
            }
            old_tenthsec = tenthsec/10;
        }
        line(20, 290, 20+tenthsec, 290, colorbar);
        line(20, 291, 20+tenthsec, 291, colorbar);
        line(20, 292, 20+tenthsec, 292, colorbar);
    }
}
