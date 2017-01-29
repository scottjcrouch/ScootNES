#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <memory>

#include <Cart.h>

bool Cart::loadFile(std::string romFileName) {
    std::ifstream romFile(romFileName.c_str(), std::ios::binary);

    char header[16];

    // verify header
    romFile.read(header, 16);
    if (header[0] != 'N' ||
        header[1] != 'E' ||
        header[2] != 'S' ||
        header[3] != 0x1A) {
        printf("ERROR: Invalid iNES header\n");
        return false;
    }

    // read header data
    prgLen = header[4];
    chrLen = header[5];
    ramLen = header[8];
    mirroring = MIRROR_HOR;
    if (header[6] & (0x1 << 3)) {
        mirroring = MIRROR_NONE;
    }
    else if (header[6] & 0x1) {
        mirroring = MIRROR_VERT;
    }
    isRamBattery = !!(header[6] & (0x1 << 1));
    isTrainer = !!(header[6] & (0x1 << 2));
    mapperNum = (header[6] >> 4) + (header[7] & 0xF0);
    if (mapperNum != 0) {
        printf("ERROR: Mapper %d not yet supported\n", mapperNum);
        return false;
    }

    // load trainer data
    if (isTrainer) {
        romFile.read((char *)trainer, 512);
    }

    // load program data
    prg = std::unique_ptr<uint8_t[]>(new uint8_t[prgLen * PRG_BANK_SIZE]);
    romFile.read((char*)prg.get(), prgLen * PRG_BANK_SIZE);

    // load pattern/character data
    chr = std::unique_ptr<uint8_t[]>(new uint8_t[CHR_BANK_SIZE]);
    // if chrLen is 0 it is simply an empty bank that 
    // the program will normally copy data to from prg 
    // using ppuADDR/ppuDATA
    // TODO!!!!
    if (chrLen != 1) {
        printf("ERROR: chrLen %d not yet supported\n", chrLen);
        return false;
    }
    romFile.read((char*)chr.get(), chrLen * CHR_BANK_SIZE);

    // load ram data
    ram = std::unique_ptr<uint8_t[]>(new uint8_t[RAM_BANK_SIZE]);
    if (isRamBattery) {
        printf("ERROR: loading battery backed ram not yet supported\n");
        return false;
    }

    romFile.close();

    return true;
}

uint8_t Cart::readPrg(int index) {
    index %= prgLen * PRG_BANK_SIZE;
    return prg[index];
}

void Cart::writePrg(int index, uint8_t value) {
    // TODO: mapper registers
    printf("Tried to write to PRG-ROM, mapper functionality incomplete\n");
}

uint8_t Cart::readChr(int index) {
    return chr[index];
}

void Cart::writeChr(int index, uint8_t value) {
    // check if chr is writeable
    if (chrLen == 0) {
        chr[index] = value;
    }
    else {
        printf("ERROR: Tried to write 0x%X to CHR-ROM: 0x%X\n", value, index);
    }
}

uint8_t Cart::readRam(int index) {
    if(index > RAM_BANK_SIZE) {
        printf("ERROR: PRG-RAM access out of bounds\n");
    }
    return ram[index];
}

void Cart::writeRam(int index, uint8_t value) {
    if(index > RAM_BANK_SIZE) {
        printf("ERROR: PRG-RAM access out of bounds\n");
    }
    ram[index] = value;
}
