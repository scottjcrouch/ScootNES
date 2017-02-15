#include <cstdint>

#include <Mapper0.h>

uint8_t Mapper0::readPrg(uint16_t addr) {
    int index = addr % cartMemory.prg.size();
    return cartMemory.prg[index];
}

void Mapper0::writePrg(uint16_t addr, uint8_t value) { }

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
