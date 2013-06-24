#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>

#include "dmx512.h"
#include "lights.h"

int main(void)
{
    init_light_subsystem();
    init_dmx512_subsystem(6);
    sei();
    
    while(1) {
        _delay_ms(100);
    }
}
