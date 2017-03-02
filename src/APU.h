#ifndef APU_H
#define APU_H

#include <cstdint>

#include <Divider.h>

static const int lengthTable[0x20] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

class APU {
public:
    APU();

    void tick(int);

    /** Mapped access registers **/

    // square wave 1 (0x4000 - 0x4003)
    void pulse1Ctrl(uint8_t);
    void pulse1RampCtrl(uint8_t);
    void pulse1FineTune(uint8_t);
    void pulse1CoarseTune(uint8_t);
    // square wave 2 (0x4004 - 0x4007)
    void pulse2Ctrl(uint8_t);
    void pulse2RampCtrl(uint8_t);
    void pulse2FineTune(uint8_t);
    void pulse2CoarseTune(uint8_t);
    // triangle wave (0x4008 - 0x400B)
    void triangleCtrl1(uint8_t);
    void triangleCtrl2(uint8_t);
    void triangleFreq1(uint8_t);
    void triangleFreq2(uint8_t);
    // noise generator (0x400C - 0x400F)
    void noiseCtrl1(uint8_t);
    void noiseCtrl2(uint8_t);
    void noiseFreq1(uint8_t);
    void noiseFreq2(uint8_t);
    // delta modulation (0x4010 - 0x4013)
    void dmcCtrl(uint8_t);
    void dmcDA(uint8_t);
    void dmcAddr(uint8_t);
    void dmcLen(uint8_t);
    // status (0x4015)
    uint8_t status();
    void ctrl(uint8_t);
    // frame counter (0x4017)
    void frameCounter(uint8_t);

private:
    enum ApuMode {
	FOUR_STEP,
	FIVE_STEP,
    };

    ApuMode mode;

    bool inhibitIrq;
    bool frameInterrupt;
    bool dmcInterrupt;

    bool dmcEnable;
    bool noiseEnable;
    bool triangleEnable;
    bool pulse1Enable;
    bool pulse2Enable;

    int noiseLenCount;
    int triangleLenCount;
    int pulse1LenCount;
    int pulse2LenCount;

    int pulse1Duty;
    bool pulse1LenCountHalt;
    bool pulse1EnvLoopEnable;
    bool pulse1EnvDisable;
    int pulse1Vol;
    int pulse1EnvPeriod;

    bool pulse1SweepEnable;
    int pulse1SweepPeriod;
    bool pulse1SweepNegate;
    int pulse1SweepShift;
    bool pulse1SweepReload;

    int pulse1Timer;

    int pulse1LenCountLoad;
    bool pulse1EnvStart;

    Divider pulse1Divider;
};

#endif
