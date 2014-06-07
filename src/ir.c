#include "ir.h"

#include "debug.h"
#include "lights.h"
#include "board.h"

#include <avr/interrupt.h>
#include <stdint.h>


#define USEC_TO_TICKS(us)   ((uint32_t) us / IR_TICK_USEC)

#define IDLE_TIMEOUT_USEC   5000    // Minimum gap between transmissions (microseconds)

// a code was received; act upon it.
static void accept(uint8_t device, uint8_t code) {
    dprintf("Received IR code: %02x:%02x\r\n", device, code);
    
    if (device != 0) return;
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
typedef uint8_t token_t;

// recognize a token by its level and duration (in ticks)
#define LO_THRESHOLD_TICKS(us)      (USEC_TO_TICKS(3 * us) / 4)
#define HI_THRESHOLD_TICKS(us)      (USEC_TO_TICKS(4 * us) / 3)
static token_t identify_mark(uint16_t ticks) {
    // 560 usec
    if (ticks < LO_THRESHOLD_TICKS( 560)) return OTHER;
    if (ticks < HI_THRESHOLD_TICKS( 560)) return BIT_MARK;
    
    // 9000 usec
    if (ticks < LO_THRESHOLD_TICKS(9000)) return OTHER;
    if (ticks < HI_THRESHOLD_TICKS(9000)) return HDR_MARK;
    
    return OTHER;
}

static token_t identify_space(uint16_t ticks) {
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

// in practice we have no trouble at all keeping up 
// with a "queue" of 1.  If we ever need it, though, 
// this can be increased. (I've tested it with a deep
// queue and lots of artificial latency)
#define IN_TOKEN_COUNT      1
static uint8_t incoming_tokens[IN_TOKEN_COUNT];
static uint8_t head = 0, count = 0;

static void received_token(token_t token) {
    if (count < IN_TOKEN_COUNT) {
        uint8_t i = (head + count) % IN_TOKEN_COUNT;
        
        incoming_tokens[i] = token;
        
        count++;
    }
}

static bool get_input_token(token_t *token) {
    if (count == 0) return false;
    
    cli();
    *token = incoming_tokens[head];
    
    head = (head + 1) % IN_TOKEN_COUNT;
    count--;
    sei();
    
    return true;
}

// parser for NEC code sequences.  accepts 2 basic sequences:
// repeat code: HDR_MARK RPT_SPACE BIT_MARK
// data code:   HDR_MARK HDR_SPACE (BIT_MARK (ONE_SPACE|ZERO_SPACE)){32}
#define EXPECT_TOKEN(t) do {PT_WAIT_UNTIL(pt, get_input_token(&token)); if (token != t) goto top;} while (0)
PT_THREAD(ir_task(struct pt *pt)) {
    static token_t token;
    
    static uint8_t bits_left;
    static uint8_t data = 0x00;
    static uint8_t device_code  = 0x00;
    static uint8_t command_code = 0x00;
    
    PT_BEGIN(pt);
    
    while(1) {
        top:
        EXPECT_TOKEN(HDR_MARK);
        
        PT_WAIT_UNTIL(pt, get_input_token(&token));
        if (token == RPT_SPACE) {
            EXPECT_TOKEN(BIT_MARK);
            // TODO: only process repeat shortly after seeing a normal code
            
            // continue to 'accept' section below
        } else if (token == HDR_SPACE) {
            bits_left = 32;
            while (bits_left) {
                EXPECT_TOKEN(BIT_MARK);
                
                PT_WAIT_UNTIL(pt, get_input_token(&token));
                if (token != ZERO_SPACE && token != ONE_SPACE) goto top;
                
                data = (data << 1) | ((token == ONE_SPACE) ? 1 : 0);
                
                bits_left--;
                if (bits_left == 24) device_code = data;
                if (bits_left == 16 && (data ^ device_code) != 0xFF) goto top;
                if (bits_left ==  8) command_code = data;
            }
        } else {
            goto top;
        }
        
        if ((data ^ command_code) == 0xFF) {
            accept(device_code, command_code);
        }
    }
    
    PT_END(pt);
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
        received_token(identify_space(time));
        rcv_state = RCV_MARK;
        break;
    
    default:
    case RCV_MARK:
        // MARK ended, record time
        received_token(identify_mark(time));
        rcv_state = RCV_SPACE;
        break;
    }
}
