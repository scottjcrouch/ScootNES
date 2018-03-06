#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

#include <Cart.h>
#include <CartMemory.h>
#include <Mirroring.h>
#include <Mapper.h>
#include <mappers/Mapper0.h>
#include <mappers/Mapper1.h>

void Cart::loadFile(std::string romFileName) {
    std::ifstream romFileStream(romFileName.c_str(), std::ios::binary);
    
    std::vector<char> iNesHeader = getINesHeaderFromFile(romFileStream);
    bool isValidHeader = verifyINesHeaderSignature(iNesHeader);
    if(!isValidHeader) {
        throw std::runtime_error("Invalid iNES header");
    }

    CartMemory mem = getCartMemoryFromFile(iNesHeader, romFileStream);

    romFileStream.close();

    int mapperNum = getMapperNumberFromHeader(iNesHeader);
    if (!initializeMapper(mapperNum, mem)) {
	throw std::runtime_error(std::string("Mapper %d not yet supported", mapperNum));
    }
}

std::vector<char> Cart::getINesHeaderFromFile(std::ifstream& romFileStream) {
    std::vector<char> header;
    for(int i  = 0; i < 16; ++i) {
	header.push_back(romFileStream.get());
    }
    
    return header;
}

bool Cart::verifyINesHeaderSignature(std::vector<char> iNesHeader) {
    return (iNesHeader[0] == 'N' &&
	    iNesHeader[1] == 'E' &&
	    iNesHeader[2] == 'S' &&
	    iNesHeader[3] == 0x1A);
}

CartMemory Cart::getCartMemoryFromFile(std::vector<char> iNesHeader, std::ifstream& romFileStream) {
    CartMemory mem;
    
    int prgSize = iNesHeader[4] * PRG_BANK_SIZE;
    for(int i  = 0; i < prgSize; ++i) {
	mem.prg.push_back(romFileStream.get());
    }

    int chrSize = iNesHeader[5] * CHR_BANK_SIZE;
    mem.chrIsRam = (chrSize == 0);
    if (mem.chrIsRam) {
	mem.chr.resize(CHR_BANK_SIZE);
    }
    else {
	for(int i  = 0; i < chrSize; ++i) {
	    mem.chr.push_back(romFileStream.get());
	}
    }

    int ramSize  = iNesHeader[8] * RAM_BANK_SIZE;
    if (ramSize == 0) {
	ramSize = RAM_BANK_SIZE;
    }
    bool isRamBattery = !!(iNesHeader[6] & (0x1 << 1));
    mem.ram.resize(ramSize);
    if (isRamBattery) {
        throw std::runtime_error("Loading battery backed ram not yet supported");
    }

    bool isTrainer = !!(iNesHeader[6] & (0x1 << 2));
    if (isTrainer) {
	for(int i  = 0; i < TRAINER_SIZE; ++i) {
	    mem.trainer.push_back(romFileStream.get());
	}
    }

    mem.mirroring = MIRROR_HORIZONTAL;
    if (iNesHeader[6] & 0x1) {
        mem.mirroring = MIRROR_VERTICAL;
    }
    if (iNesHeader[6] & (0x1 << 3)) {
        mem.mirroring = MIRROR_FOUR_SCREEN;
        throw std::runtime_error("Four screen mirroring not yet supported");
    }

    return mem;
}

int Cart::getMapperNumberFromHeader(std::vector<char> iNesHeader) {
    return (iNesHeader[6] >> 4) + (iNesHeader[7] & 0xF0);
}

bool Cart::initializeMapper(int mapperNum, CartMemory mem) {
    switch(mapperNum) {
    case 0:
	mapper = std::unique_ptr<Mapper>(new Mapper0(mem));
	return true;
    case 1:
    	mapper = std::unique_ptr<Mapper>(new Mapper1(mem));
    	return true;
    default:
        return false;
    }
}

Mirroring Cart::getMirroring() {
    return mapper->getMirroring();
}

uint8_t Cart::readPrg(uint16_t addr) {
    return mapper->readPrg(addr);
}

void Cart::writePrg(uint16_t addr, uint8_t value) {
    mapper->writePrg(addr, value);
}

uint8_t Cart::readChr(uint16_t addr) {
    return mapper->readChr(addr);
}

void Cart::writeChr(uint16_t addr, uint8_t value) {
    mapper->writeChr(addr, value);
}
