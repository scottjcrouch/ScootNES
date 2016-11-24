#include <stdint.h>
#include <stdio.h>
#include <algorithm>

#include <Console.h>
#include <CPU.h>
#include <APU.h>
#include <PPU.h>
#include <Cart.h>
#include <Controller.h>
#include <Renderer.h>

Console::Console(Cart *cart, Controller *contr1) {
    this->cart = cart;
    this->contr1 = contr1;
}

Console::~Console() {
    delete cpu, ppu, apu, renderer;
}

void Console::boot() {
    cpu = new CPU(this);
    ppu = new PPU(this);
    apu = new APU(this);
    renderer = new Renderer(this);

    cpu->boot();
    ppu->boot();
    apu->boot();

    std::fill_n(cpuRAM, 0x800, 0);
    std::fill_n(sprRAM, 0x100, 0);
    std::fill_n(nameTables, 0x1000, 0);
    uint8_t paletteOnBoot[] = {
	0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D,
	0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C,
	0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14,
	0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08};
    std::copy(paletteOnBoot, paletteOnBoot + 0x20, paletteRAM);

    openBus = 0;

    currentCycle = VBLANK;
    instrCount = 0;
    frameCount = 0;
    nmiSignal = false;
    irqSignal = false;
    vbLatch = true;
}

void Console::runFrame() {
    // spin for 1 frame, until the end of VBlank
    while (true) {
        if (currentCycle >= POST_REND) {
            if (currentCycle >= CYC_PER_FRAME) {
                ppu->exitVBlank();
                currentCycle -= CYC_PER_FRAME;
                vbLatch = true;
                break;
            }
            else if (currentCycle >= PRE_REND) {
                ppu->exitVBlank();
            }
            else if ((currentCycle >= VBLANK) && vbLatch) {
                vbLatch = false;
                ppu->enterVBlank();
                if (ppu->nmiOnVBlank) {
                    cpu->signalNMI();
                }
            }
        }
        // cpu ticks at 1/3 of the master clock rate
        currentCycle += cpu->executeNextOp() * 3;
        ++instrCount;
    }
    ++frameCount;
    renderer->load();
    renderer->renderFrame();
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
            return contr1->poll();
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
            oamDMA(data);
        }
        else if (addr == 0x4015) {
            // TODO: APU
        }
        else if (addr == 0x4016) {
            contr1->setStrobe(!!(data & 0x1));
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

uint8_t Console::ppuRead(uint16_t addr) {
    if (addr >= 0x4000) {
        printf("Address out of bounds %d\n", addr);
        addr %= 0x4000;
    }
    if (addr >= 0x3F00) {
        uint16_t index = addr % 0x20;
        return paletteRAM[index];
    }
    else if (addr >= 0x2000) {
        uint16_t index = addr % 0x1000;
        return nameTables[index];
    }
    else {
        return cart->readChr(addr);
    }
}

void Console::ppuWrite(uint16_t addr, uint8_t data) {
    if (addr >= 0x4000) {
        printf("Address out of bounds %d\n", addr);
        addr %= 0x4000;
    }
    if (addr >= 0x3F00) {
        uint16_t index = addr % 0x20;
        paletteRAM[index] = data;
        if (addr % 4 == 0) {
            paletteRAM[index ^ 0x10] = data;
        }
    }
    else if (addr >= 0x2000) {
        uint16_t index = addr % 0x1000;
        nameTables[index] = data;
        if (cart->mirroring == MIRROR_VERT) {
            nameTables[index ^ 0x800] = data;
        }
        else if (cart->mirroring == MIRROR_HOR) {
            nameTables[index ^ 0x400] = data;
        }
        else if (cart->mirroring == MIRROR_ALL) {
            nameTables[index ^ 0x800] = data;
            nameTables[index ^ 0x400] = data;
            nameTables[(index ^ 0x400) ^ 0x800] = data;
        }
    }
    else {
        cart->writeChr(addr, data);
    }
}

uint8_t Console::oamRead(uint8_t index) {
    return sprRAM[index];
}

void Console::oamWrite(uint8_t index, uint8_t data) {
    // the third byte of every sprite entry is missing bits
    // in hardware, so zero them here before writing
    if (index % 4 == 2) {
        data &= 0xE3;
    }
    sprRAM[index] = data;
}

void Console::oamDMA(uint8_t offset) {
    uint16_t start = (uint16_t)offset;
    start <<= 8;
    for (int i = 0; i < 256; ++i) {
        oamWrite(i, cpuRead(start + i));
    }
    cpu->tick(514);
}

uint8_t *Console::getPalettePointer() {
    return paletteRAM;
}

uint8_t *Console::getNameTablePointer() {
    return nameTables;
}

uint8_t *Console::getPatternTablePointer() {
    return cart->getChrPointer();
}

uint8_t *Console::getSprRamPointer() {
    return sprRAM;
}

uint32_t *Console::getFrame() {
    return renderer->getFrame();
}
