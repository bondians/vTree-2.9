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
    // set IR pin to input
    DDRB &= ~(1 << DDB4);
    
    // Timer0 configuration
    const uint8_t COM = 0b00;   // no PWM
    const uint8_t WGM = 0b010;  // CTC mode, TOP = OCR0A
    const uint8_t CS  = 0b010;  // 1:8 with IO clock
    const uint8_t TOP = 99;     // Frequency: 20 kHz (period: 50 usec)
    
    // apply configuration
    OCR0A = TOP;
    TCCR0A
        = (COM              << COM0A0)
        | (COM              << COM0B0)
        | ((WGM & 0b0011)   << WGM00);
    TCCR0B
        = ((WGM >> 2)       << WGM02)
        | (CS               << CS00);
    
    // enable compare-match interrupt
    TIMSK0 = 1 << OCIE0A;
}

// set up the light channels:
// PORTB pins 5/6/7 (OCR1A/B/C)
static void setup_light_pins() {
    // set initial values
    OCR1A = OCR1B = OCR1C = 0xFFFF;
    
    // PWM configuration
    const uint8_t COM = 0b11;   // inverted
    const uint8_t WGM = 0b1110; // fast PWM, TOP = ICR1, OCR update at TOP
    const uint8_t CS  = 0b001;  // 1:1 with IO clock: 244 Hz PWM if clock is 16Mhz
    const uint16_t TOP = 0xFFFF;
    
    // apply configuration
    ICR1 = TOP;
    TCCR1A
        = (COM              << COM1A0)
        | (COM              << COM1B0)
        | (COM              << COM1C0)
        | ((WGM & 0b0011)   << WGM10);
    TCCR1B
        = ((WGM >> 2)       << WGM12)
        | (CS               << CS10);
    
    // enable outputs
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
