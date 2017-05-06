#include <string>
#include <functional>

#include <Console.h>
#include <CPU.h>
#include <APU.h>
#include <PPU.h>
#include <Cart.h>
#include <Controller.h>
#include <Divider.h>

void Console::boot() {
    cpu.boot([this] (uint16_t addr) {return cpuRead(addr);},
	     [this] (uint16_t addr, uint8_t data) {cpuWrite(addr,data);});
    ppu.boot(&cart, [this] () {cpu.signalNMI();});
}

bool Console::loadINesFile(std::string fileName) {
    return cart.loadFile(fileName);
}

uint32_t *Console::getFrameBuffer() {
    return ppu.getFrameBuffer();
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
        int port = addr % 8;
        switch (port) {
        case 0: // 0x2000
            return openBus;
        case 1: // 0x2001
            return openBus;
        case 2: // 0x2002
            return ppu.getSTATUS();
        case 3: // 0x2003
            return openBus;
        case 4: // 0x2004
            return ppu.getOAMDATA();
        case 5: // 0x2005
            return openBus;
        case 6: // 0x2006
            return openBus;
        case 7: // 0x2007
            return ppu.getDATA();
        }
    }
    else if (addr < 0x4020) {
	switch (addr) {
        case 0x4015:
	    return apu.getStatus();
	case 0x4016:
            return controller1.poll();
	case 0x4017: // TODO: controller 2
            return 0;
	default: // disabled/unused APU test registers
            return openBus;
        }
    }
    else {
        return cart.readPrg(addr);
    }
}

void Console::cpuWrite(uint16_t addr, uint8_t value) {
    openBus = value;
    if (addr < 0x2000) {
        cpuRam[addr % 0x800] = value;
    }
    else if (addr < 0x4000) {
        int port = addr % 8;
        switch (port) {
        case 0: // 0x2000
            ppu.setCTRL(value);
            break;
        case 1: // 0x2001
            ppu.setMASK(value);
            break;
        case 2: // 0x2002
            break;
        case 3: // 0x2003
            ppu.setOAMADDR(value);
            break;
        case 4: // 0x2004
            ppu.setOAMDATA(value);
            break;
        case 5: // 0x2005
            ppu.setSCROLL(value);
            break;
        case 6: // 0x2006
            ppu.setADDR(value);
            break;
        case 7: // 0x2007
            ppu.setDATA(value);
            break;
        }
    }
    else if (addr < 0x4020) {
	if (addr == 0x4014) {
	    uint16_t startAddr = ((uint16_t)value) << 8;
	    for (int i = 0; i < 256; ++i) {
		ppu.setOAMDATA(cpuRead(startAddr + i));
	    }
	    cpu.suspend(514);
	}
	else if (addr == 0x4016) {
	    controller1.setStrobe(!!(value & 0x1));
	}
	else if (addr < 0x4018) {
	    apu.writeRegister(addr, value);
	}
    }
    else {
        cart.writePrg(addr, value);
    }
}
