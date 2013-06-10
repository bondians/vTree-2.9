#ifndef ___n_lights_h__
#define ___n_lights_h__

#include <stdint.h>

/*
 * PWM channels available on Attiny2313a:
 *
 *      Timer 0 (8 bit):
 *          OCR0A: Pin 14 (Port B, bit 2)
 *          OCR0B: Pin 9  (Port D, bit 5)
 *      Timer 1 (16 bit):
 *          OCR1A: Pin 15 (Port B, bit 3)
 *          OCR1B: Pin 16 (Port B, bit 4)
 *
 *      There are only these 2 timers available; we may
 *      need to use software PWM (probably on timer 1)
 *      if a timer is needed for other purposes.
 */

#define NUM_CHANNELS 3

void init_light_subsystem();
void set_channel_value(uint8_t chan, uint8_t value);
uint8_t get_channel_value(uint8_t chan);

#endif /* ___n_lights_h__ */
