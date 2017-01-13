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
    delete cpu, ppu, apu;
}

void Console::boot() {
    apu = new APU(this);
    ppu = new PPU(this);
    cpu = new CPU(&cart, ppu, apu, &controller1);

    cpuDivider.setInterval(3);
}

void Console::runForOneFrame() {
    do {
        tick();
    } while (!ppu->endOfFrame());
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
