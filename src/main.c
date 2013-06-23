#include <stdint.h>
#include <util/delay.h>

#include "lights.h"
#include "timing.h"

static inline uint16_t sqr(uint8_t x) {
    uint16_t y = x+1;
    return y*y-1;
}

void set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    set_channel_value(0, sqr(g));
    set_channel_value(1, sqr(b));
    set_channel_value(2, sqr(r));
}

void set_white(uint8_t x) {
    uint16_t y = sqr(x);
    set_channel_value(0, y);
    set_channel_value(1, y < 100 ? y : y >= 0xFFFF ? y : y+1);
    set_channel_value(2, y < 100 ? y : y >= 0xFFFE ? y : y+2);
}

int main(void)
{
    init_light_subsystem();
    init_timing_subsystem();
    
    while(1) {
        set_white(0);
        _delay_ms(1000);
        
        for(int i = 1; i < 255; i++) {
            set_white(i);
            _delay_ms(5);
        }

        for(int i = 0; i < 10; i++) {
            set_white(i&1 ? 0xC0 : 0xFF);
            _delay_ms(50);
        }
        
        set_white(255);
        _delay_ms(1000);
    }
}
