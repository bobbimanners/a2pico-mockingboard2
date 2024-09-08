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

#include "ay-3-8913.h"
#include "wdc6522.h"
#include "btstack/bt_audio.h"

#define MOCKINGBOARD

#define BUFSZ (4096 * 2)

#include "board.h"

void main(void) {

    uint8_t audiobuf[BUFSZ];

    // TODO: Bluetooth expects 16 bit ints it seems.
    set_shared_audio_buffer((unsigned char*)audiobuf);

    via_state *via_1 = create_via(registers1);
    via_state *via_2 = create_via(registers2);
    ay3_state *ay3_1 = create_ay3(audiobuf + 0, BUFSZ);
    ay3_state *ay3_2 = create_ay3(audiobuf + 1, BUFSZ);

    multicore_launch_core1(board);

    btstack_main(0, NULL);

#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif

    stdio_init_all();

    while (!stdio_usb_connected()) {
    }

    printf("\n\nCopyright (c) 2022 Oliver Schmidt (https://a2retro.de/)\n\n");
    printf("\n\nCopyright (c) 2024 Bobbi Webber-Manners a2pico-mockingboard proj\n\n");

    // Start time
    absolute_time_t t = get_absolute_time();

    while (true) {

#ifdef MOCKINGBOARD

        uint32_t write; // 0 for read, 1 for write
        uint32_t addr; // Address 0..0xFF
        uint32_t rs;   // Register select 0..15
        uint32_t data; // Databus
        bool via1_sel; // Chip select for VIA1
        bool via2_sel; // Chip select for VIA2
        bool rwb;      // True for write, false for read

        if (multicore_fifo_rvalid()) {
            // 6502 has read from or written to a register.
            addr     = multicore_fifo_pop_blocking();
            data     = multicore_fifo_pop_blocking();
            via1_sel = ((addr & 0x80) != 0); // A7 selects VIA1 or VIA2
            via2_sel = !via1_sel;
            rs       = addr & 0x0f;
            rwb      = ((addr & 0x8000000) != 0); // MSbit set if write
        } else {
            // No reads or writes. Deselect both VIAs.
            via1_sel = via2_sel = false;
        }

        via_clk(via_1, via1_sel, rwb, rs, data);
        via_clk(via_2, via2_sel, rwb, rs, data);
        ay3_clk(ay3_1, via_1);
        ay3_clk(ay3_2, via_2);

        // Timer to achieve 1.0000 MHz overall
        t = delayed_by_us(t, 1);
        sleep_until(t);
        
        if (reset) {
            // Reset VIAs and AYs
            reset = false;
        }

#else

        if (stdio_usb_connected()) {
            if (multicore_fifo_rvalid()) {
                // Data available from other core
                putchar(multicore_fifo_pop_blocking());
            }
        }

        int data = getchar_timeout_us(0);
        if (data != PICO_ERROR_TIMEOUT) {
            if (data == 27) {
                a2pico_irq(true);
            }
            else if (multicore_fifo_wready()) {
                // Able to write to other core
                multicore_fifo_push_blocking(data);
            } else {
                putchar('\a');
            }
        }

        if (reset) {
            reset = false;
            printf(" RESET ");
        }
#endif

#ifdef PICO_DEFAULT_LED_PIN
        gpio_put(PICO_DEFAULT_LED_PIN, active);
#endif
    }
}
