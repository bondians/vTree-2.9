#include "debug.h"
#include "ir.h"
#include "lights.h"
#include <stdint.h>

#include "board.h"

#define USEC_TO_TICKS(us)   ((uint32_t) us / IR_TICK_USEC)

#define IDLE_TIMEOUT_USEC   5000    // Minimum gap between transmissions (microseconds)

// a code was received; act upon it.
static void accept(uint8_t code) {
    dprintf("Received IR code: %d\r\n", code);
    
    if (code & 0b111) return;
    
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
#define LO_THRESHOLD_TICKS(us)      (USEC_TO_TICKS(3 * us) / 4)
#define HI_THRESHOLD_TICKS(us)      (USEC_TO_TICKS(4 * us) / 3)
static uint8_t identify_mark(uint16_t ticks) {
    // 560 usec
    if (ticks < LO_THRESHOLD_TICKS( 560)) return OTHER;
    if (ticks < HI_THRESHOLD_TICKS( 560)) return BIT_MARK;
    
    // 9000 usec
    if (ticks < LO_THRESHOLD_TICKS(9000)) return OTHER;
    if (ticks < HI_THRESHOLD_TICKS(9000)) return HDR_MARK;
    
    return OTHER;
}

static uint8_t identify_space(uint16_t ticks) {
    // 560 usec
    if (ticks < LO_THRESHOLD_TICKS( 560)) return OTHER;
    if (ticks < HI_THRESHOLD_TICKS( 560)) return ZERO_SPACE;
    
    // 1600 usec
    if (ticks < LO_THRESHOLD_TICKS(1600)) return OTHER;
    if (ticks < HI_THRESHOLD_TICKS(1600)) return ONE_SPACE;
    
    // 2250 usec
    if (ticks < LO_THRESHOLD_TICKS(2250)) return OTHER;
    if (ticks < HI_THRESHOLD_TICKS(2250)) return RPT_SPACE;
    
    // 4500 usec
    if (ticks < LO_THRESHOLD_TICKS(4500)) return OTHER;
    if (ticks < HI_THRESHOLD_TICKS(4500)) return HDR_SPACE;
    
    return OTHER;
}

// parser state.
// TODO: separate states for each byte, check the parity as we go.
// maybe check the device code too, so the result can be a single byte.
enum {PARSE_IDLE, PARSE_HDR, PARSE_BODY, PARSE_RPT, PARSE_DATA = 31};

// parser for NEC code sequences.  accepts 2 basic sequences:
// repeat code: HDR_MARK RPT_SPACE BIT_MARK
// data code:   HDR_MARK HDR_SPACE (BIT_MARK (ONE_SPACE|ZERO_SPACE)){32}
static void parse(uint8_t token) {
    static uint8_t state;
    static uint8_t offset;
    static uint8_t data, data_hi;

    static const    uint8_t device_code  = 0x00;
    static          uint8_t command_code = 0xFF;
    
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
                        data_hi = data;
                    }
                    
                    if (offset == 0) {
                        if (((data_hi ^ data) == 0xff)) {
                            command_code = data_hi;
                            accept(command_code);
                        }
                        
                        state = PARSE_IDLE;
                    } else {
                        offset--;
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

// receiver states
enum {RCV_IDLE, RCV_MARK, RCV_SPACE};
static uint8_t rcv_state = RCV_IDLE;

// A watchdog that switches the state to IDLE when
// no pulse is received before the timer overflows.
// 
// This must be called if the tick counter overflows.
void ir_pin_watchdog_timeout() {
    rcv_state = RCV_IDLE;
}

// notification from the hardware that a pin change occurred
// (either rising or falling edge).
// 
// must be called _only_ when the pin changes, and passed
// the current carrier state (true = carrier detected) and
// number of ticks since the last change.
void ir_pin_changed(bool irdata, uint16_t time) {
    switch(rcv_state) {
    case RCV_IDLE:
        if (irdata) {
            // gap just ended, start recording transmission
            rcv_state = RCV_MARK;
        }
        break;
    
    case RCV_SPACE:
        parse(identify_space(time));
        rcv_state = RCV_MARK;
        break;
    
    default:
    case RCV_MARK:
        // MARK ended, record time
        parse(identify_mark(time));
        rcv_state = RCV_SPACE;
        break;
    }
}
