#ifndef ___n_ir_h__
#define ___n_ir_h__

#include <stdbool.h>
#include <stdint.h>
#include <pt.h>

void ir_pin_watchdog_timeout();
void ir_pin_changed(bool irdata, uint16_t time);

PT_THREAD(ir_task(struct pt *pt));
#endif /* ___n_ir_h__ */
