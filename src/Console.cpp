#include <Console.h>
#include <CPU.h>
#include <APU.h>
#include <PPU.h>
#include <Cart.h>
#include <Controller.h>

void Console::boot() {
    ppu.boot(&cart, &cpu);
    cpu.boot(&ppu, &cart, &apu, &controller1);
}

uint32_t *Console::getFrameBuffer() {
    return ppu.getFrameBuffer();
}

void Console::runForOneFrame() {
    do {
        tick();
    } while (!ppu.endOfFrame());
}

void Console::tick() {
    cpuDivider.tick();
    if (cpuDivider.hasClocked()) {
	cpu.tick();
    }
    ppu.tick();
}
