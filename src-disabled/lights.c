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

#define NUM_FRAMES (NUM_CHANNELS + 1)
static frame_t frames[NUM_FRAMES] = {};
static uint8_t needs_update = 1;

static void update_frames() {
    uint8_t order[3] = {0,1,2};
    
    if (chan_values[0] < chan_values[1]) {
        if (chan_values[1] < chan_values[2]) {
            order[0] = 0; order[1] = 1, order[2] = 2;
        } else {
            order[2] = 1;
            if (chan_values[0] < chan_values[2]) {
                order[0] = 0; order[1] = 2;
            } else {
                order[0] = 2; order[1] = 0;
            }
        }
    } else {
        if (chan_values[1] < chan_values[2]) {
            order[0] = 1;
            if (chan_values[0] < chan_values[2]) {
                order[1] = 0; order[2] = 2;
            } else {
                order[1] = 2; order[2] = 0;
            }
        } else {
            order[0] = 2; order[1] = 1; order[2] = 0;
        }
    }
    
    uint8_t x = 0x00;

    frames[3].what = x;
    frames[3].when = chan_values[order[2]];
    
    x |= (8<<order[2]);
    frames[2].what = x;
    frames[2].when = chan_values[order[1]];
    
    x |= (8<<order[1]);
    frames[1].what = x;
    frames[1].when = chan_values[order[0]];
    
    x |= (8<<order[0]);
    frames[0].what = x;
    
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
