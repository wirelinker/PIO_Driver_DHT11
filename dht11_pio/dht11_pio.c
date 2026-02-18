/**
 *
 * SPDX-License-Identifier: MIT License
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "dht11.pio.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"

#define DHT11_PIO_PIN 26
#define PIO_DIV_SHIFT 17  /* Use this to do PIO clock division calculation. */


static void dht11_pio_init(PIO pio, uint sm, uint offset, uint pin) {

    uint32_t clk_div_int = 0;

    pio_sm_config c = dht11_pioasm_program_get_default_config(offset);


    /*
     * Set data pin to open-drain mode with pull-up enabled
     */

    /* Set the data pin to high as default.*/
    gpio_put(pin, 1);
    /* Enable pull-up resistor, then disable output. */
    gpio_pull_up(pin);
    /* Connect GPIO muxing to PIO function */
    pio_gpio_init(pio, pin);

    /* 
     * Now, the pin value signal from PIO should be high.
     * So, this GPIO_OVERRIDE_INVERT setting should cause the output disabled.
     * The data pin should be still pulled up by pull-up resistor.
     *
     * If the pin value signal from PIO became low, the output
     * should be enabled, then the data pin would be pulled low by
     * push-pull transistor. And the open-drain output would be achieved.
     */
    gpio_set_oeover(pin, GPIO_OVERRIDE_INVERT);



    /*
     * setup PIO state machine configuration structure
     */

    /* clock divider, set to about 5us per clock cycle
     * Use integer part of divider. 
     * The DHT11 signal is slow and less precision required.
     *
     * 5us => 0.2MHz = 200kHz = 200,000 = 2 ^ 6 * 3125
     *
     * try to choose a closest 2's power to simplify the div calculation
     * 2048 < 3125 < 4096 => 2^11 < 3125 < 2^12
     * 125MHz / 2^(6 + 11) => divider = 953 => 131164.7429 Hz => 7.624us
     * 125MHz / 2^(6 + 12) => divider = 476 => 262605.042 Hz => 3.808us
     *
     * 48MHz / 2^(6 + 11) => divider = 366 => 131147.541 Hz => 7.625us
     * 48MHz / 2^(6 + 12) => divider = 183 => 262295.082 Hz => 3.813us
     *
     * choose 2^(6 + 12) => 3.8us, which could make the result closer to 5us
     * or
     * choose 2^(6 + 11) => 7.6us, which would run at a lower frequency
     * has two benefits:
     * 1. lower power consumption(a little bit)
     * 2. save instruction memory 
     */ 
    clk_div_int = clock_get_hz(clk_sys) >> PIO_DIV_SHIFT;
    sm_config_set_clkdiv_int_frac8(&c, clk_div_int, 0);

    /* Config IN, SET, sideset pins */
    sm_config_set_in_pins(&c, pin);
    sm_config_set_set_pins(&c, pin, 1);

    /* Config ISR as MSb first, shift left, autopush enabled, 8 bits threshold. */
    sm_config_set_in_shift(&c, false, true, 8);
    /* set RX FIFO join to get 8 FIFO space for RX*/
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX); 

    /*
     * Set configuration to the state machine.
     * The PC would be set to the "offset" to the instruction memory address base.
     * That's where the state machine would start executing pioasm program.
     */
    pio_sm_init(pio, sm, offset, &c);
    /* Run the state machine. */
    pio_sm_set_enabled(pio, sm, true);
}

void dht11_init(PIO *pio, uint *sm, uint *offset) {

    printf("DHT11 init========\n");
    printf("sys clock %d Hz\n", clock_get_hz(clk_sys));
    printf("pio clock dividor %d\n", clock_get_hz(clk_sys) >> PIO_DIV_SHIFT);
    printf("Using gpio %d as DHT11 data pin.\n", DHT11_PIO_PIN);

    *offset = pio_add_program(*pio, &dht11_pioasm_program);

    printf("PIO: %X, SM: %d, pioasm offset: %d\n", pio, *sm, *offset);

    dht11_pio_init(*pio, *sm, *offset, DHT11_PIO_PIN);
}

void dht11_read(PIO* pio_ptr, uint *sm_ptr) {

    uint32_t data[5] = {0};
    uint32_t data_idx = 0;
    static uint32_t first_time_read = 1;

    /* Set data pin low to start reading.
     * Wait 18ms
     */
    if(first_time_read)
    {
        printf("First time dummy read.\n");
        first_time_read = 0;
    }

    printf("Start Reading\n");
    pio_sm_exec(*pio_ptr, *sm_ptr, pio_encode_set(pio_pins, 0));
    sleep_ms(18);

    pio_sm_exec(*pio_ptr, *sm_ptr, pio_encode_set(pio_pins, 1));

    /* Wait for sensor to send data.*/
    sleep_ms(40);

    while(!pio_sm_is_rx_fifo_empty(*pio_ptr, *sm_ptr))
    {
        data[data_idx] = pio_sm_get(*pio_ptr, *sm_ptr);
        data_idx++;
    }
    data_idx = 0;

    printf("HUDT:%d.%d TEMP:%d.%d CHECKSUM:%d\n", data[0], data[1], data[2], data[3], data[4]);

}



int main() {

    /* Use PIO block 0, state machine 0.
     * pio_code_memory_offset: Machine code offset in PIO block instruction memory.
     * Will be set when adding asm program machine code to the instruction memory.*/
    PIO pio_block = pio0;
    uint pio_sm = 0;
    uint pio_code_memory_offset = 0;

    /* For print out message. */
    setup_default_uart();

    dht11_init(&pio_block, &pio_sm, &pio_code_memory_offset); 

    while(1)
    {
        sleep_ms(2000);
        dht11_read(&pio_block, &pio_sm);
    }

    pio_remove_program_and_unclaim_sm(&dht11_pioasm_program, pio_block, pio_sm, pio_code_memory_offset);
}

