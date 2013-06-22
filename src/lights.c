#include "lights.h"

#include <avr/io.h>

// TODO: there are probably stdlib functions for these... find them
#define setBit(reg, bit, mode) ({if (mode) reg |= 1<<(bit); else reg &= ~(1<<(bit));})
#define pinMode(X, pin, mode)   setBit(DDR ## X, pin, mode)
#define setPin(X, pin, mode)    setBit(PORT ## X, pin, mode)

void init_light_subsystem() {
    // timer 0: "fast PWM", output set at TOP/cleared at OCR0x, clock/64 (977 Hz)
    TCCR0A = 0xA3;
    TCCR0B = 0x03;
    
    // timer 1: "fast PWM, 8 bit", output set at TOP/cleared at OCR1x, clock/64 (977 Hz)
    TCCR1A = 0xA1;
    TCCR1B = 0x0B;
    
    set_channel_value(0, 0);
    set_channel_value(1, 0);
    set_channel_value(2, 0);
    set_channel_value(3, 0);
}

void set_channel_value(uint8_t chan, uint8_t value) {
    uint8_t mode = value ? 1 : 0;
    switch (chan) {
        default:
        case 0:
            OCR1A = value;
            pinMode(B, 3, mode);
            break;
        
        case 1:
            OCR1B = value;
            pinMode(B, 4, mode);
            break;
        
        case 2:
            OCR0A = value;
            pinMode(B, 2, mode);
            break;
        
        case 3:
            OCR0B = value;
            pinMode(D, 5, mode);
            break;
    }
    
}

uint8_t get_channel_value(uint8_t chan) {
    switch (chan) {
        default:
        case 0:     return OCR1A;
        case 1:     return OCR1B;
        case 2:     return OCR0A;
        case 3:     return OCR0B;
    }
}
