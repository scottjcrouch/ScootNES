#include <cstdint>

#include <FrameDelayTimer.h>
#include <SDL.h>

FrameDelayTimer::FrameDelayTimer(uint32_t intervalSize) {
    this->intervalSize = intervalSize;
    endOfNextInterval = getCurrentTime() + intervalSize;
}

uint32_t FrameDelayTimer::getCurrentTime() {
    return SDL_GetTicks();
}

void FrameDelayTimer::delay() {
    uint32_t currentTime = getCurrentTime();
    int timeLeft = (int)endOfNextInterval - (int)currentTime;
    if (timeLeft > 0) {
	SDL_Delay(timeLeft);
    }
    endOfNextInterval = currentTime + intervalSize;
}
