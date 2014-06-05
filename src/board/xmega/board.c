#include <avr/interrupt.h>

#include "ir.h"
#include "xbee.h"

ISR(TCC5_OVF_vect)
{
    // Read IR pin
    uint8_t irdata = !(PORTC.IN & 0b1000);
    
    // Clear interrupt
    TCC5.INTFLAGS = TC5_OVFIF_bm;
    
    receive_ir_data(irdata);
}

#define BAUD 9600

ISR(USARTC0_RXC_vect) {
        // read status and byte immediately
    uint8_t status  = USARTC0.STATUS;
    uint8_t in_byte = USARTC0.DATA;
    
    if (status & USART_FERR_bm) {
        // reset parser on frame errors
        xbee_reset_parser();
    } else {
        xbee_byte_received(in_byte);
    }
}

// #define USE_EXTERNAL_CLOCK

static void setup_clock() {
#ifdef USE_EXTERNAL_CLOCK
    // Configure, enable, and wait for crystal
    OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
    OSC.CTRL |= OSC_XOSCEN_bm;
    while(!(OSC.STATUS & OSC_XOSCRDY_bm));
    
    // Configure, enable, and wait for 2x PLL
    OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | (2 << OSC_PLLFAC_gp);
    OSC.CTRL |= OSC_PLLEN_bm;
    while(!(OSC.STATUS & OSC_PLLRDY_bm));
    
    // Switch clock source to PLL output
    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_PLL_gc;
#else
    CCP = CCP_IOREG_gc;              // disable register security for oscillator update
    OSC.CTRL = OSC_RC32MEN_bm;       // enable 32MHz oscillator
    while(!(OSC.STATUS & OSC_RC32MRDY_bm)); // wait for oscillator to be ready
    CCP = CCP_IOREG_gc;              // disable register security for clock update
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc; // switch to 32MHz clock
#endif
}

static void setup_ir_pins() {
    // Ensure IR pin is set as input
    PORTC.DIRCLR = 0b1000;
    
    // set up TCC5 to give an interrupt every 50 usec (1600 cycles @ 32MHz)
    TCC5.CTRLB = 0b000 << TC5_WGMODE_gp;
    TCC5.CTRLC = 0;
    TCC5.CTRLD = 0;
    TCC5.CTRLE = 0;
    
    TCC5.PER = 1600;
    
    // enable TCC5_OVF_vect interrupt at LOW priority
    TCC5.INTCTRLA = 0b01 << TC5_OVFINTLVL_gp;
    
    // start the timer
    TCC5.CTRLA = 0b0001 << TC5_CLKSEL_gp;
}

static void setup_light_pins() {
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

static void setup_xbee_uart() {
    // TODO: something semi-automatic
    #define BSEL    12
    #define BSCALE  4
    #define USE_2X  0
    
    PORTC.REMAP |= PORT_USART0_bm;
    
    USARTC0.CTRLA = USART_RXCINTLVL_MED_gc;
    USARTC0.CTRLB = USE_2X ? USART_CLK2X_bm : 0;
    USARTC0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc 
                  | USART_PMODE_DISABLED_gc
                  | USART_CHSIZE_8BIT_gc;
    
    USARTC0.BAUDCTRLA = BSEL;
    USARTC0.BAUDCTRLB = (BSEL >> 8) | (BSCALE << 4);
    
    USARTC0.CTRLB |= USART_RXEN_bm;
}

void init_board(void) {
    setup_clock();
    
    setup_ir_pins();
    setup_light_pins();
    setup_xbee_uart();
    
    // Enable interrupts (first, the specific interrupt levels 
    // we care about - that is, all of them - then the global
    // interrupt flag)
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
    sei();
}

