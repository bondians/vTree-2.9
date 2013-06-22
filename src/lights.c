#include "lights.h"

#include <avr/io.h>

// TODO: there are probably stdlib functions for these... find them
#define setBit(reg, bit, mode) ({if (mode) reg |= 1<<(bit); else reg &= ~(1<<(bit));})
#define pinMode(X, pin, mode)   setBit(DDR ## X, pin, mode)
#define setPin(X, pin, mode)    setBit(PORT ## X, pin, mode)

void init_light_subsystem() {
    
    // timer 0: "phase-correct PWM", output set at TOP/cleared at OCR0x, clock/64 (977 Hz)
    TCCR0A = 0xA1;
    TCCR0B = 0x03;
    
    // timer 1: "phase-correct PWM, 8 bit", output set at TOP/cleared at OCR1x, clock/64 (977 Hz)
    TCCR1A = 0xA1;
    TCCR1B = 0x03;
    
    set_channel_value(0, 0);
    set_channel_value(1, 0);
    set_channel_value(2, 0);
    set_channel_value(3, 0);
    
    pinMode(B, 2, 1);
    pinMode(B, 3, 1);
    pinMode(B, 4, 1);
    pinMode(D, 5, 1);
}

void set_channel_value(uint8_t chan, uint8_t value) {
    switch (chan) {
        default:
        case 0:
            OCR1A = value;
            break;
        
        case 1:
            OCR1B = value;
            break;
        
        case 2:
            OCR0A = value;
            break;
        
        case 3:
            OCR0B = value;
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
