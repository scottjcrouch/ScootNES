#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <algorithm>

#include <Cart.h>

Cart::~Cart() {
    delete trainer, prg, chr, ram;
}

bool Cart::readFile(std::string romFileName) {
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
    trainer = new uint8_t[512];
    std::fill_n(trainer, 512, 0);
    if (isTrainer) {
        romFile.read((char *)trainer, 512);
    }

    // load program data
    prg = new uint8_t[prgLen * PRG_BANK_SIZE];
    std::fill_n(prg, prgLen * PRG_BANK_SIZE, 0);
    romFile.read((char *)prg, prgLen * PRG_BANK_SIZE);

    // load pattern/character data
    chr = new uint8_t[CHR_BANK_SIZE];
    std::fill_n(chr, CHR_BANK_SIZE, 0);
    // if chrLen is 0 it is simply an empty bank that 
    // the program will normally copy data to from prg 
    // using ppuADDR/ppuDATA
    // TODO!!!!
    if (chrLen != 1) {
        printf("ERROR: chrLen %d not yet supported\n", chrLen);
        return false;
    }
    romFile.read((char *)chr, chrLen * CHR_BANK_SIZE);

    // load ram data
    ram = new uint8_t[RAM_BANK_SIZE];
    std::fill_n(ram, RAM_BANK_SIZE, 0);
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
