#ifndef ___n_board_h__
#define ___n_board_h__

#include <stdint.h>

void init_board(void);

void apply_light_values(uint16_t r, uint16_t g, uint16_t b);

#define IR_TICK_USEC        50      // microseconds per clock interrupt tick

#endif /* ___n_board_h__ */
