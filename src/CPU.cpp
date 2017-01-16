#include <stdint.h>
#include <stdlib.h>
#include <algorithm>

#include <CPU.h>
#include <Cart.h>
#include <PPU.h>
#include <APU.h>
#include <Controller.h>

CPU::CPU(PPU* ppu, Cart* cart, APU* apu, Controller* controller1) {
    this->ppu = ppu;
    this->cart = cart;
    this->apu = apu;
    this->controller1 = controller1;

    pc = 0x0000;
    sp = 0xFD;
    acc = 0x00;
    x = 0x00;
    y = 0x00;
    carry = 0;
    zero = 0;
    intdisable = 1;
    decmode = 0;
    brk = 0;
    overflow = 0;
    negative = 0;

    nmiSignal = 0;
    irqSignal = 0;

    targetAddr = loadAddr(RESET_VECTOR);
    operand16 = 0x0000;
    operand8 = 0x00;
    useAcc = 0;

    OpJMP();

    cyclesLeft = 0;

    openBus = 0;

    std::fill_n(cpuRAM, 0x800, 0);
}

uint8_t CPU::read(uint16_t addr) {
    if (addr < 0x2000) {
        return cpuRAM[addr % 0x800];
    }
    else if (addr < 0x4000) {
        int port = addr % 8;
        switch (port) {
        case 0: // 0x2000
            return openBus;
        case 1: // 0x2001
            return openBus;
        case 2: // 0x2002
            return ppu->getSTATUS();
        case 3: // 0x2003
            return openBus;
        case 4: // 0x2004
            return ppu->getOAMDATA();
        case 5: // 0x2005
            return openBus;
        case 6: // 0x2006
            return openBus;
        case 7: // 0x2007
            return ppu->getDATA();
        default:
            printf("Invalid port %d\n", port);
            return 0;
        }
    }
    else if (addr < 0x4020) {
        if (addr <= 0x4013) {
            // TODO: APU
            return 0;
        }
        else if (addr == 0x4014) {
            // write only oam dma register
            return openBus;
        }
        else if (addr == 0x4015) {
            // TODO: APU
            return 0;
        }
        else if (addr == 0x4016) {
            return controller1->poll();
        }
        else if (addr == 0x4017) {
            // TODO: controller 2 and APU
            return 0;
        }
        else {
            // disabled/unused APU test registers
            return openBus;
        }
    }
    else if (addr < 0x6000) {
        // TODO: expansion rom
        return 0;
    }
    else if (addr < 0x8000) {
        return cart->readRam(addr - 0x6000);
    }
    else {
        return cart->readPrg(addr - 0x8000);
    }
}

void CPU::write(uint16_t addr, uint8_t value) {
    openBus = value;
    if (addr < 0x2000) {
        cpuRAM[addr % 0x800] = value;
    }
    else if (addr < 0x4000) {
        int port = addr % 8;
        switch (port) {
        case 0: // 0x2000
            ppu->setCTRL(value);
            break;
        case 1: // 0x2001
            ppu->setMASK(value);
            break;
        case 2: // 0x2002
            break;
        case 3: // 0x2003
            ppu->setOAMADDR(value);
            break;
        case 4: // 0x2004
            ppu->setOAMDATA(value);
            break;
        case 5: // 0x2005
            ppu->setSCROLL(value);
            break;
        case 6: // 0x2006
            ppu->setADDR(value);
            break;
        case 7: // 0x2007
            ppu->setDATA(value);
            break;
        default:
            printf("Invalid port %d\n", port);
        }
    }
    else if (addr < 0x4020) {
        if (addr <= 0x4013) {
            // TODO: APU
        }
        else if (addr == 0x4014) {
	    uint16_t startAddr = ((uint16_t)value) << 8;
	    for (int i = 0; i < 256; ++i) {
		ppu->oamWrite(i, read(startAddr + i));
	    }
	    addCycles(514);
        }
        else if (addr == 0x4015) {
            // TODO: APU
        }
        else if (addr == 0x4016) {
            controller1->setStrobe(!!(value & 0x1));
        }
        else if (addr == 0x4017) {
            // TODO: controller 2 and APU
        }
        else {
            // disabled/unused APU test registers
        }
    }
    else if (addr < 0x6000) {
        // TODO: expansion rom
    }
    else if (addr < 0x8000) {
        cart->writeRam(addr - 0x6000, value);
    }
    else {
        printf("Illegal write to %d\n", value);
    }
}

void CPU::push8(uint8_t value) {
    write(sp | 0x0100, value);
    sp -= 1;
}

void CPU::push16(uint16_t value) {
    push8((value >> 8) & 0xFF);
    push8(value & 0xFF);
}

uint8_t CPU::pop8() {
    sp += 1;
    return read(sp | 0x0100);
}

