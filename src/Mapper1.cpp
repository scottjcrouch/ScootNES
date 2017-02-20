#include <cstdint>

#include <CartMemory.h>
#include <Mapper1.h>

Mapper1::Mapper1(CartMemory mem) : Mapper(mem) {
    updateBankAddresses();
}

uint8_t Mapper1::readPrg(uint16_t addr) {
    if (addr >= 0x8000) {
	int index = decodePrgRomAddress(addr);
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

void Mapper1::writePrg(uint16_t addr, uint8_t value) {
    if (addr >= 0x8000) {
	if (value & 0x80) {
	    shiftRegister = 0x10;
	}
	else {
	    if (shiftRegister & 1) {
		shiftRegister >>= 1;
		shiftRegister |= ((value << 4) & 0x10);
		loadRegister(addr, shiftRegister);
		shiftRegister = 0x10;
	    }
	    else {
		shiftRegister >>= 1;
		shiftRegister |= ((value << 4) & 0x10);
	    }
	}
    }
    else if (addr >= 0x6000) {
	int index = addr % 0x2000;
	cartMemory.ram[index] = value;
    }
}

uint8_t Mapper1::readChr(uint16_t addr) {
    int index = decodeChrRomAddress(addr);
    return cartMemory.chr[index];
}

void Mapper1::writeChr(uint16_t addr, uint8_t value) {
    if (cartMemory.chrIsRam) {
	int index = addr % cartMemory.chr.size();
        cartMemory.chr[index] = value;
    }
}

void Mapper1::loadRegister(uint16_t addr, uint8_t value) {
    if (addr >= 0xE000) {
	prgRomBank = value & 0b01111;
	prgRamDisable = !!(value & 0b10000);
    }
    else if (addr >= 0xC000) {
	chrRomBank1 = value & 0x1F;
    }
    else if (addr >= 0xA000) {
	chrRomBank0 = value & 0x1F;
    }
    else if (addr >= 0x8000) {
	switch(value & 0b00011) {
	case 0: cartMemory.mirroring = Mirroring::MIRROR_LOWER_BANK; break;
	case 1: cartMemory.mirroring = Mirroring::MIRROR_UPPER_BANK; break;
	case 2: cartMemory.mirroring = Mirroring::MIRROR_VERTICAL;   break;
	case 3: cartMemory.mirroring = Mirroring::MIRROR_HORIZONTAL; break;
	}
	switch((value & 0b01100) >> 2) {
	case 0:
	case 1: prgMode = PrgMode::PRG_32KB;           break;
	case 2: prgMode = PrgMode::PRG_FIX_FIRST_16KB; break;
	case 3: prgMode = PrgMode::PRG_FIX_LAST_16KB;  break;
	}
	switch((value & 0b10000) >> 4) {
	case 0: chrMode = ChrMode::CHR_8KB; break;
	case 1: chrMode = ChrMode::CHR_4KB; break;
	}
    }
    updateBankAddresses();
}

void Mapper1::updateBankAddresses() {
    switch (prgMode) {
    case PrgMode::PRG_32KB:
	prg16kBankAddresses[0] = (prgRomBank & 0xFFFE) * 0x4000;
	prg16kBankAddresses[1] = (prgRomBank | 1) * 0x4000;
	break;
    case PrgMode::PRG_FIX_FIRST_16KB:
	prg16kBankAddresses[0] = 0;
	prg16kBankAddresses[1] = prgRomBank * 0x4000;
	break;
    case PrgMode::PRG_FIX_LAST_16KB:
	prg16kBankAddresses[0] = prgRomBank * 0x4000;
	prg16kBankAddresses[1] = cartMemory.prg.size() - 0x4000;
	break;
    }

    switch(chrMode) {
    case ChrMode::CHR_8KB:
	chr4kBankAddresses[0] = (chrRomBank0 & 0xFFFE) * 0x1000;
	chr4kBankAddresses[1] = (chrRomBank0 | 1) * 0x1000;
	break;
    case ChrMode::CHR_4KB:
	chr4kBankAddresses[0] = chrRomBank0 * 0x1000;
	chr4kBankAddresses[1] = chrRomBank1 * 0x1000;
	break;
    }
}

int Mapper1::decodePrgRomAddress(uint16_t addr) {
    int bankNum = (addr & 0x4000) >> 14;
    int offset = (addr % 0x4000);
    return prg16kBankAddresses[bankNum] + offset;
}

int Mapper1::decodeChrRomAddress(uint16_t addr) {
    int bankNum = (addr & 0x1000) >> 12;
    int offset = (addr % 0x1000);
    return chr4kBankAddresses[bankNum] + offset;
}
