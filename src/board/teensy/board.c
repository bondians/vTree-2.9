#include "board.h"

#include "ir.h"

#include <avr/io.h>
#include <avr/interrupt.h>

ISR(TIMER0_COMPA_vect) {
    bool irdata = !(PINB & (1 << PINB4));
    
    TIMSK0 = 1 << OCF0A;
    
    PORTD = 
        (PORTD & ~(1 << PIND6))
        | ((irdata ? 1 : 0) << PIND6);
    
    receive_ir_data(irdata);
}

void setup_clock() {
    cli();
    CLKPR = 0b10000000;
    CLKPR = 0b00000000;
}

// set up the light channels:
// PORTB pins 5/6/7 (OCR1A/B/C)
void setup_light_pins() {
    ICR1 = 0xFFFF;
    OCR1A = OCR1B = OCR1C = 0;
    
    // COMA/B/C     = 10  ("normal view")
    // WGM          = 1110 (fast PWM, TOP = ICR1, OCR update at TOP)
    // CS           = 001 (1:1 with IO clock: 244 Hz PWM if clock is 16Mhz )
    TCCR1A = 0b10101010;
    TCCR1B = 0b00011001;
    
    DDRB |= (1 << DDB5)
          | (1 << DDB6)
          | (1 << DDB7);
}

// set up input to read IR sensor on PB4
// and Timer0 to give an interrupt every 50 usec (800 cycles @ 16MHz)
void setup_ir_pin() {
    DDRB &= ~(1 << DDB4);
    
    OCR0A = 99;
    
    // COM0A/B      = 00 (no PWM)
    // WGM0         = 010 (CTC mode, TOP = OCR0A)
    // CS           = 010 (1:8 with IO clock)
    TCCR0A = 0b00000010;
    TCCR0B = 0b00000010;
    
    TIMSK0 = 1 << OCIE0A;
}

void init_board(void) {
    setup_clock();
    
    setup_light_pins();
    setup_ir_pin();
    
    DDRD |= 1 << DDD6; // enable onboard LED output so we can blink it suggestively
    
    sei();
}
