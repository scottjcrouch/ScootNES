#include <cstdint>

#include <APU.h>

APU::APU() {
    frameCounter(0xFF);
    ctrl(0);
}

void APU::tick(int cycles) {
    // TODO: increment counters
}

void APU::pulse1Ctrl(uint8_t val) {
    /* Bitfield: DDLC VVVV
       Duty (D)
       envelope loop/length counter halt (L)
       constant volume/disable envelope (C)
       volume/envelope period (V) */
    
    pulse1Duty = (val >> 6) & 0x3;
    pulse1LenCountHalt = !!(val & 0x20);
    pulse1EnvLoopEnable = pulse1LenCountHalt;
    pulse1EnvDisable = !!(val & 0x10);
    pulse1Vol = val & 0xF;
    pulse1EnvPeriod = pulse1Vol + 1;
}

void APU::pulse1RampCtrl(uint8_t val) {
    /* Bitfield: EPPP NSSS
       sweep enable (E)
       sweep period (P)
       sweep negate (N)
       sweep shift (S) */
    
    pulse1SweepEnable = !!(val & 0x80);
    pulse1SweepPeriod = ((val >> 4) & 0x7) + 1;
    pulse1SweepNegate = !!(val & 0x8);
    pulse1SweepShift = val & 0x7;
    pulse1SweepReload = true;
}

void APU::pulse1FineTune(uint8_t val) {
    /* Bitfield: TTTT TTTT
       Timer low (T) */

    pulse1Timer &= 0x0700;
    pulse1Timer |= val & 0xFF;
}

void APU::pulse1CoarseTune(uint8_t val) {
    /* Bitfield: LLLL LTTT
       Length counter load (L)
       timer high (T) */

    pulse1Timer &= 0x00FF;
    pulse1Timer |= (((int)val & 0x07) << 8);
    // TODO: set divider period without restarting it
    pulse1LenCountLoad = val >> 3;
    if (pulse1Enable) {
	pulse1LenCount = lengthTable[pulse1LenCountLoad];
    }
    pulse1EnvStart = true;
    // TODO: restart duty cycle sequence
}

void APU::pulse2Ctrl(uint8_t val) {
    /* Bitfield: DDLC VVVV
       Duty (D)
       envelope loop/length counter halt (L)
       constant volume/disable envelope (C)
       volume/envelope period (V) */
}

void APU::pulse2RampCtrl(uint8_t val) {
    /* Bitfield: EPPP NSSS
       sweep enable (E)
       sweep period (P)
       sweep negate (N)
       sweep shift (S) */
}

void APU::pulse2FineTune(uint8_t val) {
    /* Bitfield: TTTT TTTT
       Timer low (T) */
}

void APU::pulse2CoarseTune(uint8_t val) {
    /* Bitfield: LLLL LTTT
       Length counter load (L)
       timer high (T) */
}

void APU::triangleCtrl1(uint8_t val) {
    /* Bitfield: CRRR RRRR
       Length counter halt / linear counter control (C)
       linear counter load (R) */
}

void APU::triangleCtrl2(uint8_t val) {
    /* Unused */
}

void APU::triangleFreq1(uint8_t val) {
    /* Bitfield: TTTT TTTT
       Timer low (T) */
}

void APU::triangleFreq2(uint8_t val) {
    /* Bitfield: LLLL LTTT
       Length counter load (L)
       timer high (T) */
}

void APU::noiseCtrl1(uint8_t val) {
    /* Bitfield: --LC VVVV
       Envelope loop / length counter halt (L)
       constant volume (C)
       volume/envelope (V) */
}

void APU::noiseCtrl2(uint8_t val) {
    /* Unused */
}

void APU::noiseFreq1(uint8_t val) {
    /* Bitfield: L--- PPPP
       Loop noise (L)
       noise period (P) */
}

void APU::noiseFreq2(uint8_t val) {
    /* Bitfield: LLLL L---
       Length counter load (L) */
}

void APU::dmcCtrl(uint8_t val) {
    /* Bitfield: IL-- RRRR
       IRQ enable (I)
       loop (L)
       frequency (R) */
}

void APU::dmcDA(uint8_t val) {
    /* Bitfield: -DDD DDDD
       Load counter (D) */
}

void APU::dmcAddr(uint8_t val) {
    /* Bitfield: AAAA AAAA
       Sample address (A) */
}

void APU::dmcLen(uint8_t val) {
    /* Bitfield: LLLL LLLL
       Sample length (L) */
}

uint8_t APU::status() {
    /* Bitfield: IF-D NT21
       DMC interrupt (I)
       frame interrupt (F)
       DMC active (D)
       length counter > 0 (N/T/2/1) */
    
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
    /* Bitfield: ---D NT21
       Enable DMC (D)
       Enable Noise (N)
       Enable Triangle (T)
       Enable Pulse Channels (2/1) */

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
    /* Bitfield: MI-- ----
       Mode (M, 0 = 4-step, 1 = 5-step)
       IRQ inhibit flag (I) */

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
