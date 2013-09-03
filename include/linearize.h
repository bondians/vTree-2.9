#ifndef ___n_linearize_h__
#define ___n_linearize_h__

#include <stdint.h>

// This system's power subsystem makes some compromises to reduce
// the cost and part count.  One of them is that all 3 color channels
// are fed by the same LED driver, which means the per-channel current
// is not constant over the duty cycle.  This function corrects some
// of the resulting nonlinearity.
void linearize(
    volatile uint16_t *r,
    volatile uint16_t *g,
    volatile uint16_t *b);

#endif /* ___n_linearize_h__ */
