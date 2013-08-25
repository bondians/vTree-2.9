#include "lights.h"

#include <avr/io.h>

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

void set_channel_value(uint8_t chan, uint16_t value) {
    switch (chan) {
        default:
        case 0:
            TCC0.CCABUF = value;
            break;
        case 1:
            TCC0.CCBBUF = value;
            break;
        case 2:
            TCC0.CCCBUF = value;
            break;
    }
}

uint16_t get_channel_value(uint8_t chan) {
    switch (chan) {
        default:
        case 0: return TCC0.CCA;
        case 1: return TCC0.CCB;
        case 2: return TCC0.CCC;
    }
}
