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
    cpu = new CPU(this);
    ppu = new PPU(this);
    apu = new APU(this);

    cpu->boot();
    ppu->boot();
    apu->boot();

    std::fill_n(cpuRAM, 0x800, 0);

    openBus = 0;

    cpuDivider = 0;
}

void Console::runForOneFrame() {
    do {
        tick();
    } while (!ppu->endOfFrame());
}

void Console::tick() {
    if (!cpuDivider) {
	cpu->tick();
    }
    ppu->tick();
    cpuDivider = (cpuDivider + 1) % 3;
}

uint8_t Console::cpuRead(uint16_t addr) {
    if (addr < 0x2000) {
        return cpuRAM[addr % 0x800];
    }
    else if (addr < 0x4000) {
        int port = addr % 8;
        switch (port) {
        case 0: // 0x2000
            return openBus;
        case 1: // 0x2001
            return openBus;
        case 2: // 0x2002
            return ppu->getSTATUS();
        case 3: // 0x2003
            return openBus;
        case 4: // 0x2004
            return ppu->getOAMDATA();
        case 5: // 0x2005
            return openBus;
        case 6: // 0x2006
            return openBus;
        case 7: // 0x2007
            return ppu->getDATA();
        default:
            printf("Invalid port %d\n", port);
            return 0;
        }
    }
    else if (addr < 0x4020) {
        if (addr <= 0x4013) {
            // TODO: APU
            return 0;
        }
        else if (addr == 0x4014) {
            // write only oam dma register
            return openBus;
        }
        else if (addr == 0x4015) {
            // TODO: APU
            return 0;
        }
        else if (addr == 0x4016) {
            return controller1->poll();
        }
        else if (addr == 0x4017) {
            // TODO: controller 2 and APU
            return 0;
        }
        else {
            // disabled/unused APU test registers
            return openBus;
        }
    }
    else if (addr < 0x6000) {
        // TODO: expansion rom
        return 0;
    }
    else if (addr < 0x8000) {
        return cart->readRam(addr - 0x6000);
    }
    else {
        return cart->readPrg(addr - 0x8000);
    }
}

void Console::cpuWrite(uint16_t addr, uint8_t data) {
    openBus = data;
    if (addr < 0x2000) {
        cpuRAM[addr % 0x800] = data;
    }
    else if (addr < 0x4000) {
        int port = addr % 8;
        switch (port) {
        case 0: // 0x2000
            ppu->setCTRL(data);
            break;
        case 1: // 0x2001
            ppu->setMASK(data);
            break;
        case 2: // 0x2002
            break;
        case 3: // 0x2003
            ppu->setOAMADDR(data);
            break;
        case 4: // 0x2004
            ppu->setOAMDATA(data);
            break;
        case 5: // 0x2005
            ppu->setSCROLL(data);
            break;
        case 6: // 0x2006
            ppu->setADDR(data);
            break;
        case 7: // 0x2007
            ppu->setDATA(data);
            break;
        default:
            printf("Invalid port %d\n", port);
        }
    }
    else if (addr < 0x4020) {
        if (addr <= 0x4013) {
            // TODO: APU
        }
        else if (addr == 0x4014) {
            ppu->oamDMA(data);
        }
        else if (addr == 0x4015) {
            // TODO: APU
        }
        else if (addr == 0x4016) {
            controller1->setStrobe(!!(data & 0x1));
        }
        else if (addr == 0x4017) {
            // TODO: controller 2 and APU
        }
        else {
            // disabled/unused APU test registers
        }
    }
    else if (addr < 0x6000) {
        // TODO: expansion rom
    }
    else if (addr < 0x8000) {
        cart->writeRam(addr - 0x6000, data);
    }
    else {
        printf("Illegal write to %d\n", data);
    }
}

uint32_t *Console::getFrameBuffer() {
    return ppu->getFrameBuffer();
}
