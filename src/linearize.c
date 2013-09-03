#include "linearize.h"

#define swap(t,x,y) do {t tmp = x; x = y; y = tmp; } while(0);

// This corrects for the nonlinearity arising from the fact that the
// 3 LED channels are driven by a single constant-current supply.  It makes
// the following simplifying assumptions:
// 
//  1) Instantaneous intensity is assumed to be a linear function of current
//  2) The current is assumed to be evenly divided between all enabled channels
//  3) It is assumed that the supply reacts very quickly relative to the PWM
//     frequency (TODO: determine whether additional correction is needed for
//     power supply response lag)
void linearize(
    volatile uint16_t *ch0_loc,
    volatile uint16_t *ch1_loc,
    volatile uint16_t *ch2_loc)
{
    uint16_t ch0 = *ch0_loc;
    uint16_t ch1 = *ch1_loc;
    uint16_t ch2 = *ch2_loc;
    
    // sort channels into non-decreasing order of brightness
    if (ch0 > ch1) {swap(uint16_t,ch0,ch1); swap(volatile uint16_t *,ch0_loc,ch1_loc)}
    if (ch1 > ch2) {swap(uint16_t,ch1,ch2); swap(volatile uint16_t *,ch1_loc,ch2_loc)}
    if (ch0 > ch1) {swap(uint16_t,ch0,ch1); swap(volatile uint16_t *,ch0_loc,ch1_loc)}
    
    // if ch0 == ch2, all channels are equal and no correction is needed.
    // note that this accounts for the case where ch2 == 0, so the division
    // below will always succeed.
    if (ch0 < ch2) {
        // channel weights corrected so that time*current is allocated in
        // the proportion specified by ch0, ch1 and ch2
        uint32_t a = 3 * (uint32_t)ch0;
        uint32_t b = (uint32_t)ch0 + 2 * (uint32_t)ch1;
        uint32_t c = (uint32_t) ch0 + (uint32_t) ch1 + (uint32_t) ch2;
        
        // normalization factor to map c to max intensity (i.e., ch2)
        uint16_t x = (uint16_t) (((uint32_t) ch2 << 16) / c);
        
        // output the final corrected values
        // (only ch0 and ch1, ch2 is already the desired value)
        *ch0_loc = (uint16_t) (a * (uint32_t) x >> 16);
        *ch1_loc = (uint16_t) (b * (uint32_t) x >> 16);
    }
}
