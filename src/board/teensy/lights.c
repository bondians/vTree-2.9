#include "lights.h"

#include <avr/io.h>

void set_rgb(uint16_t r, uint16_t g, uint16_t b) {
    OCR1A = r;
    OCR1B = g;
    OCR1C = b;
}

void set_channel_value(uint8_t chan, uint16_t value) {
    switch (chan) {
        default:
        case 0:
            OCR1A = value;
            break;
            
        case 1:
            OCR1B = value;
            break;
            
        case 2:
            OCR1C = value;
            break;
    }
}

uint16_t get_channel_value(uint8_t chan) {
    switch (chan) {
        default:
        case 0:
            return OCR1A;
        
        case 1:
            return OCR1B;
        
        case 2:
            return OCR1C;
    }
}
