#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>

#include "xbee.h"
#include "lights.h"
#include "ir.h"

int main(void)
{
    init_light_subsystem();
    init_ir_subsystem();
    init_xbee_subsystem();
    sei();
    
    while(1) {
        sleep_mode();
    }
}
