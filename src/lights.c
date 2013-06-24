#include "lights.h"

#include <avr/io.h>
#include <util/atomic.h>

// TODO: there are probably stdlib functions for these... find them
#define setBit(reg, bit, mode) ({if (mode) reg |= 1<<(bit); else reg &= ~(1<<(bit));})
#define pinMode(X, pin, mode)   setBit(DDR ## X, pin, mode)
#define setPin(X, pin, mode)    setBit(PORT ## X, pin, mode)

// channel set-points
static uint16_t chan_values[NUM_CHANNELS] = {};

typedef struct {
    uint16_t    when;
    uint8_t     what;
} frame_t;

static uint8_t frame = 0;

#define NUM_FRAMES 4
static frame_t frames[NUM_FRAMES] = {};
static uint8_t needs_update = 1;

static void update_frames() {
    uint8_t a,b,c;
    
    if (chan_values[0] < chan_values[1]) {
        if (chan_values[1] < chan_values[2]) {
            a = 0; b = 1, c = 2;
        } else {
            c = 1;
            if (chan_values[0] < chan_values[2]) {
                a = 0; b = 2;
            } else {
                a = 2; b = 0;
            }
        }
    } else {
        if (chan_values[1] < chan_values[2]) {
            a = 1;
            if (chan_values[0] < chan_values[2]) {
                b = 0; c = 2;
            } else {
                b = 2; c = 0;
            }
        } else {
            a = 2; b = 1; c = 0;
        }
    }
    
    uint8_t x = 0x38;
    frames[0].what = x;
    
    frames[1].when = chan_values[a];
    x &= ~(1<<(3+a));
    frames[1].what = x;
    
    frames[2].when = chan_values[b];
    x &= ~(1<<(3+b));
    frames[2].what = x;
    
    frames[3].when = chan_values[c];
    x &= ~(1<<(3+c));
    frames[3].what = x;
    
    for (int i = NUM_FRAMES-2; i >= 0; i--) {
        if (frames[i].when == frames[i+1].when) {
            frames[i].what = frames[i+1].what;
        }
    }
}

static void dispatch() {
    while (frame < NUM_FRAMES) {
        OCR1A = frames[frame].when;
        
        if (OCR1A > TCNT1+4) { // + a few to make sure there's time for the interrupt to fire again if necessary
            break;
        } else {
            PORTD = (PORTD & 0xC7) | frames[frame].what;
            frame++;
        }
    }
    
    if (needs_update) {
        update_frames();
        needs_update = 0;
    }
}

ISR(TIMER1_COMPA_vect) {
    dispatch();
}

ISR(TIMER1_OVF_vect) {
    frame = 0;
    dispatch();
}

void init_light_subsystem() {
    // set data directions
    pinMode(D, 3, 1);
    pinMode(D, 4, 1);
    pinMode(D, 5, 1);
    
    // set up timer
    // start timer 1 in "normal mode" (16-bit) with no clock scaling and no output-compare
    TCCR1A = 0x00;
    TCCR1B = 0x01;
    
    // TODO: need to explicitly enable interrupts?
    TIMSK = 0xC0;
}

void set_channel_value(uint8_t chan, uint16_t value) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        chan_values[chan] = value;
        needs_update = 1;
    }
}

uint16_t get_channel_value(uint8_t chan) {
    return chan_values[chan];
}
