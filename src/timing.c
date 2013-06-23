#include "timing.h"
#include <avr/interrupt.h>

#define NULL            ((void *) 0)
#define LG2_MAX_EVENTS  3
#define MAX_EVENTS      (1 << LG2_MAX_EVENTS)
#define NEAR_PAST       0xFF

// double-buffer of events ordered by time.  active buffer contains
// events yet to be fired in this cycle, arranged as a circular queue
// ordered by time.
// 
// inactive buffer contains events for next cycle, also ordered by time.

// after each event fires, it will be removed.
// if more than MAX_EVENTS are queued, behavior is undefined
uint8_t next = 0;
uint8_t which_buf = 0;
event_t *events[2][MAX_EVENTS] = {};

static inline void incr() {
    next = (next + 1) & (MAX_EVENTS - 1);
}

static inline void dispatch_events() {
    event_t *event;
    event_t **buf = events[which_buf];
    while((event = buf[next])) {
        OCR1A = event->when;
        if (OCR1A > TCNT1+4) { // + a few to make sure there's time for the interrupt to fire again if necessary
            break;
        } else {
            buf[next] = NULL;
            incr();
            event->what();
        }
    }
}

// COMPA fires before OVF, COMPB fires after OVF (and other stuff, including UART interrupts)
ISR(TIMER1_COMPA_vect) {
    dispatch_events();
}

ISR(TIMER1_OVF_vect) {
    next = 0;
    which_buf = which_buf ? 0 : 1;
    
    dispatch_events();
}

void init_timing_subsystem() {
    // start timer 1 in "normal mode" (16-bit) with no clock scaling and no output-compare
    TCCR1A = 0x00;
    TCCR1B = 0x01;
    
    // TODO: need to explicitly enable interrupts?
    TIMSK = 0xC0;
    sei();
}

static void insert(event_t **buf, uint8_t i0, event_t *event) {
    for (uint8_t i = 0; i < MAX_EVENTS; i++) {
        uint8_t slot = (i0 + i) & (MAX_EVENTS - 1);
        
        if (buf[slot] == NULL) {
            buf[slot] = event;
            break;
        } else {
            if (buf[slot]->when > event->when) {
                event_t *tmp = buf[slot];
                buf[slot] = event;
                event = tmp;
            }
        }
    }
}


void register_event(event_t *event, uint8_t near) {
    if (near && event->when + NEAR_PAST >= TCNT1) {
        // event is in future (or near past) of current cycle, add it to current buffer
        insert(events[which_buf], next, event);
    } else {
        // event is in next cycle, add it to inactive buffer
        insert(events[!which_buf], 0, event);
    }
}
