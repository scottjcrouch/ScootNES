#include <memory>

#include <Console.h>
#include <CPU.h>
#include <APU.h>
#include <PPU.h>
#include <Cart.h>
#include <Controller.h>

void Console::boot() {
    ppu = std::unique_ptr<PPU>(new PPU(&cart));
    cpu = std::unique_ptr<CPU>(new CPU(ppu.get(), &cart, &apu, &controller1));
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
