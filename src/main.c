#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <pt.h>
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>

#include "debug.h"

#include "board.h"
#include "xbee.h"

int main(void)
{
    init_board();
    init_debug_subsystem();
    
    dprintf("booted in debug mode\r\n");
    
    struct pt xbee_pt;
    PT_INIT(&xbee_pt);
    
    while(1) {
        xbee_task(&xbee_pt);
        
        sleep_mode();
    }
}
