#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <memory>

#include <Cart.h>
#include <Mapper.h>
#include <Mapper0.h>

bool Cart::loadFile(std::string romFileName) {
    std::ifstream romFileStream(romFileName.c_str(), std::ios::binary);
    
    char iNesHeader[16];
    romFileStream.read(iNesHeader, 16);

    if(!verifyINesHeaderSignature(iNesHeader)) {
	printf("ERROR: Invalid iNES header\n");
	return false;
    }

    loadINesHeaderData(iNesHeader);
    
    allocateCartMemory();

    loadCartMemoryFromFile(romFileStream);

    if (isRamBattery) {
        printf("ERROR: loading battery backed ram not yet supported\n");
        return false;
    }

    if (!initializeMapper()) {
	printf("ERROR: Mapper %d not yet supported\n", mapperNum);
    }

    mapper->doStuff();
    
    romFileStream.close();
    
    return true;
}

bool Cart::verifyINesHeaderSignature(char* iNesHeader) {
    return (iNesHeader[0] == 'N' &&
	    iNesHeader[1] == 'E' &&
	    iNesHeader[2] == 'S' &&
	    iNesHeader[3] == 0x1A);
}

void Cart::loadINesHeaderData(char* iNesHeader) {
    prgSize = iNesHeader[4] * PRG_BANK_SIZE;

    chrSize = iNesHeader[5] * CHR_BANK_SIZE;
    chrIsSingleRamBank = (chrSize == 0);

    ramSize  = iNesHeader[8] * RAM_BANK_SIZE;

    isRamBattery = !!(iNesHeader[6] & (0x1 << 1));
    
    mirroring = MIRROR_HOR;
    if (iNesHeader[6] & (0x1 << 3)) {
        mirroring = MIRROR_NONE;
    }
    else if (iNesHeader[6] & 0x1) {
        mirroring = MIRROR_VERT;
    }
    
    isTrainer = !!(iNesHeader[6] & (0x1 << 2));
    
    mapperNum = (iNesHeader[6] >> 4) + (iNesHeader[7] & 0xF0);
}

void Cart::allocateCartMemory() {
    prg = std::unique_ptr<uint8_t[]>(new uint8_t[prgSize]);

    if (chrIsSingleRamBank) {
	chr = std::unique_ptr<uint8_t[]>(new uint8_t[CHR_BANK_SIZE]);
    }
    else {
	chr = std::unique_ptr<uint8_t[]>(new uint8_t[chrSize]);
    }
    
    ram = std::unique_ptr<uint8_t[]>(new uint8_t[ramSize]);     
}

void Cart::loadCartMemoryFromFile(std::ifstream& romFileStream) {
    romFileStream.read((char*)prg.get(), prgSize);
    romFileStream.read((char*)chr.get(), chrSize);
    if (isTrainer) {
        romFileStream.read((char *)trainer, TRAINER_SIZE);
    }
}

bool Cart::initializeMapper() {
    switch(mapperNum) {
    case 0:
	mapper = std::unique_ptr<Mapper>(new Mapper0);
	return true;
    default:
        return false;
    }
}

uint8_t Cart::readPrg(uint16_t addr) {
    int index = addr % prgSize;
    return prg[index];
}

void Cart::writePrg(uint16_t addr, uint8_t value) {
    printf("Tried to write to PRG-ROM, mapper functionality incomplete\n");
}

uint8_t Cart::readChr(uint16_t addr) {
    return chr[addr];
}

void Cart::writeChr(uint16_t addr, uint8_t value) {
    if (chrIsSingleRamBank) {
        chr[addr] = value;
    }
    else {
        printf("ERROR: Tried to write 0x%X to CHR-ROM: 0x%X\n", value, addr);
    }
}
