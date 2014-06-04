#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>

#include "debug.h"

#include "board.h"
#include "lights.h"

int main(void)
{
    init_board();
    init_debug_subsystem();
    
    dprintf("booted in debug mode\r\n");
    
    while(1) {
        sleep_mode();
    }
}
