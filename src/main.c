#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>

#include "dmx512.h"
#include "lights.h"
#include "ir.h"

int main(void)
{
    init_light_subsystem();
    init_dmx512_subsystem(6);
    init_ir_subsystem();
    sei();
    
    while(1) {
        sleep_mode();
    }
}
