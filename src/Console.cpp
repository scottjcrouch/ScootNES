#include <stdint.h>
#include <stdio.h>
#include <algorithm>

#include <Console.h>
#include <CPU.h>
#include <APU.h>
#include <PPU.h>
#include <Cart.h>
#include <Controller.h>

Console::Console(Cart *cart, Controller *controller1) {
    this->cart = cart;
    this->controller1 = controller1;
}

Console::~Console() {
    delete cpu, ppu, apu;
}

void Console::boot() {
    apu = new APU(this);
    apu->boot();
    ppu = new PPU(this);
    ppu->boot();
    cpu = new CPU(cart, ppu, apu, controller1);
    cpu->boot();

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
