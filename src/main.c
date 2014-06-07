#include <avr/sleep.h>
#include <pt.h>

#include "debug.h"

#include "board.h"
#include "ir.h"
#include "xbee.h"

int main(void)
{
    init_board();
    init_debug_subsystem();
    
    dprintf("booted in debug mode\r\n");
    
    struct pt xbee_pt; PT_INIT(&xbee_pt);
    struct pt ir_pt;   PT_INIT(&ir_pt);
    
    while(1) {
        xbee_task(&xbee_pt);
        ir_task(&ir_pt);
        
        sleep_mode();
    }
}
