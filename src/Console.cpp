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

uint32_t *Console::getFrameBuffer() {
    return ppu->getFrameBuffer();
}

void Console::runForOneFrame() {
    do {
        tick();
    } while (!ppu->endOfFrame());

    if (ppu->isNmiEnabled()) {
	cpu->signalNMI();
    }
}

void Console::tick() {
    cpuDivider.tick();
    if (cpuDivider.hasClocked()) {
	cpu->tick();
    }
    ppu->tick();
}
