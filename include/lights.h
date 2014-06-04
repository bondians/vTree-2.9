#ifndef ___n_lights_h__
#define ___n_lights_h__

#include <stdint.h>

void set_rgb(uint16_t r, uint16_t g, uint16_t b);
void set_channel_value(uint8_t chan, uint16_t value);
uint16_t get_channel_value(uint8_t chan);

#endif /* ___n_lights_h__ */
