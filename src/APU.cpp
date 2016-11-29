#include <APU.h>
#include <stdint.h>
#include <Console.h>

APU::APU(Console *console) {
    this->console = console;
}

void APU::boot() {
    frameCounter(0xFF);
    ctrl(0);
}

void APU::clock(int cycles) {
    // increment counters
}

uint8_t APU::status() {
    /* IF-D NT21, DMC interr (I), frame interr (F), DMC active (D), length counter > 0 (N/T/2/1) */
    // for all channels but dmc, if length counter > 0 then return true
    // dmc returns true if bytes remaining in buffer > 0
    // for any read, clear frame interrupt flag, but not dmc interrupt flag
    // if interrupt flag set at same time as read, will return true and not be cleared
    return 0;
}

void APU::ctrl(uint8_t val) {
    /* ---D NT21, Enable DMC (D), noise (N), triangle (T), and pulse channels (2/1) */
    // if disable channel, set corresponding length counter to 0
    // if disable dmc, flush buffer and silence it
    // if enable dmc, restart sample only if buffer empty
    // for any write, clear dmc interrupt flag
}

void APU::frameCounter(uint8_t val) {
    /* MI-- ----, Mode (M, 0 = 4-step, 1 = 5-step), IRQ inhibit flag (I) */

    mode = (val & 0x80) ? FIVE_STEP : FOUR_STEP;
    inhibitIrq = !!(val & 0x40);
    
    if (inhibitIrq) {
	frameInterrupt = false;
    }

    // reset divider and sequences
    // ......

    if (mode == FIVE_STEP)
    {
	// clock once
    }
}
