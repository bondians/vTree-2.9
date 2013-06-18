#include <stdint.h>
#include <util/delay.h>

#include "lights.h"

void set_all(uint8_t i) {
    set_channel_value(0, i);
    set_channel_value(1, i);
    set_channel_value(2, i);
    set_channel_value(3, i);
}

int main(void)
{
    init_light_subsystem();

    while(1) {
        // ramp up once
        uint16_t i;
        for (i = 0; i < 0x100; i++) {
            _delay_ms(4);
            set_all(i);
        }

        // blink a few times
        for (i = 0; i < 10; i++) {
            _delay_ms(50);
            set_all(0x40);
            _delay_ms(50);
            set_all(0x7f);
        }
    }
}
