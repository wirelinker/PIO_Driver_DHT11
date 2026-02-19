/*
 * Copyright (c) 2026 wirelinker
 * SPDX-License-Identifier: MIT License
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "dht11.pio.h"
#include "dht11_pio.h"

int main() {

    /* Use PIO block 0, state machine 0.
     * pio_code_memory_offset: Machine code offset in PIO block instruction memory.
     * Will be set when adding asm program machine code to the instruction memory.*/
    PIO pio_block = pio0;
    uint pio_sm = 0;
    uint pio_code_memory_offset = 0;

    /* For print out message. */
    setup_default_uart();

    printf("dht11 main start\n");
    dht11_init(&pio_block, &pio_sm, &pio_code_memory_offset); 

    while(1)
    {
        sleep_ms(2000);
        dht11_read(&pio_block, &pio_sm);
    }

    pio_remove_program_and_unclaim_sm(&dht11_pioasm_program, pio_block, pio_sm, pio_code_memory_offset);
}


