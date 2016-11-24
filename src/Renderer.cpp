#include <stdint.h>

#include <Renderer.h>
#include <Console.h>

void Palette::init(uint8_t *palettePointer) {
    this->palettePointer = palettePointer;
}

uint8_t Palette::getValue(int index) {
    if (index % 4 == 0) {
        return *palettePointer;
    }
    else {
        return *(palettePointer + index);
    }
}

void Metatile::init(uint8_t *attribute) {
    this->attribute = attribute;
}

uint8_t Metatile::getValue(uint8_t quadrant) {
    return ((*attribute) >> (quadrant * 2)) & 0b11;
}

void Tile::init(Metatile *metaTile, uint8_t quadrant, uint8_t *patternIndex, uint8_t *patternTable, bool *ptOffset) {
    this->metaTile = metaTile;
    this->quadrant = quadrant;
    this->patternIndex = patternIndex;
    this->patternTable = patternTable;
    this->ptOffset = ptOffset;
}

static uint8_t getBit(uint8_t byte, int index) {
    return (byte >> index) & 0x1;
}

uint8_t Tile::getValue(uint8_t x, uint8_t y) {
    uint8_t bit0 = getBit(pattern[y], 7 - x);
    uint8_t bit1 = getBit(pattern[y + 8], 7 - x);
    uint8_t index = bit0 | (bit1 << 1);
    uint8_t selector = metaTile->getValue(quadrant) << 2;
    return index | selector;
}

void Tile::reload() {
    int addr = *patternIndex * 16;
    if (*ptOffset) {
        addr += 0x1000;
    }
    pattern = patternTable + addr;
}

void Pixel::init(Tile *tile, uint8_t x, uint8_t y) {
    this->tile = tile;
    this->x = x;
    this->y = y;
}

uint8_t Pixel::getValue() {
    // get pixel's 4-bit index into its
    // 16 byte palette array
    return tile->getValue(x, y);
}

void Sprite::init(uint8_t *oamData, uint8_t *patternTable, bool *sprSize, bool *sprPatternTableAddr) {
    this->oamData = oamData;
    this->patternTable = patternTable;
    this->sprSize = sprSize;
    this->sprPatternTableAddr = sprPatternTableAddr;
}

void Sprite::reload() {
    xPos = oamData[3];
    yPos = oamData[0];
    visible = (xPos < 0xF9) && (yPos < 0xEF);
    yPos++;
    xBound = (int)xPos + 8;
    yBound = (int)yPos + (*sprSize ? 16 : 8);
        
    int ptOffset = 0;
    if (*sprSize) {
        // 8x16 sprite
        ptOffset = (oamData[1] & 0b11111110) * 16;
        if (oamData[1] & 0b00000001) {
            ptOffset += 0x1000;
        }
    }
    else {
        // 8x8 sprite
        ptOffset = oamData[1] * 16;
        if (*sprPatternTableAddr) {
            // stuff
            ptOffset += 0x1000;
        }
    }
    pattern = patternTable + ptOffset;
    
    paletteSelect = ((oamData[2] & 0b00000011) << 2) | 0x10;
    priority = !!(oamData[2] & 0b00100000);
    flipHor = !!(oamData[2] & 0b01000000);
    flipVert = !!(oamData[2] & 0b10000000);
}

uint8_t Sprite::getValue(uint8_t x, uint8_t y) {
    x -= xPos;
    y -= yPos;
    if (y >= 8) {
        y += 16;
    }
    uint8_t bitX = (flipHor ? x : 7 - x);
    uint8_t bitY = (flipVert ? 7 - y : y);
    uint8_t bit0 = getBit(pattern[bitY], bitX);
    uint8_t bit1 = getBit(pattern[bitY + 8], bitX);
    uint8_t index = bit0 | (bit1 << 1);
    return index | paletteSelect;
}

bool Sprite::occludes(uint8_t x, uint8_t y) {
    if (!visible ||
        x < xPos ||
        x >= xBound ||
        y < yPos ||
        y >= yBound) {
        return false;
    }
    return true;
}

