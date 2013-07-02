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

// a code was received; act upon it.  0xFF indicates the code was not recognized.
static void accept(uint8_t code) {
    uint8_t x = code_x(code);
    uint8_t y = code_y(code);
    
    uint8_t a, t;
    switch(y) {
        case 5: // special buttons row
            a = 0; t = 0;
            break;
        default:
            a = y*51; t = 255;
            break;
    }
    
    uint8_t r,g,b;
    switch(x) {
        default:
        case 0: // red->grn
            r = t-a; g = a; b = 0;
            break;
        case 1: // grn->blu
            r = 0; g = t-a; b = a;
            break;
        case 2: // blu->red
            r = a; g = 0; b = t-a;
            break;
        case 3: // specials
            if (y == 0) // white
                r = g = b = 255;
            else
                r = g = b = 0;
    }
    
    set_channel_value(0, b<<8);
    set_channel_value(1, g<<8);
    set_channel_value(2, r<<8);
}

// tokens we care about:
enum {
    BIT_MARK, HDR_MARK,
    ZERO_SPACE, ONE_SPACE, RPT_SPACE, HDR_SPACE,
    OTHER,
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
enum {PARSE_IDLE, PARSE_HDR, PARSE_BODY, PARSE_RPT, PARSE_DATA};
static volatile uint8_t state;
static volatile uint8_t offset;
static volatile uint32_t data;

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
                    if (offset-- == 0) {
                        accept(identify_code(data, 0xFF));
                        
                        state = PARSE_IDLE;
                    }
                    break;
                default:
                case PARSE_RPT:
                    // TODO: only process repeat shortly after seeing a normal code
                    accept(identify_code(data, 0xFF));
                    state = PARSE_IDLE;
                    break;
            }
            break;
        case ONE_SPACE:
        case ZERO_SPACE:
            if (state == PARSE_DATA) {
                data <<= 1;
                if (token == ONE_SPACE) {
                    data |= 1L;
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

ISR(TIMER0_COMPA_vect)
{
    uint8_t irdata = !(PIND & (1<<6));
    
    timer++; // One more 50us tick
    switch(rcv_state) {
        default:
        case RCV_IDLE: // In the middle of a gap
            if (irdata) {
                if (timer < GAP_TICKS) {
                    // Not big enough to be a gap.
                    timer = 0;
                } else {
                    // gap just ended, start recording transmission
                    timer = 0;
                    rcv_state = RCV_MARK;
                }
            }
            break;
        
        case RCV_MARK: // timing MARK
            if (!irdata) {   // MARK ended, record time
                parse(identify_token(irdata, timer));
                timer = 0;
                rcv_state = RCV_SPACE;
            }
            break;
        
        case RCV_SPACE: // timing SPACE
            if (irdata) { // SPACE just ended, record it
                parse(identify_token(irdata, timer));
                timer = 0;
                rcv_state = RCV_MARK;
            } else { // SPACE
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
    // set up timer to give an interrupt every 50 usec (800 cycles @ 16MHz)
    // using CTC mode, clk/8, reset at 100
    OCR0A = 100;
    TCCR0A = 0x02;
    TCCR0B = 0x02;
    
    // enable TIMER0_COMPA interrupt
    TIMSK |= 1 << OCIE0A;
}