#include "lights.h"
#include "timing.h"

#include <avr/io.h>
#include <util/atomic.h>

// TODO: there are probably stdlib functions for these... find them
#define setBit(reg, bit, mode) ({if (mode) reg |= 1<<(bit); else reg &= ~(1<<(bit));})
#define pinMode(X, pin, mode)   setBit(DDR ## X, pin, mode)
#define setPin(X, pin, mode)    setBit(PORT ## X, pin, mode)

// channel set-points
static volatile uint8_t chan_init = 0;
static uint16_t chan_values[NUM_CHANNELS] = {};

static void all_on() {
    PORTB = (PORTB & 0xE3) | chan_init;
}

static event_t chan_on_event = {
    when:   TIMING_OFFSET-1,
    what:   &all_on,
};

static void ch0_off() {
    setPin(B, 2, 0);
}

static void ch1_off() {
    setPin(B, 3, 0);
}

static void ch2_off() {
    setPin(B, 4, 0);
}

static event_t chan_off_events[3] = {
    { when: 0, what: &ch0_off},
    { when: 0, what: &ch1_off},
    { when: 0, what: &ch2_off},
};

static void on_cycle_start();
static event_t cycle_start = {
    when: 0,
    what: &on_cycle_start,
};

static void on_cycle_start() {
    register_event(&cycle_start, 0);
    
    register_event(&chan_on_event, 1);
    for (int i = 0; i < 3; i++) {
        chan_off_events[i].when = chan_values[i];
        register_event(&chan_off_events[i], 1);
    }
}

void init_light_subsystem() {
    // set data directions
    pinMode(B, 2, 1);
    pinMode(B, 3, 1);
    pinMode(B, 4, 1);
    
    // set up timer (pick timer and rate...)
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        register_event(&cycle_start, 0);
    }
}

void set_channel_value(uint8_t chan, uint16_t value) {
    chan_values[chan] = (value >= MAX_LEVEL ? MAX_LEVEL : value) + TIMING_OFFSET + VALUE_OFFSET;
    setBit(chan_init, chan + 2, value > VALUE_OFFSET);
}

uint16_t get_channel_value(uint8_t chan) {
    return chan_values[chan] - TIMING_OFFSET;
}