uint16_t CPU::pop16() {
    uint8_t low = pop8();
    uint8_t high = pop8();
    return low | (high << 8);
}

uint16_t CPU::loadAddr(uint16_t addr) {
    uint8_t low = read(addr);
    uint8_t high = read(addr+1);
    return low | (high << 8);
}

uint8_t CPU::getStatus() {
    uint8_t flags =
        (carry      << 0) |
        (zero       << 1) |
        (intdisable << 2) |
        (decmode    << 3) |
        (brk        << 4) |
        (0x1        << 5) |
        (overflow   << 6) |
        (negative   << 7);
    return flags;
}

void CPU::setStatus(uint8_t value) {
    carry       = !!(value & 0x01);
    zero        = !!(value & 0x02);
    intdisable  = !!(value & 0x04);
    decmode     = !!(value & 0x08);
    brk         = !!(value & 0x10);
    // bit 5 unused
    overflow    = !!(value & 0x40);
    negative    = !!(value & 0x80);
}

void CPU::handleInterrupts() {
    if(nmiSignal) {
        push16(pc);
        push8(getStatus() & ~(0x20));
        intdisable = 1;
        pc = loadAddr(NMI_VECTOR);
        addCycles(7);
        nmiSignal = 0;
    }
    else if(irqSignal && !intdisable) {
        push16(pc);
        push8(getStatus() & ~(0x20));
        intdisable = 1;
        pc = loadAddr(IRQ_VECTOR);
        addCycles(7);
        irqSignal = 0;
    }
}

void CPU::branch() {
    // the offset is stored as a signed byte
    int8_t offset = (int8_t)read(targetAddr);
    uint16_t branchAddr = pc + offset;
    if((branchAddr & 0xFF00) != (pc & 0xFF00)) {
        // page boundary crossed
        addCycles(2);
    }
    else {
        addCycles(1);
    }
    pc = branchAddr;
}

/* Instructions */

void CPU::OpADC() {
    int memAdd = read(targetAddr);
    int tempAcc = acc + memAdd + (carry ? 1 : 0);
    zero = (tempAcc & 0xFF) == 0;
    negative = !!(tempAcc & 0x80);
    carry = (tempAcc >> 8) != 0;
    overflow = (((acc ^ tempAcc) & (memAdd ^ tempAcc)) & 0x80) != 0;
    acc = tempAcc & 0xFF;
}

void CPU::OpAND() {
    acc &= read(targetAddr);
    zero = (acc == 0);
    negative = !!(acc & 0x80);
}

void CPU::OpASL() {
    uint8_t result = 0;
    if(useAcc) {
        carry = !!(acc & 0x80);
        result = acc << 1;
        acc = result;
        useAcc = 0;
    }
    else {
        uint8_t target = read(targetAddr);
        carry = !!(target & 0x80);
        result = target << 1;
        write(targetAddr, result);
    }
    zero = (result == 0);
    negative = !!(result & 0x80);
}

void CPU::OpBCC() {
    if(!carry) {
        branch();
    }
}

void CPU::OpBCS() {
    if(carry) {
        branch();
    }
}

void CPU::OpBEQ() {
    if(zero) {
        branch();
    }
}

void CPU::OpBIT() {
    uint8_t fetched = read(targetAddr);
    uint8_t result = acc & fetched;
    zero = (result == 0);
    overflow = !!(fetched & 0x40);
    negative = !!(fetched & 0x80);
}

void CPU::OpBMI() {
    if(negative) {
        branch();
    }
}

void CPU::OpBNE() {
    if(!zero) {
        branch();
    }
}

void CPU::OpBPL() {
    if(!negative) {
        branch();
    }
}

void CPU::OpBRK() {
    push16(pc+1);
    push8(getStatus() | 0x10);
    intdisable = 1;
    pc = loadAddr(IRQ_VECTOR); 
}

void CPU::OpBVC() {
    if(!overflow) {
        branch();
    }
}

void CPU::OpBVS() {
    if(overflow) {
        branch();
    }
}

void CPU::OpCLC() {
    carry = 0;
}

void CPU::OpCLD() {
    decmode = 0;
}

void CPU::OpCLI() {
    intdisable = 0;
}

void CPU::OpCLV() {
    overflow = 0;
}

void CPU::OpCMP() {
    uint8_t fetched = read(targetAddr);
    uint8_t result = acc - fetched;
    carry = acc >= fetched;
    zero = acc == fetched;
    negative = !!(result & 0x80);
}

void CPU::OpCPX() {
    uint8_t fetched = read(targetAddr);
    uint8_t result = x - fetched;
    carry = x >= fetched;
    zero = x == fetched;
    negative = !!(result & 0x80);
}

void CPU::OpCPY() {
    uint8_t fetched = read(targetAddr);
    uint8_t result = y - fetched;
    carry = y >= fetched;
    zero = y == fetched;
    negative = !!(result & 0x80);
}

