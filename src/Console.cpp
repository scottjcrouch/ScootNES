#include <stdint.h>
#include <stdio.h>
#include <algorithm>

#include <Console.h>
#include <CPU.h>
#include <APU.h>
#include <PPU.h>
#include <Cart.h>
#include <Controller.h>

Console::~Console() {
    delete cpu, ppu;
}

void Console::boot() {
    ppu = new PPU(this);
    cpu = new CPU(&cart, ppu, &apu, &controller1);

    cpuDivider.setInterval(3);
}

void Console::runForOneFrame() {
    while(true) {
        tick();
	if (ppu->endOfFrame()) {
	    if (ppu->nmiOnVBlank) {
		cpu->signalNMI();
	    }
	    break;
	}
    }
}

void Console::tick() {
    cpuDivider.tick();
    if (cpuDivider.hasClocked()) {
	cpu->tick();
    }
    ppu->tick();
}

uint32_t *Console::getFrameBuffer() {
    return ppu->getFrameBuffer();
}
