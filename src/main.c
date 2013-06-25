#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>

#include "dmx512.h"
#include "lights.h"

int main(void)
{
    init_light_subsystem();
    init_dmx512_subsystem(6);
    sei();
    
    while(1) {
        sleep_mode();
    }
}
