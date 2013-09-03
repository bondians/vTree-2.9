#include "debug.h"
#include "ir.h"
#include "lights.h"
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

// codes on my remote are CODE(0,x<<3), where 'x' is:
// 
//  0x14 0x04 0x0c 0x1c
//  0x12 0x02 0x0a 0x1a
//  0x16 0x06 0x0e 0x1e
//  0x15 0x05 0x0d 0x1d
//  0x13 0x03 0x0b 0x1b
//  0x11 0x01 0x09 0x19
//
// and CODE(d,n), for 0 <= n < 32, is:
#define CODE(d,n)   (  (uint32_t)(d)        << 24   \
                    | ((uint32_t)(d)^0xFFL) << 16   \
                    |  (uint32_t)(n)        << 8    \
                    | ((uint32_t)(n)^0xFFL))

// recognize a code from our particular remote.
// 
// requires that the 'device' portion be 0 and the other 
// part be a multiple of 8.
static uint8_t identify_code(uint32_t code, uint8_t no_match) {
    if (code >> 16 != 0xFF) return no_match;
    uint8_t a = code;
    uint8_t b = code>>8;
    
    return (((a ^ b) == 0xFF) && !(b & 0x7)) ? b>>3 : no_match;
}

static inline uint8_t code_x(uint8_t code) {
    uint8_t raw_x = (code >> 3) & 3;
    switch(raw_x) {
        default:
        case 2: return 0;
        case 0: return 1;
        case 1: return 2;
        case 3: return 3;
    }
}

static inline uint8_t code_y(uint8_t code) {
    uint8_t raw_y = code & 7;
    switch(raw_y) {
        default:
        case 4: return 5;
        case 2: return 0;
        case 6: return 1;
        case 5: return 2;
        case 3: return 3;
        case 1: return 4;
    }
}

// a code was received; act upon it.
static void accept(uint8_t code) {
    dprintf("Received IR code: %d\r\n", code);
    
    code >>= 3;
    
    uint8_t x, y;
    switch(code & 0x7) {
        // special buttons row
        case 4: x =   0; y =   0; break;
        
        // color rows, in order
        case 2: x =   0; y = 255; break;
        case 6: x =  51; y = 204; break;
        case 5: x = 102; y = 103; break;
        case 3: x = 153; y = 102; break;
        default:
        case 1: x = 204; y =  51; break;
    }
    
    uint8_t r,g,b;
    switch((code >> 3) & 3) {
        default:
        case 2: // red->grn
            r = y; g = x; b = 0;
            break;
        case 0: // grn->blu
            r = 0; g = y; b = x;
            break;
        case 1: // blu->red
            r = x; g = 0; b = y;
            break;
        case 3: // specials
            if (code == 0x1a) // white
                r = g = b = 255;
            else
                r = g = b = 0;
    }
    
    set_rgb(r<<8, g<<8, b<<8);
}

// tokens we care about:
enum {
    ZERO_SPACE, ONE_SPACE, RPT_SPACE, HDR_SPACE,
    BIT_MARK, HDR_MARK,
    OTHER = 0xff,
};

// recognize a token by its level and duration (in ticks)
static inline uint8_t identify_token(uint8_t state, uint16_t ticks) {
    if (!state) {
        if (ticks <   8) return OTHER;
        if (ticks <  15) return BIT_MARK; // 560 usec (11.2 ticks)
        if (ticks < 135) return OTHER;
        if (ticks < 225) return HDR_MARK; // 9000 usec (180 ticks)
        return OTHER;
    } else {
        if (ticks <   8) return OTHER;
        if (ticks <  15) return ZERO_SPACE; // 560 usec (11.2 ticks)
        if (ticks <  24) return OTHER;
        if (ticks <  40) return ONE_SPACE; // 1600 usec (32 ticks)
        if (ticks <  33) return OTHER;
        if (ticks <  56) return RPT_SPACE; // 2250 usec (45 ticks)
        if (ticks <  57) return OTHER;
        if (ticks < 113) return HDR_SPACE; // 4500 usec (90 ticks)
        return OTHER;
    }
}

// parser state.
// TODO: separate states for each byte, check the parity as we go.
// maybe check the device code too, so the result can be a single byte.
enum {PARSE_IDLE, PARSE_HDR, PARSE_BODY, PARSE_RPT, PARSE_DATA = 31};
static volatile uint8_t state;
static volatile uint8_t offset;
static volatile uint8_t data, data_hi;

static const    uint8_t device_code  = 0x00;
static volatile uint8_t command_code = 0xFF;

