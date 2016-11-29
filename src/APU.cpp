#include <stdint.h>

#include <Console.h>
#include <APU.h>

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
    uint8_t status = 0;
    status |= dmcInterrupt << 7;
    status |= frameInterrupt << 6;
    // set if dmc bytes remaining in buffer is > 1
    // ......
    status |= (noiseLenCount > 0) << 3;
    status |= (triLenCount > 0) << 2;
    status |= (pulse2LenCount > 0) << 1;
    status |= (pulse1LenCount > 0);

    // clear frame interrupt flag (NOTE: if interrupt flag set at same
    // time as read, would normally return true and not be cleared)
    frameInterrupt = false;
    
    return status;
}

void APU::ctrl(uint8_t val) {
    /* ---D NT21, Enable DMC (D), noise (N), triangle (T), and pulse channels (2/1) */

    // enable/disable channels
    enableDMC = !!(val & 0x10);
    enableNoise = !!(val & 0x08);
    enableTriangle = !!(val & 0x04);
    enablePulse2 = !!(val & 0x02);
    enablePulse1 = !!(val & 0x01);

    // reset length counters
    if (!enableNoise) {
	noiseLenCount = 0;
    }
    if (!enableTriangle) {
	triLenCount = 0;
    }
    if (!enablePulse2) {
	pulse1LenCount = 0;
    }
    if (!enablePulse1) {
	pulse2LenCount = 0;
    }

    // clear dmc interrupt flag
    dmcInterrupt = false;

    if (enableDMC) {
	// restart sample only if buffer empty
	// ......
    }
    else {
	// flush buffer and silence it
	// ......
    }
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
