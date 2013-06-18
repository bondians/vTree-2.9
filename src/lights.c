#include "lights.h"

#include <avr/io.h>

// TODO: there are probably stdlib functions for these... find them
#define setBit(reg, bit, mode) ({if (mode) reg |= 1<<(bit); else reg &= ~(1<<(bit));})
#define pinMode(X, pin, mode)   setBit(DDR ## X, pin, mode)
#define setPin(X, pin, mode)    setBit(PORT ## X, pin, mode)

void init_light_subsystem() {
    pinMode(B, 2, 1);
    pinMode(B, 3, 1);
    pinMode(B, 4, 1);
    pinMode(D, 5, 1);
    
    // timer 0: "fast PWM", output set at OCR0x/cleared at TOP, clock/64 (977 Hz)
    TCCR0A = 0xF3;
    TCCR0B = 0x03;
    
    // timer 1: "fast PWM, 8 bit", output set at OCR1x/cleared at TOP, clock/64 (977 Hz)
    TCCR1A = 0xF1;
    TCCR1B = 0x0B;
    
    OCR0A = 0x00;
    OCR0B = 0x00;
    OCR1A = 0x0000;
    OCR1B = 0x0000;
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