void CPU::OpDEC() {
    uint8_t result = read(targetAddr) - 1;
    write(targetAddr, result);
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpDEX() {
    x = x - 1;
    zero = x == 0;
    negative = !!(x & 0x80);
}

void CPU::OpDEY() {
    y = y - 1;
    zero = y == 0;
    negative = !!(y & 0x80);
}

void CPU::OpEOR() {
    acc = acc ^ read(targetAddr);
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpINC() {
    uint8_t result = read(targetAddr) + 1;
    write(targetAddr, result);
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpINX() {
    x = x + 1;
    zero = x == 0;
    negative = !!(x & 0x80);
}

void CPU::OpINY() {
    y = y + 1;
    zero = y == 0;
    negative = !!(y & 0x80);
}

void CPU::OpJMP() {
    pc = targetAddr;
}

void CPU::OpJSR() {
    push16(pc - 1);
    pc = targetAddr;
}

void CPU::OpLDA() {
    acc = read(targetAddr);
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpLDX() {
    x = read(targetAddr);
    zero = x == 0;
    negative = !!(x & 0x80);
}

void CPU::OpLDY() {
    y = read(targetAddr);
    zero = y == 0;
    negative = !!(y & 0x80);
}

void CPU::OpLSR() {
    uint8_t result = 0;
    if(useAcc) {
        carry = !!(acc & 0x01);
        result = acc >> 1;
        acc = result;
        useAcc = 0;
    }
    else {
        uint8_t target = read(targetAddr);
        carry = !!(target & 0x01);
        result = target >> 1;
        write(targetAddr, result);
    }
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpNOP() { }

void CPU::OpORA() {
    acc = acc | read(targetAddr);
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpPHA() {
    push8(acc);
}

void CPU::OpPHP() {
    push8(getStatus() | 0x10);
}

void CPU::OpPLA() {
    acc = pop8();
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpPLP() {
    setStatus(pop8() & 0xEF);
}

void CPU::OpROL() {
    uint8_t result = 0;
    if(useAcc) {
        result = (acc << 1) | (carry ? 1 : 0);
        carry = !!(acc & 0x80);
        acc = result;
        useAcc = 0;
    }
    else {
        uint8_t target = read(targetAddr);
        result = (target << 1) | (carry ? 1 : 0);
        carry = !!(target & 0x80);
        write(targetAddr, result);
    }
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpROR() {
    uint8_t result = 0;
    if(useAcc) {
        result = (acc >> 1) | (carry ? 0x80 : 0);
        carry = !!(acc & 0x01);
        acc = result;
        useAcc = 0;
    }
    else {
        uint8_t target = read(targetAddr);
        result = (target >> 1) | (carry ? 0x80 : 0);
        carry = !!(target & 0x01);
        write(targetAddr, result);
    }
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpRTI() {
    setStatus(pop8() & 0xEF);
    pc = pop16();
}

void CPU::OpRTS() {
    pc = pop16() + 1;
}

void CPU::OpSBC() {
    uint8_t memAdd = read(targetAddr) ^ 0xFF;
    uint16_t tempAcc = acc + memAdd + (carry ? 1 : 0);
    zero = (tempAcc & 0xFF) == 0;
    negative = !!(tempAcc & 0x80);
    carry = (tempAcc >> 8) != 0;
    overflow = (((acc ^ tempAcc) & (memAdd ^ tempAcc)) & 0x80) != 0;
    acc = (uint8_t)(tempAcc & 0xFF);
}

void CPU::OpSEC() {
    carry = 1;
}

void CPU::OpSED() {
    decmode = 1;
}

void CPU::OpSEI() {
    intdisable = 1;
}

void CPU::OpSTA() {
    write(targetAddr, acc);
}

void CPU::OpSTX() {
    write(targetAddr, x);
}

void CPU::OpSTY() {
    write(targetAddr, y);
}

void CPU::OpTAX() {
    x = acc;
    zero = x == 0;
    negative = !!(x & 0x80);
}

void CPU::OpTAY() {
    y = acc;
    zero = y == 0;
    negative = !!(y & 0x80);
}

void CPU::OpTSX() {
    x = sp;
    zero = sp == 0;
    negative = !!(sp & 0x80);
}

void CPU::OpTXA() {
    acc = x;
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpTXS() {
    sp = x;
}

void CPU::OpTYA() {
    acc = y;
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

/* Undocument Ops */

void CPU::OpJAM() { }
void CPU::OpSLO() { }
void CPU::OpDOP() { }
void CPU::OpANC() { }
void CPU::OpTOP() { }
void CPU::OpRLA() { }
void CPU::OpSRE() { }
void CPU::OpALR() { }
void CPU::OpRRA() { }
void CPU::OpARR() { }
void CPU::OpSAX() { }
void CPU::OpXAA() { }
void CPU::OpAHX() { }
void CPU::OpXAS() { }
void CPU::OpSHY() { }
void CPU::OpSHX() { }
void CPU::OpLAX() { }
void CPU::OpLAR() { }
void CPU::OpDCP() { }
void CPU::OpAXS() { }
void CPU::OpISC() { }

/* Address Modes */

void CPU::AmABS() {
    targetAddr = loadAddr(pc + 1);
    pc += 3;
}

void CPU::AmABX() {
    targetAddr = loadAddr(pc + 1) + x;
    pc += 3;
}

void CPU::AmABX_C() {
    uint8_t low = read(pc + 1) + x;
    uint8_t high = read(pc + 2);
    targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    if(low < x) { // page boundary crossed
        read(targetAddr); // dummy read
        high += 1;
        targetAddr = (uint16_t)low | ((uint16_t)high << 8);
        addCycles(1);
    }
    pc += 3;
}

void CPU::AmABY() {
    targetAddr = loadAddr(pc + 1) + y;
    pc += 3;
}

void CPU::AmABY_C() {
    uint8_t low = read(pc + 1) + y;
    uint8_t high = read(pc + 2);
    targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    if(low < y) { // page boundary crossed
        read(targetAddr); // dummy read
        high += 1;
        targetAddr = (uint16_t)low | ((uint16_t)high << 8);
        addCycles(1);
    }
    pc += 3;
}

void CPU::AmACC() {
    useAcc = 1;
    pc += 1;
}

void CPU::AmIMM() {
    targetAddr = pc + 1;
    pc += 2;
}

void CPU::AmIMP() {
    pc += 1;
}

void CPU::AmIND() {
    uint16_t addrOperand = loadAddr(pc + 1);
    if((addrOperand & 0xFF) == 0xFF) { // force low byte to wrap
        uint8_t low = read(addrOperand);
        uint8_t high = read(addrOperand - 0xFF);
        targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    }
    else {
        targetAddr = loadAddr(addrOperand);
    }
    pc += 1; // no effect since only used for OpJMP()
}

void CPU::AmINX() {
    uint8_t addrOperand = x + read(pc + 1);
    // below is a modified loadAddr() such that
    // addrOperand wraps to the zero page
    uint8_t low = read(addrOperand);
    addrOperand += 1;
    uint8_t high = read(addrOperand);
    targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    pc += 2;
}

void CPU::AmINY() {
    uint8_t addrOperand = read(pc + 1);
    uint8_t low = read(addrOperand) + y;
    addrOperand += 1;
    uint8_t high = read(addrOperand);
    targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    if(low < y) { // page boundary crossed
        high += 1;
        targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    }
    pc += 2;
}

void CPU::AmINY_C() {
    uint8_t addrOperand = read(pc + 1);
    uint8_t low = read(addrOperand) + y;
    addrOperand += 1;
    uint8_t high = read(addrOperand);
    targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    if(low < y) { // page boundary crossed
        read(targetAddr); // dummy read
        high += 1;
        targetAddr = (uint16_t)low | ((uint16_t)high << 8);
        addCycles(1);
    }
    pc += 2;
}

void CPU::AmZPG() {
    targetAddr = read(pc + 1);
    pc += 2;
}

void CPU::AmZPX() {
    uint8_t addr = read(pc + 1) + x;
    targetAddr = addr;
    pc += 2;
}

void CPU::AmZPY() {
    uint8_t addr = read(pc + 1) + y;
    targetAddr = addr;
    pc += 2;
}

void CPU::runInstr(uint8_t opCode) {
    switch(opCode) {
        case (0x00): AmIMP(); OpBRK(); addCycles(7); break;
        case (0x01): AmINX(); OpORA(); addCycles(6); break;
        case (0x02): AmIMP(); OpJAM(); addCycles(0); break;
        case (0x03): AmINX(); OpSLO(); addCycles(8); break;
        case (0x04): AmZPG(); OpDOP(); addCycles(3); break;
        case (0x05): AmZPG(); OpORA(); addCycles(3); break;
        case (0x06): AmZPG(); OpASL(); addCycles(5); break;
        case (0x07): AmZPG(); OpSLO(); addCycles(5); break;
        case (0x08): AmIMP(); OpPHP(); addCycles(3); break;
        case (0x09): AmIMM(); OpORA(); addCycles(2); break;
        case (0x0A): AmACC(); OpASL(); addCycles(2); break;
        case (0x0B): AmIMM(); OpANC(); addCycles(2); break;
        case (0x0C): AmABS(); OpTOP(); addCycles(4); break;
        case (0x0D): AmABS(); OpORA(); addCycles(4); break;
        case (0x0E): AmABS(); OpASL(); addCycles(6); break;
        case (0x0F): AmABS(); OpSLO(); addCycles(6); break;
        case (0x10): AmIMM(); OpBPL(); addCycles(2); break;
        case (0x11): AmINY_C(); OpORA(); addCycles(5); break;
        case (0x12): AmIMP(); OpJAM(); addCycles(0); break;
        case (0x13): AmINY(); OpSLO(); addCycles(8); break;
        case (0x14): AmZPX(); OpDOP(); addCycles(4); break;
        case (0x15): AmZPX(); OpORA(); addCycles(4); break;
        case (0x16): AmZPX(); OpASL(); addCycles(6); break;
        case (0x17): AmZPX(); OpSLO(); addCycles(6); break;
        case (0x18): AmIMP(); OpCLC(); addCycles(2); break;
        case (0x19): AmABY_C(); OpORA(); addCycles(4); break;
        case (0x1A): AmIMP(); OpNOP(); addCycles(2); break;
        case (0x1B): AmABY(); OpSLO(); addCycles(7); break;
        case (0x1C): AmABX_C(); OpTOP(); addCycles(4); break;
        case (0x1D): AmABX_C(); OpORA(); addCycles(4); break;
        case (0x1E): AmABX(); OpASL(); addCycles(7); break;
        case (0x1F): AmABX(); OpSLO(); addCycles(7); break;
        case (0x20): AmABS(); OpJSR(); addCycles(6); break;
        case (0x21): AmINX(); OpAND(); addCycles(6); break;
        case (0x22): AmIMP(); OpJAM(); addCycles(0); break;
        case (0x23): AmINX(); OpRLA(); addCycles(8); break;
        case (0x24): AmZPG(); OpBIT(); addCycles(3); break;
        case (0x25): AmZPG(); OpAND(); addCycles(3); break;
        case (0x26): AmZPG(); OpROL(); addCycles(5); break;
        case (0x27): AmZPG(); OpRLA(); addCycles(5); break;
        case (0x28): AmIMP(); OpPLP(); addCycles(4); break;
        case (0x29): AmIMM(); OpAND(); addCycles(2); break;
        case (0x2A): AmACC(); OpROL(); addCycles(2); break;
        case (0x2B): AmIMM(); OpANC(); addCycles(2); break;
        case (0x2C): AmABS(); OpBIT(); addCycles(4); break;
        case (0x2D): AmABS(); OpAND(); addCycles(4); break;
        case (0x2E): AmABS(); OpROL(); addCycles(6); break;
        case (0x2F): AmABS(); OpRLA(); addCycles(6); break;
        case (0x30): AmIMM(); OpBMI(); addCycles(2); break;
        case (0x31): AmINY_C(); OpAND(); addCycles(5); break;
        case (0x32): AmIMP(); OpJAM(); addCycles(0); break;
        case (0x33): AmINY(); OpRLA(); addCycles(8); break;
        case (0x34): AmZPX(); OpDOP(); addCycles(4); break;
        case (0x35): AmZPX(); OpAND(); addCycles(4); break;
        case (0x36): AmZPX(); OpROL(); addCycles(6); break;
        case (0x37): AmZPX(); OpRLA(); addCycles(6); break;
        case (0x38): AmIMP(); OpSEC(); addCycles(2); break;
        case (0x39): AmABY_C(); OpAND(); addCycles(4); break;
        case (0x3A): AmIMP(); OpNOP(); addCycles(2); break;
        case (0x3B): AmABY(); OpRLA(); addCycles(7); break;
        case (0x3C): AmABX_C(); OpTOP(); addCycles(4); break;
        case (0x3D): AmABX_C(); OpAND(); addCycles(4); break;
        case (0x3E): AmABX(); OpROL(); addCycles(7); break;
        case (0x3F): AmABX(); OpRLA(); addCycles(7); break;
        case (0x40): AmIMP(); OpRTI(); addCycles(6); break;
        case (0x41): AmINX(); OpEOR(); addCycles(6); break;
        case (0x42): AmIMP(); OpJAM(); addCycles(0); break;
        case (0x43): AmINX(); OpSRE(); addCycles(8); break;
        case (0x44): AmZPG(); OpDOP(); addCycles(3); break;
        case (0x45): AmZPG(); OpEOR(); addCycles(3); break;
        case (0x46): AmZPG(); OpLSR(); addCycles(5); break;
        case (0x47): AmZPG(); OpSRE(); addCycles(5); break;
        case (0x48): AmIMP(); OpPHA(); addCycles(3); break;
        case (0x49): AmIMM(); OpEOR(); addCycles(2); break;
        case (0x4A): AmACC(); OpLSR(); addCycles(2); break;
        case (0x4B): AmIMM(); OpALR(); addCycles(2); break;
        case (0x4C): AmABS(); OpJMP(); addCycles(3); break;
        case (0x4D): AmABS(); OpEOR(); addCycles(4); break;
        case (0x4E): AmABS(); OpLSR(); addCycles(6); break;
        case (0x4F): AmABS(); OpSRE(); addCycles(6); break;
        case (0x50): AmIMM(); OpBVC(); addCycles(2); break;
        case (0x51): AmINY_C(); OpEOR(); addCycles(5); break;
        case (0x52): AmIMP(); OpJAM(); addCycles(0); break;
        case (0x53): AmINY(); OpSRE(); addCycles(8); break;
        case (0x54): AmZPX(); OpDOP(); addCycles(4); break;
        case (0x55): AmZPX(); OpEOR(); addCycles(4); break;
        case (0x56): AmZPX(); OpLSR(); addCycles(6); break;
        case (0x57): AmZPX(); OpSRE(); addCycles(6); break;
        case (0x58): AmIMP(); OpCLI(); addCycles(2); break;
        case (0x59): AmABY_C(); OpEOR(); addCycles(4); break;
        case (0x5A): AmIMP(); OpNOP(); addCycles(2); break;
        case (0x5B): AmABY(); OpSRE(); addCycles(7); break;
        case (0x5C): AmABX_C(); OpTOP(); addCycles(4); break;
        case (0x5D): AmABX_C(); OpEOR(); addCycles(4); break;
        case (0x5E): AmABX(); OpLSR(); addCycles(7); break;
        case (0x5F): AmABX(); OpSRE(); addCycles(7); break;
        case (0x60): AmIMP(); OpRTS(); addCycles(6); break;
        case (0x61): AmINX(); OpADC(); addCycles(6); break;
        case (0x62): AmIMP(); OpJAM(); addCycles(0); break;
        case (0x63): AmINX(); OpRRA(); addCycles(8); break;
        case (0x64): AmZPG(); OpDOP(); addCycles(3); break;
        case (0x65): AmZPG(); OpADC(); addCycles(3); break;
        case (0x66): AmZPG(); OpROR(); addCycles(5); break;
        case (0x67): AmZPG(); OpRRA(); addCycles(5); break;
        case (0x68): AmIMP(); OpPLA(); addCycles(4); break;
        case (0x69): AmIMM(); OpADC(); addCycles(2); break;
        case (0x6A): AmACC(); OpROR(); addCycles(2); break;
        case (0x6B): AmIMM(); OpARR(); addCycles(2); break;
        case (0x6C): AmIND(); OpJMP(); addCycles(5); break;
        case (0x6D): AmABS(); OpADC(); addCycles(4); break;
        case (0x6E): AmABS(); OpROR(); addCycles(6); break;
        case (0x6F): AmABS(); OpRRA(); addCycles(6); break;
        case (0x70): AmIMM(); OpBVS(); addCycles(2); break;
        case (0x71): AmINY_C(); OpADC(); addCycles(5); break;
        case (0x72): AmIMP(); OpJAM(); addCycles(0); break;
        case (0x73): AmINY(); OpRRA(); addCycles(8); break;
        case (0x74): AmZPX(); OpDOP(); addCycles(4); break;
        case (0x75): AmZPX(); OpADC(); addCycles(4); break;
        case (0x76): AmZPX(); OpROR(); addCycles(6); break;
        case (0x77): AmZPX(); OpRRA(); addCycles(6); break;
        case (0x78): AmIMP(); OpSEI(); addCycles(2); break;
        case (0x79): AmABY_C(); OpADC(); addCycles(4); break;
        case (0x7A): AmIMP(); OpNOP(); addCycles(2); break;
        case (0x7B): AmABY(); OpRRA(); addCycles(7); break;
        case (0x7C): AmABX_C(); OpTOP(); addCycles(4); break;
        case (0x7D): AmABX_C(); OpADC(); addCycles(4); break;
        case (0x7E): AmABX(); OpROR(); addCycles(7); break;
        case (0x7F): AmABX(); OpRRA(); addCycles(7); break;
        case (0x80): AmIMM(); OpDOP(); addCycles(2); break;
        case (0x81): AmINX(); OpSTA(); addCycles(6); break;
        case (0x82): AmIMM(); OpDOP(); addCycles(2); break;
        case (0x83): AmINX(); OpSAX(); addCycles(6); break;
        case (0x84): AmZPG(); OpSTY(); addCycles(3); break;
        case (0x85): AmZPG(); OpSTA(); addCycles(3); break;
        case (0x86): AmZPG(); OpSTX(); addCycles(3); break;
        case (0x87): AmZPG(); OpSAX(); addCycles(3); break;
        case (0x88): AmIMP(); OpDEY(); addCycles(2); break;
        case (0x89): AmIMM(); OpDOP(); addCycles(2); break;
        case (0x8A): AmIMP(); OpTXA(); addCycles(2); break;
        case (0x8B): AmIMM(); OpXAA(); addCycles(2); break;
        case (0x8C): AmABS(); OpSTY(); addCycles(4); break;
        case (0x8D): AmABS(); OpSTA(); addCycles(4); break;
        case (0x8E): AmABS(); OpSTX(); addCycles(4); break;
        case (0x8F): AmABS(); OpSAX(); addCycles(4); break;
        case (0x90): AmIMM(); OpBCC(); addCycles(2); break;
        case (0x91): AmINY(); OpSTA(); addCycles(6); break;
        case (0x92): AmIMP(); OpJAM(); addCycles(0); break;
        case (0x93): AmINY(); OpAHX(); addCycles(6); break;
        case (0x94): AmZPX(); OpSTY(); addCycles(4); break;
        case (0x95): AmZPX(); OpSTA(); addCycles(4); break;
        case (0x96): AmZPY(); OpSTX(); addCycles(4); break;
        case (0x97): AmZPY(); OpSAX(); addCycles(4); break;
        case (0x98): AmIMP(); OpTYA(); addCycles(2); break;
        case (0x99): AmABY(); OpSTA(); addCycles(5); break;
        case (0x9A): AmIMP(); OpTXS(); addCycles(2); break;
        case (0x9B): AmABY(); OpXAS(); addCycles(5); break;
        case (0x9C): AmABX(); OpSHY(); addCycles(5); break;
        case (0x9D): AmABX(); OpSTA(); addCycles(5); break;
        case (0x9E): AmABY(); OpSHX(); addCycles(5); break;
        case (0x9F): AmABY(); OpAHX(); addCycles(5); break;
        case (0xA0): AmIMM(); OpLDY(); addCycles(2); break;
        case (0xA1): AmINX(); OpLDA(); addCycles(6); break;
        case (0xA2): AmIMM(); OpLDX(); addCycles(2); break;
        case (0xA3): AmINX(); OpLAX(); addCycles(6); break;
        case (0xA4): AmZPG(); OpLDY(); addCycles(3); break;
        case (0xA5): AmZPG(); OpLDA(); addCycles(3); break;
        case (0xA6): AmZPG(); OpLDX(); addCycles(3); break;
        case (0xA7): AmZPG(); OpLAX(); addCycles(3); break;
        case (0xA8): AmIMP(); OpTAY(); addCycles(2); break;
        case (0xA9): AmIMM(); OpLDA(); addCycles(2); break;
        case (0xAA): AmIMP(); OpTAX(); addCycles(2); break;
        case (0xAB): AmIMM(); OpLAX(); addCycles(2); break;
        case (0xAC): AmABS(); OpLDY(); addCycles(4); break;
        case (0xAD): AmABS(); OpLDA(); addCycles(4); break;
        case (0xAE): AmABS(); OpLDX(); addCycles(4); break;
        case (0xAF): AmABS(); OpLAX(); addCycles(4); break;
        case (0xB0): AmIMM(); OpBCS(); addCycles(2); break;
        case (0xB1): AmINY_C(); OpLDA(); addCycles(5); break;
        case (0xB2): AmIMP(); OpJAM(); addCycles(0); break;
        case (0xB3): AmINY(); OpLAX(); addCycles(5); break;
        case (0xB4): AmZPX(); OpLDY(); addCycles(4); break;
        case (0xB5): AmZPX(); OpLDA(); addCycles(4); break;
        case (0xB6): AmZPY(); OpLDX(); addCycles(4); break;
        case (0xB7): AmZPY(); OpLAX(); addCycles(4); break;
        case (0xB8): AmIMP(); OpCLV(); addCycles(2); break;
        case (0xB9): AmABY_C(); OpLDA(); addCycles(4); break;
        case (0xBA): AmIMP(); OpTSX(); addCycles(2); break;
        case (0xBB): AmABY(); OpLAR(); addCycles(4); break;
        case (0xBC): AmABX_C(); OpLDY(); addCycles(4); break;
        case (0xBD): AmABX_C(); OpLDA(); addCycles(4); break;
        case (0xBE): AmABY_C(); OpLDX(); addCycles(4); break;
        case (0xBF): AmABY(); OpLAX(); addCycles(4); break;
        case (0xC0): AmIMM(); OpCPY(); addCycles(2); break;
        case (0xC1): AmINX(); OpCMP(); addCycles(6); break;
        case (0xC2): AmIMM(); OpDOP(); addCycles(2); break;
        case (0xC3): AmINX(); OpDCP(); addCycles(8); break;
        case (0xC4): AmZPG(); OpCPY(); addCycles(3); break;
        case (0xC5): AmZPG(); OpCMP(); addCycles(3); break;
        case (0xC6): AmZPG(); OpDEC(); addCycles(5); break;
        case (0xC7): AmZPG(); OpDCP(); addCycles(5); break;
        case (0xC8): AmIMP(); OpINY(); addCycles(2); break;
        case (0xC9): AmIMM(); OpCMP(); addCycles(2); break;
        case (0xCA): AmIMP(); OpDEX(); addCycles(2); break;
        case (0xCB): AmIMM(); OpAXS(); addCycles(2); break;
        case (0xCC): AmABS(); OpCPY(); addCycles(4); break;
        case (0xCD): AmABS(); OpCMP(); addCycles(4); break;
        case (0xCE): AmABS(); OpDEC(); addCycles(6); break;
        case (0xCF): AmABS(); OpDCP(); addCycles(6); break;
        case (0xD0): AmIMM(); OpBNE(); addCycles(2); break;
        case (0xD1): AmINY_C(); OpCMP(); addCycles(5); break;
        case (0xD2): AmIMP(); OpJAM(); addCycles(0); break;
        case (0xD3): AmINY(); OpDCP(); addCycles(8); break;
        case (0xD4): AmZPX(); OpDOP(); addCycles(4); break;
        case (0xD5): AmZPX(); OpCMP(); addCycles(4); break;
        case (0xD6): AmZPX(); OpDEC(); addCycles(6); break;
        case (0xD7): AmZPX(); OpDCP(); addCycles(6); break;
        case (0xD8): AmIMP(); OpCLD(); addCycles(2); break;
        case (0xD9): AmABY_C(); OpCMP(); addCycles(4); break;
        case (0xDA): AmIMP(); OpNOP(); addCycles(2); break;
        case (0xDB): AmABY(); OpDCP(); addCycles(7); break;
        case (0xDC): AmABX_C(); OpTOP(); addCycles(4); break;
        case (0xDD): AmABX_C(); OpCMP(); addCycles(4); break;
        case (0xDE): AmABX(); OpDEC(); addCycles(7); break;
        case (0xDF): AmABX(); OpDCP(); addCycles(7); break;
        case (0xE0): AmIMM(); OpCPX(); addCycles(2); break;
        case (0xE1): AmINX(); OpSBC(); addCycles(6); break;
        case (0xE2): AmIMM(); OpDOP(); addCycles(2); break;
        case (0xE3): AmINX(); OpISC(); addCycles(8); break;
        case (0xE4): AmZPG(); OpCPX(); addCycles(3); break;
        case (0xE5): AmZPG(); OpSBC(); addCycles(3); break;
        case (0xE6): AmZPG(); OpINC(); addCycles(5); break;
        case (0xE7): AmZPG(); OpISC(); addCycles(5); break;
        case (0xE8): AmIMP(); OpINX(); addCycles(2); break;
        case (0xE9): AmIMM(); OpSBC(); addCycles(2); break;
        case (0xEA): AmIMP(); OpNOP(); addCycles(2); break;
        case (0xEB): AmIMM(); OpSBC(); addCycles(2); break;
        case (0xEC): AmABS(); OpCPX(); addCycles(4); break;
        case (0xED): AmABS(); OpSBC(); addCycles(4); break;
        case (0xEE): AmABS(); OpINC(); addCycles(6); break;
        case (0xEF): AmABS(); OpISC(); addCycles(6); break;
        case (0xF0): AmIMM(); OpBEQ(); addCycles(2); break;
        case (0xF1): AmINY_C(); OpSBC(); addCycles(5); break;
        case (0xF2): AmIMP(); OpJAM(); addCycles(0); break;
        case (0xF3): AmINY(); OpISC(); addCycles(8); break;
        case (0xF4): AmZPX(); OpDOP(); addCycles(4); break;
        case (0xF5): AmZPX(); OpSBC(); addCycles(4); break;
        case (0xF6): AmZPX(); OpINC(); addCycles(6); break;
        case (0xF7): AmZPX(); OpISC(); addCycles(6); break;
        case (0xF8): AmIMP(); OpSED(); addCycles(2); break;
        case (0xF9): AmABY_C(); OpSBC(); addCycles(4); break;
        case (0xFA): AmIMP(); OpNOP(); addCycles(2); break;
        case (0xFB): AmABY(); OpISC(); addCycles(7); break;
        case (0xFC): AmABX_C(); OpTOP(); addCycles(4); break;
        case (0xFD): AmABX_C(); OpSBC(); addCycles(4); break;
        case (0xFE): AmABX(); OpINC(); addCycles(7); break;
        case (0xFF): AmABX(); OpISC(); addCycles(7); break;
        default: printf("Invalid opcode\n");
    }
}

void CPU::addCycles(int c) {
    cyclesLeft += c;
}

void CPU::tick() {
    if (cyclesLeft == 0) {
	executeNextOp();
    }
    cyclesLeft -= 1;
}

void CPU::signalNMI() {
    nmiSignal = 1;
}

void CPU::signalIRQ() {
    irqSignal = 1;
}

void CPU::executeNextOp() {
    runInstr(read(pc));
    handleInterrupts();
}
