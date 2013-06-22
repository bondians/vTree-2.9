#include <stdint.h>
#include <util/delay.h>

#include "lights.h"

void set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    set_channel_value(0, g);
    set_channel_value(1, b);
    set_channel_value(2, r);
}

int main(void)
{
    init_light_subsystem();
    
    uint8_t x = 0;
    while(1) {
        set_rgb(x, x, x);
        x++;
        _delay_ms(10);
    }
}
