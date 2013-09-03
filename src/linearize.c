#include "linearize.h"
#include "math.h"

#define R_65535 (1.5259021896696422e-5)

#define swap(t,x,y) do {t tmp = x; x = y; y = tmp; } while(0);

static float clip(float x) {
    return x <= 0 ? 0 : x <= 1 ? x : 1;
}

// This corrects for the nonlinearity arising from the fact that the
// 3 LED channels are driven by a single constant-current supply.  It makes
// the following simplifying assumptions:
// 
//  1) Instantaneous intensity is assumed to be a linear function of current
//  2) The current is assumed to be evenly divided between all enabled channels
//  3) It is assumed that the supply reacts very quickly relative to the PWM
//     frequency (TODO: determine whether additional correction is needed for
//     power supply response lag)
// TODO: see whether this can easily be converted to integer math
void linearize(
    volatile uint16_t *ch0_loc,
    volatile uint16_t *ch1_loc,
    volatile uint16_t *ch2_loc)
{
    float ch0 = clip(*ch0_loc * R_65535);
    float ch1 = clip(*ch1_loc * R_65535);
    float ch2 = clip(*ch2_loc * R_65535);
    
    if (ch0 > ch1) {swap(float,ch0,ch1); swap(volatile uint16_t *,ch0_loc,ch1_loc)}
    if (ch1 > ch2) {swap(float,ch1,ch2); swap(volatile uint16_t *,ch1_loc,ch2_loc)}
    if (ch0 > ch1) {swap(float,ch0,ch1); swap(volatile uint16_t *,ch0_loc,ch1_loc)}
    
    // TODO: think about normalization/gamut tradeoffs
    //  currently normalizing to preserve maximum channel value
    float C = ch2; 
    
    // The others are simple linear combinations of that one.
    // This is a useful common factor.
    float ch_sum = ch0 + ch1 + ch2;
    float k = ch_sum <= 0 ? 0 : C / ch_sum;
    
    *ch0_loc = (uint16_t) round(clip(3. * ch0         * k) * 65535.0);
    *ch1_loc = (uint16_t) round(clip((ch0 + 2. * ch1) * k) * 65535.0);
    *ch2_loc = (uint16_t) round(clip(C                   ) * 65535.0);
}
