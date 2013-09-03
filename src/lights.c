#include "lights.h"
#include "linearize.h"

#include <avr/io.h>

static uint16_t channel_values[3] = {};

void init_light_subsystem() {
    // Set OC0A/OC0B
    PORTC.DIRSET |= 0b00000111;
    
    // Set up 3-channel 16-bit dual-slope pwm on PORTC 0-2
    TCC0.PER = 0xFFFF;
    
    TCC0.CTRLB = TC_WGMODE_DSBOTH_gc | TC0_CCAEN_bm | TC0_CCBEN_bm | TC0_CCCEN_bm;
    TCC0.CTRLC = 0;
    TCC0.CTRLD = 0;
    TCC0.CTRLE = 0;
    
    TCC0.CCA = 0;
    TCC0.CCB = 0;
    TCC0.CCC = 0;
    
    // start the timer
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
}

static void apply() {
    TCC0.CTRLFSET = TC0_LUPD_bm;
    
    TCC0.CCABUF = channel_values[0];
    TCC0.CCBBUF = channel_values[1];
    TCC0.CCCBUF = channel_values[2];
    
    linearize(
        &TCC0.CCABUF,
        &TCC0.CCBBUF,
        &TCC0.CCCBUF);
    TCC0.CTRLFCLR = TC0_LUPD_bm;
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
