#include <stdint.h>
#include <util/delay.h>

#include "lights.h"

static inline uint16_t sqr(uint8_t x) {
    uint16_t y = x+1;
    return y*y-1;
}

static void set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    set_channel_value(0, sqr(b));
    set_channel_value(1, sqr(g));
    set_channel_value(2, sqr(r));
}

static inline int16_t abs(int16_t x) {
    return x >= 0 ? x : -x;
}

static void set_white(uint8_t x) {
    uint16_t y = sqr(x);
    set_channel_value(0, y);
    set_channel_value(1, y < 100 ? y : y >= 0xFFFF ? y : y+1);
    set_channel_value(2, y < 100 ? y : y >= 0xFFFE ? y : y+2);
}

// TODO: clean up... lots of fast-and-loose reasoning and ad-hoc patches to fix the outcome...
static void set_hsv(uint8_t h, uint8_t s, uint8_t v) {
    if (v == 0) {
        set_white(0);
        return;
    }
    
    uint16_t c = v * s; // * 2^-16
    uint16_t hPrime = h * 6; // * 2^-8
    uint16_t x = (c >> 8) * (256 - abs(hPrime % 512 - 256)); // * 2^-16
    
    uint16_t r1, g1, b1;
    switch (hPrime >> 8) {
        default:
        case 0:
            r1 = c, g1 = x; b1 = 0;
            break;
        case 1:
            r1 = x, g1 = c; b1 = 0;
            break;
        case 2:
            r1 = 0, g1 = c; b1 = x;
            break;
        case 3:
            r1 = 0, g1 = x; b1 = c;
            break;
        case 4:
            r1 = x, g1 = 0; b1 = c;
            break;
        case 5:
            r1 = c, g1 = 0; b1 = x;
            break;
    }
    
    uint16_t m = (v<<8) - c - 1; // * 2^-16
    
    set_channel_value(0, sqr((m + b1)>>8));
    set_channel_value(1, sqr((m + g1)>>8));
    set_channel_value(2, sqr((m + r1)>>8));
}

int main(void)
{
    init_light_subsystem();
    
    uint8_t h = 0;
    while(1) {
#if 0
        for (int i = 0; i <= 255; i++) {
            set_hsv(h, i, i);
            if (i) _delay_ms(5); else _delay_ms(500);
        }
        
        h += 101;
#else
        for(int i = 0; i <= 255; i++) {
            set_hsv(i, 255, 255);
            _delay_ms(25);
        }
#endif
    }
}