// parser for NEC code sequences.  accepts 2 basic sequences:
// repeat code: HDR_MARK RPT_SPACE BIT_MARK
// data code:   HDR_MARK HDR_SPACE (BIT_MARK (ONE_SPACE|ZERO_SPACE)){32}
void parse(uint8_t token) {
    switch(token) {
        case HDR_MARK:
            state = PARSE_HDR;
            break;
        case HDR_SPACE:
            state = (state == PARSE_HDR) ? PARSE_BODY : PARSE_IDLE;
            break;
        case RPT_SPACE:
            state = (state == PARSE_HDR) ? PARSE_RPT : PARSE_IDLE;
            break;
        case BIT_MARK:
            switch (state) {
                case PARSE_BODY:
                    state = PARSE_DATA;
                    offset = 31;
                    break;
                case PARSE_DATA:
                    if (((offset == 24) && (data != (uint8_t)  device_code))
                     || ((offset == 16) && (data != (uint8_t) ~device_code))) {
                        state = PARSE_IDLE;
                    }
                    
                    if (offset == 8) {
                        // small space-saving shortcut;
                        // will cause false acceptance of some invalid codes
                        // but this should be exceptionally unlikely in practice.
                        data_hi = data & 0xf8;
                        // a correct alternative would be:
                        // if (data & 7) { state = PARSE_IDLE; } else { data_hi = data; }
                    }
                    
                    if (offset-- == 0) {
                        if (((data_hi ^ data) == 0xff)) {
                            command_code = data_hi;
                            accept(command_code);
                        }
                        
                        state = PARSE_IDLE;
                    }
                    break;
                default:
                case PARSE_RPT:
                    // TODO: only process repeat shortly after seeing a normal code
                    accept(command_code);
                    
                    state = PARSE_IDLE;
                    break;
            }
            break;
        case ONE_SPACE:
        case ZERO_SPACE:
            if (state == PARSE_DATA) {
                data <<= 1;
                if (token == ONE_SPACE) {
                    data |= 1;
                }
            }
            
            break;
        default:
            state = PARSE_IDLE;
            break;
    }
}

// an extremely stripped-down version of the timer interrupt from
// [https://github.com/shirriff/Arduino-IRremote] follows.

#define USECPERTICK 50      // microseconds per clock interrupt tick

#define _GAP        5000    // Minimum gap between transmissions (microseconds)
#define GAP_TICKS (_GAP/USECPERTICK)

// receiver states
enum {RCV_IDLE, RCV_MARK, RCV_SPACE};
static volatile uint8_t rcv_state   = RCV_IDLE;
static volatile uint16_t timer      = 0; // ticks since start of current state

ISR(TCC1_OVF_vect)
{
    uint8_t irdata = !(PORTA.IN & 0b00000001);
    
    timer++; // One more 50us tick
    switch(rcv_state) {
        default:
        case RCV_IDLE: // In the middle of a gap
            if (irdata) {
                if (timer >= GAP_TICKS) {
                    // gap just ended, start recording transmission
                    rcv_state = RCV_MARK;
                }
                
                timer = 0;
            }
            break;
        
        case RCV_MARK: // timing MARK
            if (!irdata) {   // MARK ended, record time
                parse(identify_token(irdata, timer));
                rcv_state = RCV_SPACE;
                
                timer = 0;
            }
            break;
        
        case RCV_SPACE: // timing SPACE
            if (irdata) { // SPACE just ended, record it
                parse(identify_token(irdata, timer));
                rcv_state = RCV_MARK;
                
                timer = 0;
            } else { // still in SPACE
                if (timer > GAP_TICKS) {
                    // big SPACE, indicates gap between codes
                    // Don't reset timer; keep counting space width
                    rcv_state = RCV_IDLE;
                }
            }
            break;
    }
}

void init_ir_subsystem() {
    // Ensure IR pin is set as input
    PORTA.DIRCLR = 0b00000001;
    PORTE.DIRCLR = 0b1000;
    //PORTE.PIN3CTRL = PORT_OPC_PULLUP_gc;
    
    // set up TCC1 to give an interrupt every 50 usec (1600 cycles @ 32MHz)
    TCC1.CTRLB = TC_WGMODE_NORMAL_gc;
    TCC1.CTRLC = 0;
    TCC1.CTRLD = 0;
    TCC1.CTRLE = 0;
    
    TCC1.PER = 1600;
    
    // enable TCC1_OVF_vect interrupt
    TCC1.INTCTRLA = TC_OVFINTLVL_LO_gc;
    
    // start the timer
    TCC1.CTRLA = TC_CLKSEL_DIV1_gc;
}