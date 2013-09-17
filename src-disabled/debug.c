#include "debug.h"

#ifdef DEBUG

#include <avr/interrupt.h>
#include <util/atomic.h>

// must be power of 2
#define OUTBUF_SIZE 128

static int start = 0, count = 0;
static char output_buffer[OUTBUF_SIZE];

static void queue_byte();

// return 0 on success, anything else on failure
static int put_dbg(char c, FILE *f) {
    int result = 1;
    
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (count < OUTBUF_SIZE) {
            output_buffer[(start + count) & (OUTBUF_SIZE-1)] = c;
            count++;
            result = 0;
            
            if (USARTD1.STATUS & USART_DREIF_bm) {
                queue_byte();
            }
        }
    }
    
    return result;
}

static int get_dbg() {
    // not atomic; should only be called within serial tx ISR
    if (count) {
        count--;
        uint8_t c = output_buffer[start];
        start = (start + 1) & (OUTBUF_SIZE - 1);
        return c;
    } else {
        return -1;
    }
}

static FILE _dbg = FDEV_SETUP_STREAM(put_dbg, NULL, _FDEV_SETUP_WRITE);
FILE *dbg = NULL;

ISR(USARTD1_DRE_vect) {
    queue_byte();
}

static void queue_byte() {
    int c = get_dbg();
    if (c >= 0) {
        //USARTD1.CTRLB |= USART_TXEN_bm;
        USARTD1.CTRLA |= USART_DREINTLVL_LO_gc;
        
        USARTD1.DATA = c;
    } else {
        //USARTD1.CTRLB &= ~USART_TXEN_bm;
        USARTD1.CTRLA &= ~USART_DREINTLVL_gm;
    }
}

void init_debug_subsystem() {
    // 115200 baud
    #define BSEL    131
    #define BSCALE  (-3)
    #define USE_2X  0
    
    USARTD1.CTRLA = 0;
    USARTD1.CTRLB = USE_2X ? USART_CLK2X_bm : 0;
    USARTD1.CTRLC = USART_CMODE_ASYNCHRONOUS_gc 
                  | USART_PMODE_DISABLED_gc
                  | USART_CHSIZE_8BIT_gc; // 8N1
    
    USARTD1.BAUDCTRLA = BSEL;
    USARTD1.BAUDCTRLB = ((BSEL >> 8) & 0x0F) | (BSCALE << 4);
    
    // enable transmitter
    PORTD.DIRSET = 0b10000000;
    PORTD.OUTSET = 0b10000000;
    USARTD1.CTRLB |= USART_TXEN_bm;
    
    dbg = &_dbg;
}

#endif
