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

void APU::tick(int cycles) {
    // increment counters
}

/* SQUARE WAVE 1 (0x4000 - 0x4003) */
void pulse1Ctrl(uint8_t val) {
    pulse1Duty = (val >> 6) & 0x3;
    pulse1LenCountHalt = !!(val & 0x20);
    pulse1EnvLoopEnable = pulse1LenCountHalt;
    pulse1ConstVol = !!(val & 0x10);
    pulse1Vol = val & 0xF;
    pulse1EnvPeriod = pulse1Vol + 1;
}

void pulse1RampCtrl(uint8_t) {}
void pulse1FineTune(uint8_t) {}
void pulse1CoarseTune(uint8_t) {}
/* SQUARE WAVE 2 (0x4004 - 0x4007) */
void pulse2Ctrl(uint8_t) {}
void pulse2RampCtrl(uint8_t) {}
void pulse2FineTune(uint8_t) {}
void pulse2CoarseTune(uint8_t) {}
/* TRIANGLE WAVE (0x4008 - 0x400B) */
void triangleCtrl1(uint8_t) {}
void triangleCtrl2(uint8_t) {}
void triangleFreq1(uint8_t) {}
void triangleFreq2(uint8_t) {}
/* NOISE GENERATOR (0x400C - 0x400F) */
void noiseCtrl1(uint8_t) {}
void noiseCtrl2(uint8_t) {}
void noiseFreq1(uint8_t) {}
void noiseFreq2(uint8_t) {}
/* DELTA MODULATION (0x4010 - 0x4013) */
void dmcCtrl(uint8_t) {}
void dmcDA(uint8_t) {}
void dmcAddr(uint8_t) {}
void dmcLen(uint8_t) {}

uint8_t APU::status() {
    /* IF-D NT21, DMC interr (I), frame interr (F), DMC active (D), length counter > 0 (N/T/2/1) */
    
    // for all channels but dmc, if length counter > 0 then return true
    uint8_t status = 0;
    status |= dmcInterrupt << 7;
    status |= frameInterrupt << 6;
    // set if dmc bytes remaining in buffer is > 1
    // TODO
    status |= (noiseLenCount > 0) << 3;
    status |= (triangleLenCount > 0) << 2;
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
    dmcEnable = !!(val & 0x10);
    noiseEnable = !!(val & 0x08);
    triangleEnable = !!(val & 0x04);
    pulse2Enable = !!(val & 0x02);
    pulse1Enable = !!(val & 0x01);

    // reset length counters
    if (!noiseEnable) {
	noiseLenCount = 0;
    }
    if (!triangleEnable) {
	triangleLenCount = 0;
    }
    if (!pulse1Enable) {
	pulse1LenCount = 0;
    }
    if (!pulse2Enable) {
	pulse2LenCount = 0;
    }

    // clear dmc interrupt flag
    dmcInterrupt = false;

    if (dmcEnable) {
	// restart sample only if buffer empty
	// TODO
    }
    else {
	// flush buffer and silence it
	// TODO
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
    // TODO

    if (mode == FIVE_STEP)
    {
	// clock once
    }
}
