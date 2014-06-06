#include "lights.h"
#include "board.h"

#include <avr/io.h>

static uint16_t channel_values[3] = {};

static void apply() {
    uint16_t r = channel_values[0],
             g = channel_values[1],
             b = channel_values[2];
    
    apply_light_values(r, g, b);
}

void set_rgb(uint16_t r, uint16_t g, uint16_t b) {
    channel_values[0] = r;
    channel_values[1] = g;
    channel_values[2] = b;
    
    apply();
}

void set_channel_value(uint8_t chan, uint16_t value) {
    channel_values[chan] = value;
    
    apply();
}

uint16_t get_channel_value(uint8_t chan) {
    return channel_values[chan];
}
