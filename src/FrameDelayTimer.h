#ifndef FRAMEDELAYTIMER_H
#define FRAMEDELAYTIMER_H

#include <cstdint>

/*
 * Notes:
 *
 * - SDL_Timer wraps if running for more than ~49 days.
 *
 * - If a period of time greater than intervalSize has elapsed between
 * one call to delay() and the previous one, the following delay()
 * call will not shorten its delay to play catch up.
 */

class FrameDelayTimer
{
public:
    FrameDelayTimer(uint32_t intervalSize = 1000);
    void delay();

private:
    uint32_t intervalSize;
    uint32_t endOfNextInterval;

    uint32_t getCurrentTime();
};

#endif
