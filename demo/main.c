/*

MIT License

Copyright (c) 2022 Oliver Schmidt (https://a2retro.de/)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>

#include <a2pico.h>

#include "board.h"

void main(void) {
    multicore_launch_core1(board);

#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif

    stdio_init_all();

    while (!stdio_usb_connected()) {
    }

    printf("\n\nCopyright (c) 2022 Oliver Schmidt (https://a2retro.de/)\n\n");

    while (true) {
        if (stdio_usb_connected()) {
            if (multicore_fifo_rvalid()) {
                putchar(multicore_fifo_pop_blocking());
            }
        }

        int data = getchar_timeout_us(0);
        if (data != PICO_ERROR_TIMEOUT) {
            if (data == 27) {
                a2pico_irq(true);
            }
            else if (multicore_fifo_wready()) {
                multicore_fifo_push_blocking(data);
            } else {
                putchar('\a');
            }
        }

        if (reset) {
            reset = false;
            printf(" RESET ");
        }

#ifdef PICO_DEFAULT_LED_PIN
        gpio_put(PICO_DEFAULT_LED_PIN, active);
#endif
    }
}
