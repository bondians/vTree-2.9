#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>

#include "clock.h"
#include "ir.h"
#include "lights.h"
#include "xbee.h"

int main(void)
{
    setup_clock();
    
    init_light_subsystem();
    init_ir_subsystem();
    init_xbee_subsystem();
    
    // Enable interrupts (first, the specific interrupt levels 
    // we care about - that is, all of them - then the global
    // interrupt flag)
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
    sei();
    
    while(1) {
        sleep_mode();
    }
}
