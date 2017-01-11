#ifndef CONSOLE_H
#define CONSOLE_H

#include <Cart.h>
#include <Controller.h>
#include <CPU.h>
#include <PPU.h>
#include <APU.h>

class Divider {
public:
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
  Console(Cart *cart, Controller *controller1);
  ~Console();
  void boot();
  void runForOneFrame();
  uint32_t *getFrameBuffer();

  void tick();
  
  Cart *cart;
  Controller *controller1;
  CPU *cpu;
  PPU *ppu;
  APU *apu;

  Divider cpuDivider;
};

#endif
