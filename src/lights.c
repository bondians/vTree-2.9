#include "lights.h"
#include "linearize.h"

#include <avr/io.h>

static uint16_t channel_values[3] = {};

void init_light_subsystem() {
    // Set OC0A/OC0B
    PORTC.DIRSET |= 0b00000111;
    
    // Set up 3-channel 16-bit dual-slope pwm on PORTC 0-2
    TCC4.PER = 0xFFFF;
    
    TCC4.CTRLB = 0b110 << TC4_WGMODE_gp;
    TCC4.CTRLC = TC4_POLA_bm | TC4_POLB_bm | TC4_POLC_bm;
    TCC4.CTRLD = 0;
    TCC4.CTRLE = (0b01 << TC4_CCAMODE_gp)
               | (0b01 << TC4_CCBMODE_gp)
               | (0b01 << TC4_CCCMODE_gp);
    
    TCC4.CCA = 0;
    TCC4.CCB = 0;
    TCC4.CCC = 0;
    
    // start the timer
    TCC4.CTRLA = 0b0001 << TC4_CLKSEL_gp;
}

static void apply() {
    TCC4.CTRLGSET = TC4_LUPD_bm;
    
    TCC4.CCABUF = channel_values[0];
    TCC4.CCBBUF = channel_values[1];
    TCC4.CCCBUF = channel_values[2];
    
    linearize(
        &TCC4.CCABUF,
        &TCC4.CCBBUF,
        &TCC4.CCCBUF);
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
