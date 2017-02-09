#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <memory>
#include <vector>

#include <Cart.h>
#include <Mirroring.h>
#include <Mapper.h>
#include <Mapper0.h>
#include <Mapper1.h>

bool Cart::loadFile(std::string romFileName) {
    std::ifstream romFileStream(romFileName.c_str(), std::ios::binary);
    
    char iNesHeader[16];
    romFileStream.read(iNesHeader, 16);

    if(!verifyINesHeaderSignature(iNesHeader)) {
	printf("ERROR: Invalid iNES header\n");
	return false;
    }

    int mapperNum = (iNesHeader[6] >> 4) + (iNesHeader[7] & 0xF0);
    if (!initializeMapper(mapperNum)) {
	printf("ERROR: Mapper %d not yet supported\n", mapperNum);
	return false;
    }

    int prgSize = iNesHeader[4] * PRG_BANK_SIZE;
    for(int i  = 0; i < prgSize; ++i) {
	mapper->prg.push_back(romFileStream.get());
    }

    int chrSize = iNesHeader[5] * CHR_BANK_SIZE;
    mapper->chrIsRam = (chrSize == 0);
    if (mapper->chrIsRam) {
	mapper->chr.resize(CHR_BANK_SIZE);
    }
    else {
	for(int i  = 0; i < chrSize; ++i) {
	    mapper->chr.push_back(romFileStream.get());
	}
    }

    int ramSize  = iNesHeader[8] * RAM_BANK_SIZE;
    bool isRamBattery = !!(iNesHeader[6] & (0x1 << 1));
    mapper->ram.resize(ramSize);
    if (isRamBattery) {
        printf("ERROR: loading battery backed ram not yet supported\n");
    }

    bool isTrainer = !!(iNesHeader[6] & (0x1 << 2));
    if (isTrainer) {
	for(int i  = 0; i < TRAINER_SIZE; ++i) {
	    mapper->trainer.push_back(romFileStream.get());
	}
    }

    mapper->mirroring = MIRROR_HORIZONTAL;
    if (iNesHeader[6] & 0x1) {
        mapper->mirroring = MIRROR_VERTICAL;
    }
    if (iNesHeader[6] & (0x1 << 3)) {
        mapper->mirroring = MIRROR_FOUR_SCREEN;
	printf("Warning: four screen mirroring not yet supported");
    }
    
    romFileStream.close();
    
    return true;
}

bool Cart::verifyINesHeaderSignature(char* iNesHeader) {
    return (iNesHeader[0] == 'N' &&
	    iNesHeader[1] == 'E' &&
	    iNesHeader[2] == 'S' &&
	    iNesHeader[3] == 0x1A);
}

bool Cart::initializeMapper(int mapperNum) {
    switch(mapperNum) {
    case 0:
	mapper = std::unique_ptr<Mapper>(new Mapper0);
	return true;
    case 1:
    	mapper = std::unique_ptr<Mapper>(new Mapper1);
    	return true;
    default:
        return false;
    }
}

Mirroring Cart::getMirroring() {
    return mapper->mirroring;
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
