//
// Emulation of General Instruments AY-3-8913 Sound Chip
// Bobbi Webber-Manners
// Sept 2024
//

#pragma once

#include <stdint.h>
#include "wdc6522.h"

#define AY3_SAMPLES 4096           // Number of samples to buffer
#define CLOCKSPEED 1020500         // Host CPU clock
#define AY3_SAMPLERATE (CLOCKSPEED/16)

//
// We generate a sample of output every 16 clocks
// 1020500/16 -> 63.78kHz ouput
// If we drop every 4th sample -> 47.84kHz which is very close to 48kHz
//

// State of AY-3-8913
typedef struct {
  uint8_t regs[16];
  uint8_t selected; // Selected register

  // Output buffer
  uint16_t *output;
  unsigned int bufsize;

  // Write index into output buffers
  unsigned int idx;

  // Interal state of tone generator
  struct {
    unsigned int period[3];   // Period in terms of CLOCKSPEED/16
    unsigned int counter[3];  // Count remaining until flip
    unsigned int signal[3];   // Current signal state high or low
  } tone_state;

  // Interal state of noise generator
  struct {
    unsigned int period;      // Period in terms of CLOCKSPEED/16
    unsigned int counter;     // Count remaining until next random value
    unsigned int signal;      // Current signal state high or low
  } noise_state;

  unsigned int mixed[3];      // Mix of tone & noise

  // Internal state of envelope generator
  struct {
    unsigned int remaining;
    unsigned int period_counter;
    uint8_t      envelope_value;
  } envelope_state;
} ay3_state;

// Create an instance of AY-3-8913
// Param: h      - AY3 handle
//        buffer - pointer to output buffer
//        sz     - size of buffer (must be even number of bytes)
// Returns an AY3 handle
void init_ay3(ay3_state *h, uint16_t *buffer, unsigned int sz);

// Called on every clock
// Params: h     - AY3 handle
//         via   - VIA handle of connected VIA
void ay3_clk(ay3_state *h, via_state *via);


