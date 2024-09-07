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

#include <a2pico.h>

#include "board.h"

extern const __attribute__((aligned(4))) uint8_t firmware[];

volatile bool active;
volatile bool reset;
volatile uint8_t registers1[16];
volatile uint8_t registers2[16];

static void __time_critical_func(handler)(bool asserted) {
    if (asserted) {
        a2pico_irq(false);
        active = false;
    } else {
        reset = true;
    }
}

void __time_critical_func(board)(void) {

    a2pico_init(pio0);

    a2pico_resethandler(&handler);

    while (true) {
        uint32_t pico = a2pico_getaddr(pio0);
        uint32_t addr = pico & 0x0FFF;
        uint32_t io   = pico & 0x0F00;  // IOSTRB or IOSEL
        uint32_t strb = pico & 0x0800;  // IOSTRB
        uint32_t read = pico & 0x1000;  // R/W
        uint32_t data = 0;

        if (read) {
            // 6502 read from card. 16 registers are supported.
            // Read cached registers (may be slightly delayed)
            if (!io) {  // DEVSEL
                if (addr & 0x80 != 0) {
                    // VIA1
                    a2pico_putdata(pio0, registers1[addr & 0x0F]);
                } else {
                    // VIA2
                    a2pico_putdata(pio0, registers2[addr & 0x0F]);
                }
                sio_hw->fifo_wr = 0; // Means READ
                sio_hw->fifo_wr = addr;
                sio_hw->fifo_wr = data;
            }

        } else {
            // 6502 write to card. 16 registers are supported.
            uint32_t data = a2pico_getdata(pio0);
            if (!io) { // DEVSEL
                sio_hw->fifo_wr = 1; // Means WRITE
                sio_hw->fifo_wr = addr;
                sio_hw->fifo_wr = data;
            }
        }

        if (io && !strb) {
            active = true;
        } else if (addr == 0x0FFF) {
            active = false;
        }
    }
}
