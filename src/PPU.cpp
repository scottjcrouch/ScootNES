#include <PPU.h>
#include <Console.h>
#include <stdint.h>

PPU::PPU(Console *console) {
    this->console = console;
}

void PPU::boot() {
    // 0x2000: CTRL
    nameTableAddr = 0;              // 0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00
    vRAMAddrIncr = false;           // 0=1 (across name table), 1=32 (down name table)
    sprPatternTableAddr = false;    // 0=0x0000, 1=0x1000, ignored if sprSize is 8x16
    bgPatternTableAddr = false;     // 0=0x0000, 1=0x1000
    sprSize = false;                // 0=8x8, 1=8x16
    // unused bit
    nmiOnVBlank = false;

    // 0x2001: MASK
    grayscale = false;              // mirrors palette to only shades of grey
    imageMask = false;              // show bg in left 8 columns of screen
    sprMask = false;                // show sprites in left 8 columns of screen
    showBg = false;
    showSpr = false;
    buffRed = false;                // dims non red colours
    buffGreen = false;              // dims non green colours
    buffBlue = false;               // dims non blue colours

    // 0x2002: STATUS
    // 5 unused bits
    sprOverflow = false;            // if > 8 sprites on any scanline in current frame
    spr0Hit = false;                // at first non-zero pixel overlap in both spr0 and bg
    isVBlank = false;               // set during VBlank

    // 0x2003: OAMADDR
    oamAddrBuffer = 0x00;

    // 0x2004: OAMDATA
    oamReadBuffer = 0x00;

    // 0x2005: SCROLL
    scrollX = 0x00;
    scrollY = 0x00;

    // 0x2006: ADDR
    addrBuffer = 0x0000;

    // 0x2007: DATA
    readBuffer = 0x00;

    // latch used for SCROLL, ADDR. Unset upon STATUS read
    latch = false;
}

void PPU::enterVBlank() {
    isVBlank = true;
}

void PPU::exitVBlank() {
    isVBlank = false;
}

void PPU::setCTRL(uint8_t value) {
    nameTableAddr = value & 0x03;
    vRAMAddrIncr = !!(value & 0x04);
    sprPatternTableAddr = !!(value & 0x08);
    bgPatternTableAddr = !!(value & 0x10);
    sprSize = !!(value & 0x20);
    nmiOnVBlank = !!(value & 0x80);
}

void PPU::setMASK(uint8_t value) {
    grayscale = !!(value & 0x01);
    imageMask = !!(value & 0x02);
    sprMask = !!(value & 0x04);
    showBg = !!(value & 0x08);
    showSpr = !!(value & 0x10);
    buffRed = !!(value & 0x20);
    buffGreen = !!(value & 0x40);
    buffBlue = !!(value & 0x80);
}

uint8_t PPU::getSTATUS() {
    uint8_t result = 0x00;
    if (sprOverflow) {
        result |= 0x20;
    }
    if (spr0Hit) {
        result |= 0x40;
    }
    if (isVBlank) {
        result |= 0x80;
    }
    isVBlank = false;
    latch = false;
    return result;
}

void PPU::setOAMADDR(uint8_t value) {
    oamAddrBuffer = value;
}

void PPU::setOAMDATA(uint8_t value) {
    oamReadBuffer = value;
    console->oamWrite(oamAddrBuffer++, value);
}

uint8_t PPU::getOAMDATA() {
    // apparently unreliable in NES hardware, but some games use it
    return console->oamRead(oamAddrBuffer);
}

void PPU::setSCROLL(uint8_t value) {
    if (!latch) {
        scrollX = value;
    }
    else {
        scrollY = value;
    }
    latch = !latch;
}

void PPU::setADDR(uint8_t value) {
    if (!latch) {
        // set high byte
        addrBuffer &= 0x00FF;
        addrBuffer |= ((uint16_t)value << 8);
    }
    else {
        // set low byte
        addrBuffer &= 0xFF00;
        addrBuffer |= value;
    }
    latch = !latch;
}

void PPU::setDATA(uint8_t value) {
    console->ppuWrite(addrBuffer, value);
    addrBuffer += ((vRAMAddrIncr) ? 32 : 1);
}

uint8_t PPU::getDATA() {
    uint8_t returnVal;
    if (addrBuffer < 0x3F00) {
        // return what's in the read buffer from the previous
        // read, then load data at current address buffer
        // (before incrementing the address) into the read buffer
        returnVal = readBuffer;
        readBuffer = console->ppuRead(addrBuffer);
    }
    else {
        // when the address buffer points to the palette address range,
        // instead of reading from the read buffer, it returns the 
        // data at the immediate address, and stores the name table
        // data that would otherwise be mirrored "underneath" the 
        // palette address space in the read buffer
        returnVal = console->ppuRead(addrBuffer);
        readBuffer = console->ppuRead(addrBuffer - 0x1000);
    }
    addrBuffer += ((vRAMAddrIncr) ? 32 : 1);
    return returnVal;
}
