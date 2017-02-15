#include <cstdint>

#include <Mapper0.h>

uint8_t Mapper0::readPrg(uint16_t addr) {
    if (addr >= 0x8000) {
	int index = addr % cartMemory.prg.size();
	return cartMemory.prg[index];
    }
    else if (addr >= 0x6000) {
	int index = addr % 0x2000;
	return cartMemory.ram[index];
    }
    else {
	return 0;
    }
}

uint8_t Mapper0::readChr(uint16_t addr) {
    int index = addr % cartMemory.chr.size();
    return cartMemory.chr[index];
}

void Mapper0::writeChr(uint16_t addr, uint8_t value) {
    if (cartMemory.chrIsRam) {
	int index = addr % cartMemory.chr.size();
        cartMemory.chr[index] = value;
    }
}
