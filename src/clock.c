#include "clock.h"

#include <avr/io.h>

#define USE_EXTERNAL_CLOCK

void setup_clock() {
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