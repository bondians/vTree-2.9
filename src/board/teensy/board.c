/// \file teensy/board.c
///
/// Board-specific setup and peripheral driver code for Dave's
/// teensy-based (specifically, teensy 2.0) prototype.
///
/// Board overview:
///  pin mapping quick reference:
///      https://www.pjrc.com/teensy/card2a.pdf
///  schematic:
///      https://www.pjrc.com/teensy/schematic2.gif
/// 
/// Usage details 
///
/// Board has a 16 MHz crystal.  Clock starts up with divider 
/// set to 8, so we start by switching divider to 1.
/// 
/// Peripheral usage:
///
/// * Timer1 - PWM channels for LED output.
/// * Timer3 - IR input capture and timing
/// * USART1 - XBee
///
/// Pin usage:
/// 
/// * PB5/6/7: LED PWM output
/// * PC7: IR input
/// * PD2/3: xbee RX/TX, respectively
/// * PD6: status LED (on teensy board)
///

#include "board.h"

#include "ir.h"
#include "linearize.h"
#include "xbee.h"

#include <avr/io.h>
#include <avr/interrupt.h>

static void setup_clock();
static void setup_ir_pin();
static void setup_light_pins();
static void setup_xbee_uart();

#define ENABLE_STATUS_LED       do {DDRD |= 1 << DDD6;} while(0)
#define SET_STATUS_LED(on)      do {PORTD = (PORTD & ~(1 << PIND6)) | ((on ? 1 : 0) << PIND6);} while(0)

/// Setup the clock and all peripherals and interrupts used by the
/// teensy to implement functionality used by the board-independent
/// code in "src" 
void init_board(void) {
    setup_clock();
    
    setup_ir_pin();
    setup_light_pins();
    setup_xbee_uart();
    
    // enable onboard LED output so we can blink it suggestively
    ENABLE_STATUS_LED;
    
    sei();
}

/// teensy starts up in CLKDIV8 mode (2 MHz).
/// all we need to do here is switch out of it.
static void setup_clock() {
    CLKPR = 0b10000000; // enable changing the clock prescaler
    CLKPR = 0b00000000; // execute the change
}

/// the IR edge being waited for (this chip only
/// supports input capture for one edge at a time,
/// so we have to switch it back and forth).
/// 
/// \todo consider switching to a general pin-change
///       interrupt, we probably don't really need hardware
///       input capture.  the one benefit of using the timer
///       peripheral for this is that we can use the hardware
///       debouncing, but I think the IR module already does
///       some pretty aggressive noise filtering.  Dual-edge
///       interrupt sources to consider: INT0 on PD0, INT1
///       on PD1, or PCINT0:7 on PB0:7.  Alternatively,
///       consider relaxing IR parser to only time from
///       pulse to pulse, ignoring pulse widths) 
static bool rising_edge = false;

/// set up input to read IR sensor on PC7
/// and Timer3 to tick 64 usec, interrupt on pin change,
/// and interrupt twice during the cycle as well.
static void setup_ir_pin() {
    // set IR pin to input
    DDRC &= ~(1 << DDC7);
    
    // Timer3 configuration
    const uint8_t COM  = 0b00;   // no PWM
    const uint8_t WGM  = 0b0000; // Normal mode
    const uint8_t CS   = 0b101;  // 1:1024 with IO clock (15.625 kHz, period 64 µs)
    const uint8_t ICNC = 0b1;    // input-capture noise canceler enabled
    const uint8_t ICES = 0b0;    // initially looking for falling edge.
    
    // apply configuration
    TCCR3A
        = (COM              << COM3A0)
        | (COM              << COM3B0)
        | (COM              << COM3C0)
        | ((WGM & 0b0011)   << WGM30);
    TCCR3B
        = ((WGM >> 2)       << WGM32)
        | (CS               << CS30)
        | (ICES             << ICES3)
        | (ICNC             << ICNC3);
    rising_edge = ICES;
    
    // enable interrupts
    TIMSK3
        = 1 << ICIE3
        | 1 << TOIE3;
}

/// IR input pin changed
ISR(TIMER3_CAPT_vect) {
    bool     irdata     = !rising_edge;
    uint16_t event_time = ICR3;
    
    // reset the timer
    TCNT3 = 0;
    
    // switch the edge mode
    rising_edge = !rising_edge;
    TCCR3B = (TCCR3B & ~(1 << ICES3))
           | ((rising_edge ? 1 : 0) << ICES3);
    
    // report the event
    SET_STATUS_LED(irdata);
    ir_pin_changed(irdata, event_time);
}

/// IR input pin didn't change for a full timer cycle
ISR(TIMER3_OVF_vect) {
    ir_pin_watchdog_timeout();
}

/// set up the light channels:
/// PORTB pins 5/6/7 (OCR1A/B/C)
static void setup_light_pins() {
    // set initial values
    OCR1A = OCR1B = OCR1C = 0xFFFF;
    
    // PWM configuration
    const uint8_t COM = 0b11;   // inverted (in order to be able to set lights fully-off)
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

/// \brief Called by main application code to change output light levels.
///
/// Linearization is performed here rather than in shared code because
/// the XMEGA timers support locking out OCR updates, allowing it to
/// be done slightly more efficiently there.
void apply_light_values(uint16_t r, uint16_t g, uint16_t b) {
    linearize(&r, &g, &b);
    
    OCR1A = 0xFFFF - b;
    OCR1B = 0xFFFF - g;
    OCR1C = 0xFFFF - r;
}

/// initialize USART1 to the selected baud rate.
/// RX on PD2, TX on PD3.
static void setup_xbee_uart() {
    DDRD &= ~(1 << DDD2);   // set RX as input
    DDRD |=   1 << DDD3;    // set TX as output
    
    // set baud rate
    #define BAUD 9600
    #include <util/setbaud.h>
    UBRR1H = UBRRH_VALUE;
    UBRR1L = UBRRL_VALUE;
    UCSR1A = ((USE_2X ? 1 : 0) << U2X1);
    
    // set format to 8N1
    UCSR1C = ((3<<UCSZ10) | (0<<UPM10) | (0<<USBS1));
    
    // enable RX and RX-complete interrupt
    UCSR1B |= (1 << RXCIE1) | (1 << RXEN1);
}

/// byte received from XBee
ISR(USART1_RX_vect) {
    // read status and byte immediately
    uint8_t status  = UCSR1A;
    uint8_t in_byte = UDR1;
    
    xbee_byte_received(in_byte, status & (1 << FE1));
}
