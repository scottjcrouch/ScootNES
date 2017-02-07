#include <cstdint>

#include <Mapper0.h>

uint8_t Mapper0::readPrg(uint16_t addr) {
    int index = addr % prg.size();
    return prg[index];
}

void Mapper0::writePrg(uint16_t addr, uint8_t value) { }

uint8_t Mapper0::readChr(uint16_t addr) {
    int index = addr % chr.size();
    return chr[index];
}

void Mapper0::writeChr(uint16_t addr, uint8_t value) {
    if (chrIsSingleRamBank) {
	int index = addr % chr.size();
        chr[index] = value;
    }
}
