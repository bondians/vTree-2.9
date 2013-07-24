#include "dmx512.h"
#include "lights.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define BUF_SZ (2*NUM_CHANNELS)
static volatile uint16_t address;
static volatile uint8_t buf[BUF_SZ];

enum {IDLE, CHAN_DATA, BREAK};
static volatile uint8_t state = IDLE;
static volatile int16_t offset;

static inline void process_frame() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        set_channel_value(i, buf[2*i] | (buf[2*i+1]<<8));
    }
}

ISR(USART_RX_vect) {
    // read status and byte immediately
    uint8_t status  = UCSRA;
    uint8_t in_byte = UDR;
    
    if (status & (1<<FE)) {
        state = BREAK;
    } else switch (state) {
        case BREAK:
            // first successful byte; if it's a 0, start reading chan data
            state = in_byte ? IDLE : CHAN_DATA;
            offset = -address;
            break;
        case CHAN_DATA:
            if (offset >= 0) {
                buf[offset] = in_byte;
            }
            
            if (++offset >= BUF_SZ) {
                state = IDLE;
                process_frame();
            }
            
            break;
    }
}

static inline void set_rs485_dir(uint8_t tx) {
    if (tx) {
        PORTD |= 1<<2;
    } else {
        PORTD &= ~(1<<2);
    }
}

void init_dmx512_subsystem(uint16_t addr) {
    DDRD |= 1<<2;
    set_rs485_dir(0);
    set_dmx512_address(addr);

    #define BAUD 250000
    #include <util/setbaud.h>
    UBRRH = UBRRH_VALUE;
    UBRRL = UBRRL_VALUE;
    #if USE_2X
        UCSRA = (1 << U2X);
    #else
        UCSRA = 0;
    #endif
    UCSRB |= (1 << RXCIE) | (1 << RXEN);
    UCSRC = ((1<<USBS) | (0<<UPM0) | (3<<UCSZ0)); // 8N2
}

void set_dmx512_address(uint16_t addr) {
    address = addr;
}
