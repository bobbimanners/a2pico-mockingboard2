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

#include "wdc6522.h"
#include "ay-3-8913.h"

#define MOCKINGBOARD

extern const __attribute__((aligned(4))) uint8_t firmware[];

volatile bool active;
volatile bool reset;

static void __time_critical_func(handler)(bool asserted) {
    if (asserted) {
        a2pico_irq(false);
        active = false;
    } else {
        reset = true;
    }
}

void __time_critical_func(board)(void) {

    // Two 6522 VIAs and two General Instruments AY-3 sound chips
    via_state *via_1 = create_via();
    via_state *via_2 = create_via();
    ay3_state *ay3_1 = create_ay3();
    ay3_state *ay3_2 = create_ay3();

    a2pico_init(pio0);

    a2pico_resethandler(&handler);

    while (true) {
        uint32_t pico = a2pico_getaddr(pio0);
        uint32_t addr = pico & 0x0FFF;
        uint32_t io   = pico & 0x0F00;  // IOSTRB or IOSEL
        uint32_t strb = pico & 0x0800;  // IOSTRB
        uint32_t read = pico & 0x1000;  // R/W

        uint32_t data = a2pico_getdata(pio0);

#ifdef MOCKINGBOARD

        // Mockingboard addressing
        // A3..A0 are wired to RS3..RS0 to select VIA registers
        // via_1: CS1 = A7
        // via_2: CS1 = A7'
        uint8_t rs = addr & 0x000F;
        if (!io) { // DEVSEL
            via_clk(via_1, (addr & 0x0080) != 0, false, !read, rs, data);
            via_clk(via_2, (addr & 0x0080) == 0, false, !read, rs, data);
        } else {
            // Not selected but must clock them anyhow to advance timers
            via_clk(via_1, false, false, !read, rs, data);
            via_clk(via_2, false, false, !read, rs, data);
        }
        ay3_clk(ay3_1, via_1);
        ay3_clk(ay3_2, via_2);

#else
        if (read) {
            if (!io) {  // DEVSEL
                switch (addr & 0xF) {
                    case 0x0:
                        a2pico_putdata(pio0, sio_hw->fifo_rd);
                        break;
                    case 0x1:
                        // SIO_FIFO_ST_VLD_BITS _u(0x00000001)
                        // SIO_FIFO_ST_RDY_BITS _u(0x00000002)
                        a2pico_putdata(pio0, sio_hw->fifo_st << 6);
                        break;
                }
            } else {
                if (!strb || (active && (addr != 0x0FFF))) {
                    a2pico_putdata(pio0, firmware[addr]);
                }
            }
        } else {
            uint32_t data = a2pico_getdata(pio0);
            if (!io) {  // DEVSEL
                switch (addr & 0xF) {
                    case 0x0:
                        sio_hw->fifo_wr = data;
                        break;
                    case 0x1:
                        a2pico_irq(false);
                        break;
                }
            }
        }

        if (io && !strb) {
            active = true;
        } else if (addr == 0x0FFF) {
            active = false;
        }
#endif        
    }
}