Renderer::Renderer(Console *console) {
    this->console = console;

    // palette for background and sprite tiles
    palette.init(console->getPalettePointer());

    // background metatiles
    for (int nt = 0; nt < 4; ++nt) {
        for (int x = 0; x < 8; ++x) {
            for (int y = 0; y < 8; ++y) {
                // offset in nametable where attribute data is located
                uint16_t offset = 0x3C0 + (0x400 * nt) + (y * 8) + x;
                // init
                bgMetaTiles[nt][x][y].init(
                    console->getNameTablePointer() + offset);
            }
        }
    }
    // background tiles
    for (int nt = 0; nt < 4; ++nt) {
        for (int x = 0; x < 32; ++x) {
            for (int y = 0; y < 30; ++y) {
                // offset in name table where data is located
                uint16_t offset = (0x400 * nt) + x + (y * 32);
                // position within enclosing metatile
                uint8_t quadrant = 0;
                if ((x % 4) >= 2) {
                    quadrant |= 0b01;
                }
                if ((y % 4) >= 2) {
                    quadrant |= 0b10;
                }
                // coordinates of parent metatile
                uint8_t mtX = x / 4;
                uint8_t mtY = y / 4;
                // init
                bgTiles[nt][x][y].init(
                    &bgMetaTiles[nt][mtX][mtY],
                    quadrant,
                    console->getNameTablePointer() + offset,
                    console->getPatternTablePointer(),
                    &(console->ppu->bgPatternTableAddr));
            }
        }
    }
    // background pixels
    for (int x = 0; x < 512; ++x) {
        for (int y = 0; y < 480; ++y) {
            // find tile to which pixel belongs
            int nt = 0;
            if (x >= 256) {
                nt += 0b01;
            }
            if (y >= 240) {
                nt += 0b10;
            }
            int tileX = (x % 256) / 8;
            int tileY = (y % 240) / 8;
            // init
            bgPixels[x][y].init(
                &bgTiles[nt][tileX][tileY],
                x % 8,
                y % 8);
        }
    }

    // sprite objects
    for (int i = 0; i < 64; i++) {
        sprites[i].init(
            console->getSprRamPointer() + (i * 4),
            console->getPatternTablePointer(),
            &(console->ppu->sprSize),
            &(console->ppu->sprPatternTableAddr));
    }
}

void Renderer::load() {
    for (int nt = 0; nt < 4; ++nt) {
        for (int x = 0; x < 32; ++x) {
            for (int y = 0; y < 30; ++y) {
                bgTiles[nt][x][y].reload();
            }
        }
    }
    for (int i = 0; i < 64; ++i) {
        sprites[i].reload();
    }
    // TODO reload ppu data here such as scrollX and scrollY
    // reload sprites, due to oamaddr changing
}

void Renderer::renderPixel(int x, int y) {
    int realX = x;
    if (console->ppu->nameTableAddr & 0b01) {
        realX += 256;
    }
    realX += console->ppu->scrollX;
    realX %= 512;

    int realY = y;
    if (console->ppu->nameTableAddr & 0b10) {
        realY += 240;
    }
    realY += console->ppu->scrollY;
    realY %= 480;

    // render first occluding sprite that isn't transparent
    for (int i = 0; i < 64; ++i) {
        if (sprites[i].occludes(x, y)) {
            uint8_t paletteIndex = sprites[i].getValue(x, y);
            if (paletteIndex % 4 != 0) {
                uint8_t paletteVal = palette.getValue(paletteIndex);
                uint32_t pixelColour = universalPalette[paletteVal];
                frameBuffer[x + (y * FRAME_WIDTH)] = pixelColour;
                return;
            }
        }
    }

    // otherwise render background if not transparent
    uint8_t paletteIndex = bgPixels[realX][realY].getValue();
    if (paletteIndex % 4 != 0) {
        uint8_t paletteVal = palette.getValue(paletteIndex);
        uint32_t pixelColour = universalPalette[paletteVal];
        frameBuffer[x + (y * FRAME_WIDTH)] = pixelColour;
        return;
    }

    // otherwise render universal background colour
    frameBuffer[x + (y * FRAME_WIDTH)] = universalPalette[palette.getValue(0)];
}

void Renderer::renderFrame() {
    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            renderPixel(x, y);
            //if(!(x%8) || !(y%8))
            //    frameBuffer[x + (y * FRAME_WIDTH)] = 0xFF0000;
        }
    }
}

uint32_t *Renderer::getFrame()
{
    return frameBuffer;
}

const uint32_t Renderer::universalPalette[64] = {
    0x757575, 0x271B8F, 0x0000AB, 0x47009F, 0x8F0077, 0xAB0013,
    0xA70000, 0x7F0B00, 0x432F00, 0x004700, 0x005100, 0x003F17,
    0x1B3F5F, 0x000000, 0x000000, 0x000000, 0xBCBCBC, 0x0073EF,
    0x233BEF, 0x8300F3, 0xBF00BF, 0xE7005B, 0xDB2B00, 0xCB4F0F,
    0x8B7300, 0x009700, 0x00AB00, 0x00933B, 0x00838B, 0x000000,
    0x000000, 0x000000, 0xFFFFFF, 0x3FBFFF, 0x5F97FF, 0xA78BFD,
    0xF77BFF, 0xFF77B7, 0xFF7763, 0xFF9B3B, 0xF3BF3F, 0x83D313,
    0x4FDF4B, 0x58F898, 0x00EBDB, 0x444444, 0x000000, 0x000000,
    0xFFFFFF, 0xABE7FF, 0xC7D7FF, 0xD7CBFF, 0xFFC7FF, 0xFFC7DB,
    0xFFBFB3, 0xFFDBAB, 0xFFE7A3, 0xE3FFA3, 0xABF3BF, 0xB3FFCF,
    0x9FFFF3, 0xAAAAAA, 0x000000, 0x000000};
