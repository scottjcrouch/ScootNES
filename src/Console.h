#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>

#include <Cart.h>
#include <Controller.h>
#include <CPU.h>
#include <PPU.h>
#include <APU.h>

class Divider {
public:
    Divider(int interval = 1) {
	setInterval(interval);
    }
    void setInterval(int interval) {
	this->interval = interval;
	clockCounter = interval - 1;
    }
    void tick() {
	clockCounter++;
	if (clockCounter == interval) {
	    clockCounter = 0;
	}
    }
    bool hasClocked() {
	return clockCounter == 0;
    }

private:
    int interval;
    int clockCounter;
};

class Console {
public:
    Console() : cpuDivider(3) { }
    void boot();
    bool loadINesFile(std::string fileName);
    uint32_t *getFrameBuffer();
    void runForOneFrame();

    Cart cart;
    Controller controller1;
    APU apu;

private:
    void tick();

    PPU ppu;
    CPU cpu;
    Divider cpuDivider;
};

#endif
