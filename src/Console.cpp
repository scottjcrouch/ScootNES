#include <string>
#include <functional>
#include <vector>

#include <Console.h>
#include <CPU.h>
#include <APU.h>
#include <PPU.h>
#include <Cart.h>
#include <Controller.h>
#include <Divider.h>

void Console::boot() {
    ReadBus cpuReadCallback = [this] (uint16_t addr) {
	return cpuRead(addr);
    };
    WriteBus cpuWriteCallback = [this] (uint16_t addr, uint8_t data) {
	cpuWrite(addr,data);
    };
    NMI nmiCallback = [this] () {cpu.signalNMI();};

    cpu.boot(cpuReadCallback, cpuWriteCallback);
    ppu.boot(&cart, nmiCallback);
}

void Console::loadINesFile(std::string fileName) {
    return cart.loadFile(fileName);
}

uint32_t *Console::getFrameBuffer() {
    return ppu.getFrameBuffer();
}

std::vector<short> Console::getAvailableSamples() {
    return apu.getAvailableSamples();
}

void Console::runForOneFrame() {
    do {
        tick();
    } while (!ppu.endOfFrame());
    apu.endFrame();
}

void Console::tick() {
    cpuDivider.tick();
    if (cpuDivider.hasClocked()) {
	cpu.tick();
    }
    ppu.tick();
}

uint8_t Console::cpuRead(uint16_t addr) {
    if (addr < 0x2000) {
        return cpuRam[addr % 0x800];
    }
    else if (addr < 0x4000) {
        int registerAddr = addr & 0x2007;
        switch (registerAddr) {
        case 0x2000: return cpuBusMDR;
        case 0x2001: return cpuBusMDR;
        case 0x2002: return ppu.getSTATUS();
        case 0x2003: return cpuBusMDR;
        case 0x2004: return ppu.getOAMDATA();
        case 0x2005: return cpuBusMDR;
        case 0x2006: return cpuBusMDR;
        case 0x2007: return ppu.getDATA();
        }
    }
    else if (addr < 0x4018) {
	switch (addr) {
        case 0x4015: return apu.getStatus();
	case 0x4016: return controller1.poll();
	case 0x4017: return 0; // TODO: controller 2
        }
    }
    else if (addr < 0x4020) {
	return cpuBusMDR; // disabled/unused APU test registers
    }
    else {
        return cart.readPrg(addr);
    }
}

void Console::cpuWrite(uint16_t addr, uint8_t value) {
    cpuBusMDR = value;
    if (addr < 0x2000) {
        cpuRam[addr % 0x800] = value;
    }
    else if (addr < 0x4000) {
        int registerAddr = addr & 0x2007;
        switch (registerAddr) {
        case 0x2000: ppu.setCTRL(value); break;
        case 0x2001: ppu.setMASK(value); break;
        case 0x2002: break; // ppu status, read-only
        case 0x2003: ppu.setOAMADDR(value); break;
        case 0x2004: ppu.setOAMDATA(value); break;
        case 0x2005: ppu.setSCROLL(value); break;
        case 0x2006: ppu.setADDR(value); break;
        case 0x2007: ppu.setDATA(value); break;
        }
    }
    else if (addr < 0x4018) {
	switch (addr) {
	case 0x4014: {
	    uint16_t startAddr = ((uint16_t)value) << 8;
	    for (int i = 0; i < 256; ++i) {
		ppu.setOAMDATA(cpuRead(startAddr + i));
	    }
	    cpu.suspend(514);
	} break;
	case 0x4016: controller1.setStrobe(!!(value & 0x1)); break;
	default: apu.writeRegister(addr, value); break;
	}
    }
    else if (addr < 0x4020) {
	return; // disabled/unused APU test registers
    }
    else {
        cart.writePrg(addr, value);
    }
}
