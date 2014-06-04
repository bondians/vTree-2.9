#ifndef ___n_xbee_h__
#define ___n_xbee_h__

#include <stdint.h>

void xbee_reset_parser(void);
void xbee_byte_received(uint8_t byte);

#endif /* ___n_xbee_h__ */
