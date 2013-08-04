#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>

#include "xbee.h"
#include "lights.h"
#include "ir.h"

int main(void)
{
    // set clock divider to 2 (8 MHz)
    CLKPR = 1 << CLKPCE;
    CLKPR = 0x01;
    
    init_light_subsystem();
    init_ir_subsystem();
    init_xbee_subsystem();
    sei();
    
    while(1) {
        sleep_mode();
    }
}
