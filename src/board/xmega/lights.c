#include "lights.h"
#include "linearize.h"

#include <avr/io.h>

static uint16_t channel_values[3] = {};

static void apply() {
    TCC4.CTRLGSET = TC4_LUPD_bm;
    
    TCC4.CCABUF = channel_values[0];
    TCC4.CCCBUF = channel_values[1];
    TCC4.CCBBUF = channel_values[2];
    
    linearize(
        &TCC4.CCABUF,
        &TCC4.CCCBUF,
        &TCC4.CCBBUF);
    TCC4.CTRLGCLR = TC4_LUPD_bm;
}

void set_rgb(uint16_t r, uint16_t g, uint16_t b) {
    channel_values[0] = r;
    channel_values[1] = g;
    channel_values[2] = b;
    
    apply();
}

void set_channel_value(uint8_t chan, uint16_t value) {
    channel_values[chan] = value;
    
    apply();
}

uint16_t get_channel_value(uint8_t chan) {
    return channel_values[chan];
}
