#include "board.h"

#include "ir.h"
#include "xbee.h"

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

#define BAUD 9600

ISR(USART1_RX_vect) {
    // read status and byte immediately
    uint8_t status  = UCSR1A;
    uint8_t in_byte = UDR1;
    
    if (status & (1 << FE1)) {
        // reset parser on frame errors
        xbee_reset_parser();
    } else {
        xbee_byte_received(in_byte);
    }
}

// teensy starts up in CLKDIV8 mode (2 MHz).
// all we need to do here is switch out of it.
static void setup_clock() {
    CLKPR = 0b10000000; // enable changing the clock prescaler
    CLKPR = 0b00000000; // execute the change
}

// set up input to read IR sensor on PB4
// and Timer0 to give an interrupt every 50 usec (800 cycles @ 16MHz)
static void setup_ir_pin() {
    DDRB &= ~(1 << DDB4);
    
    OCR0A = 99;
    
    // COM0A/B      = 00 (no PWM)
    // WGM0         = 010 (CTC mode, TOP = OCR0A)
    // CS           = 010 (1:8 with IO clock)
    TCCR0A = 0b00000010;
    TCCR0B = 0b00000010;
    
    // enable compare-match interrupt
    TIMSK0 = 1 << OCIE0A;
}

// set up the light channels:
// PORTB pins 5/6/7 (OCR1A/B/C)
static void setup_light_pins() {
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

// initialize USART1 to the selected baud rate.
// RX on PD2, TX on PD3.
static void setup_xbee_uart() {
    DDRD &= ~(1 << DDD2); // set RX as input
    // DDRD |=   1 << DDD3; // set TX as output
    
    // set baud rate
    #include <util/setbaud.h>
    UBRR1H = UBRRH_VALUE;
    UBRR1L = UBRRL_VALUE;
    UCSR1A = ((USE_2X ? 1 : 0) << U2X1);
    
    // set format to 8N1
    UCSR1C = ((3<<UCSZ10) | (0<<UPM10) | (0<<USBS1));
    
    // enable RX and RX-complete interrupt
    UCSR1B |= (1 << RXCIE1) | (1 << RXEN1);
}

void init_board(void) {
    setup_clock();
    
    setup_ir_pin();
    setup_light_pins();
    setup_xbee_uart();
    
    DDRD |= 1 << DDD6; // enable onboard LED output so we can blink it suggestively
    
    sei();
}
