#ifndef ___n_debug_h__
#define ___n_debug_h__

#define DEBUG

#ifdef DEBUG

#include <avr/pgmspace.h>
#include <stdio.h>

void init_debug_subsystem();

extern FILE *dbg;
#define dprintf(args...) fprintf(dbg, args);

#else

#define init_debug_subsystem() do {} while(0)
#define dbg     NULL
#define dprintf(args...) do{} while(0);

#endif



#endif /* ___n_debug_h__ */
