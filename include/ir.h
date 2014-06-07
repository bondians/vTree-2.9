#ifndef ___n_ir_h__
#define ___n_ir_h__

#include <stdbool.h>
#include <stdint.h>

void ir_pin_watchdog_timeout();
void ir_pin_changed(bool irdata, uint16_t time);

#endif /* ___n_ir_h__ */
