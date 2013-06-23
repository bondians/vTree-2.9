#ifndef ___n_timing_h__
#define ___n_timing_h__

#include <avr/io.h>

typedef void (*action_t)();
typedef struct event_t {
    uint16_t    when;
    action_t    what;
} event_t;

void init_timing_subsystem(); 
void register_event(event_t *event, uint8_t near); // mem must remain valid till event fires

static inline uint16_t now() {
    return TCNT1;
}

#endif /* ___n_timing_h__ */
