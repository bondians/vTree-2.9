#ifndef ___n_xbee_h__
#define ___n_xbee_h__

#include <pt.h>
#include <stdbool.h>
#include <stdint.h>

void xbee_byte_received(uint8_t byte, bool err);

PT_THREAD(xbee_task(struct pt *pt));

#endif /* ___n_xbee_h__ */
