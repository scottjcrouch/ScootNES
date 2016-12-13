#include <stdint.h>
#include <stdlib.h>

#include <CPU.h>
#include <Console.h>

CPU::CPU(Console *console) {
    this->console = console;
}

void CPU::boot() {
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

    opCycles = 0;
}

uint8_t CPU::read(uint16_t addr) {
    return console->cpuRead(addr);
}

void CPU::write(uint16_t addr, uint8_t data) {
    console->cpuWrite(addr, data);
}

void CPU::push8(uint8_t data) {
    write(sp | 0x0100, data);
    sp -= 1;
}

void CPU::push16(uint16_t data) {
    push8((data >> 8) & 0xFF);
    push8(data & 0xFF);
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

void CPU::setStatus(uint8_t data) {
    carry       = !!(data & 0x01);
    zero        = !!(data & 0x02);
    intdisable  = !!(data & 0x04);
    decmode     = !!(data & 0x08);
    brk         = !!(data & 0x10);
    // bit 5 unused
    overflow    = !!(data & 0x40);
    negative    = !!(data & 0x80);
}

void CPU::handleInterrupts() {
    if(nmiSignal) {
        push16(pc);
        push8(getStatus() & ~(0x20));
        intdisable = 1;
        pc = loadAddr(NMI_VECTOR);
        tick(7);
        nmiSignal = 0;
    }
    else if(irqSignal && !intdisable) {
        push16(pc);
        push8(getStatus() & ~(0x20));
        intdisable = 1;
        pc = loadAddr(IRQ_VECTOR);
        tick(7);
        irqSignal = 0;
    }
}

void CPU::branch() {
    // the offset is stored as a signed byte
    int8_t offset = (int8_t)read(targetAddr);
    uint16_t branchAddr = pc + offset;
    if((branchAddr & 0xFF00) != (pc & 0xFF00)) {
        // page boundary crossed
        tick(2);
    }
    else {
        tick(1);
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
        tick(1);
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
        tick(1);
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
        tick(1);
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
        case (0x00): AmIMP(); OpBRK(); tick(7); break;
        case (0x01): AmINX(); OpORA(); tick(6); break;
        case (0x02): AmIMP(); OpJAM(); tick(0); break;
        case (0x03): AmINX(); OpSLO(); tick(8); break;
        case (0x04): AmZPG(); OpDOP(); tick(3); break;
        case (0x05): AmZPG(); OpORA(); tick(3); break;
        case (0x06): AmZPG(); OpASL(); tick(5); break;
        case (0x07): AmZPG(); OpSLO(); tick(5); break;
        case (0x08): AmIMP(); OpPHP(); tick(3); break;
        case (0x09): AmIMM(); OpORA(); tick(2); break;
        case (0x0A): AmACC(); OpASL(); tick(2); break;
        case (0x0B): AmIMM(); OpANC(); tick(2); break;
        case (0x0C): AmABS(); OpTOP(); tick(4); break;
        case (0x0D): AmABS(); OpORA(); tick(4); break;
        case (0x0E): AmABS(); OpASL(); tick(6); break;
        case (0x0F): AmABS(); OpSLO(); tick(6); break;
        case (0x10): AmIMM(); OpBPL(); tick(2); break;
        case (0x11): AmINY_C(); OpORA(); tick(5); break;
        case (0x12): AmIMP(); OpJAM(); tick(0); break;
        case (0x13): AmINY(); OpSLO(); tick(8); break;
        case (0x14): AmZPX(); OpDOP(); tick(4); break;
        case (0x15): AmZPX(); OpORA(); tick(4); break;
        case (0x16): AmZPX(); OpASL(); tick(6); break;
        case (0x17): AmZPX(); OpSLO(); tick(6); break;
        case (0x18): AmIMP(); OpCLC(); tick(2); break;
        case (0x19): AmABY_C(); OpORA(); tick(4); break;
        case (0x1A): AmIMP(); OpNOP(); tick(2); break;
        case (0x1B): AmABY(); OpSLO(); tick(7); break;
        case (0x1C): AmABX_C(); OpTOP(); tick(4); break;
        case (0x1D): AmABX_C(); OpORA(); tick(4); break;
        case (0x1E): AmABX(); OpASL(); tick(7); break;
        case (0x1F): AmABX(); OpSLO(); tick(7); break;
        case (0x20): AmABS(); OpJSR(); tick(6); break;
        case (0x21): AmINX(); OpAND(); tick(6); break;
        case (0x22): AmIMP(); OpJAM(); tick(0); break;
        case (0x23): AmINX(); OpRLA(); tick(8); break;
        case (0x24): AmZPG(); OpBIT(); tick(3); break;
        case (0x25): AmZPG(); OpAND(); tick(3); break;
        case (0x26): AmZPG(); OpROL(); tick(5); break;
        case (0x27): AmZPG(); OpRLA(); tick(5); break;
        case (0x28): AmIMP(); OpPLP(); tick(4); break;
        case (0x29): AmIMM(); OpAND(); tick(2); break;
        case (0x2A): AmACC(); OpROL(); tick(2); break;
        case (0x2B): AmIMM(); OpANC(); tick(2); break;
        case (0x2C): AmABS(); OpBIT(); tick(4); break;
        case (0x2D): AmABS(); OpAND(); tick(4); break;
        case (0x2E): AmABS(); OpROL(); tick(6); break;
        case (0x2F): AmABS(); OpRLA(); tick(6); break;
        case (0x30): AmIMM(); OpBMI(); tick(2); break;
        case (0x31): AmINY_C(); OpAND(); tick(5); break;
        case (0x32): AmIMP(); OpJAM(); tick(0); break;
        case (0x33): AmINY(); OpRLA(); tick(8); break;
        case (0x34): AmZPX(); OpDOP(); tick(4); break;
        case (0x35): AmZPX(); OpAND(); tick(4); break;
        case (0x36): AmZPX(); OpROL(); tick(6); break;
        case (0x37): AmZPX(); OpRLA(); tick(6); break;
        case (0x38): AmIMP(); OpSEC(); tick(2); break;
        case (0x39): AmABY_C(); OpAND(); tick(4); break;
        case (0x3A): AmIMP(); OpNOP(); tick(2); break;
        case (0x3B): AmABY(); OpRLA(); tick(7); break;
        case (0x3C): AmABX_C(); OpTOP(); tick(4); break;
        case (0x3D): AmABX_C(); OpAND(); tick(4); break;
        case (0x3E): AmABX(); OpROL(); tick(7); break;
        case (0x3F): AmABX(); OpRLA(); tick(7); break;
        case (0x40): AmIMP(); OpRTI(); tick(6); break;
        case (0x41): AmINX(); OpEOR(); tick(6); break;
        case (0x42): AmIMP(); OpJAM(); tick(0); break;
        case (0x43): AmINX(); OpSRE(); tick(8); break;
        case (0x44): AmZPG(); OpDOP(); tick(3); break;
        case (0x45): AmZPG(); OpEOR(); tick(3); break;
        case (0x46): AmZPG(); OpLSR(); tick(5); break;
        case (0x47): AmZPG(); OpSRE(); tick(5); break;
        case (0x48): AmIMP(); OpPHA(); tick(3); break;
        case (0x49): AmIMM(); OpEOR(); tick(2); break;
        case (0x4A): AmACC(); OpLSR(); tick(2); break;
        case (0x4B): AmIMM(); OpALR(); tick(2); break;
        case (0x4C): AmABS(); OpJMP(); tick(3); break;
        case (0x4D): AmABS(); OpEOR(); tick(4); break;
        case (0x4E): AmABS(); OpLSR(); tick(6); break;
        case (0x4F): AmABS(); OpSRE(); tick(6); break;
        case (0x50): AmIMM(); OpBVC(); tick(2); break;
        case (0x51): AmINY_C(); OpEOR(); tick(5); break;
        case (0x52): AmIMP(); OpJAM(); tick(0); break;
        case (0x53): AmINY(); OpSRE(); tick(8); break;
        case (0x54): AmZPX(); OpDOP(); tick(4); break;
        case (0x55): AmZPX(); OpEOR(); tick(4); break;
        case (0x56): AmZPX(); OpLSR(); tick(6); break;
        case (0x57): AmZPX(); OpSRE(); tick(6); break;
        case (0x58): AmIMP(); OpCLI(); tick(2); break;
        case (0x59): AmABY_C(); OpEOR(); tick(4); break;
        case (0x5A): AmIMP(); OpNOP(); tick(2); break;
        case (0x5B): AmABY(); OpSRE(); tick(7); break;
        case (0x5C): AmABX_C(); OpTOP(); tick(4); break;
        case (0x5D): AmABX_C(); OpEOR(); tick(4); break;
        case (0x5E): AmABX(); OpLSR(); tick(7); break;
        case (0x5F): AmABX(); OpSRE(); tick(7); break;
        case (0x60): AmIMP(); OpRTS(); tick(6); break;
        case (0x61): AmINX(); OpADC(); tick(6); break;
        case (0x62): AmIMP(); OpJAM(); tick(0); break;
        case (0x63): AmINX(); OpRRA(); tick(8); break;
        case (0x64): AmZPG(); OpDOP(); tick(3); break;
        case (0x65): AmZPG(); OpADC(); tick(3); break;
        case (0x66): AmZPG(); OpROR(); tick(5); break;
        case (0x67): AmZPG(); OpRRA(); tick(5); break;
        case (0x68): AmIMP(); OpPLA(); tick(4); break;
        case (0x69): AmIMM(); OpADC(); tick(2); break;
        case (0x6A): AmACC(); OpROR(); tick(2); break;
        case (0x6B): AmIMM(); OpARR(); tick(2); break;
        case (0x6C): AmIND(); OpJMP(); tick(5); break;
        case (0x6D): AmABS(); OpADC(); tick(4); break;
        case (0x6E): AmABS(); OpROR(); tick(6); break;
        case (0x6F): AmABS(); OpRRA(); tick(6); break;
        case (0x70): AmIMM(); OpBVS(); tick(2); break;
        case (0x71): AmINY_C(); OpADC(); tick(5); break;
        case (0x72): AmIMP(); OpJAM(); tick(0); break;
        case (0x73): AmINY(); OpRRA(); tick(8); break;
        case (0x74): AmZPX(); OpDOP(); tick(4); break;
        case (0x75): AmZPX(); OpADC(); tick(4); break;
        case (0x76): AmZPX(); OpROR(); tick(6); break;
        case (0x77): AmZPX(); OpRRA(); tick(6); break;
        case (0x78): AmIMP(); OpSEI(); tick(2); break;
        case (0x79): AmABY_C(); OpADC(); tick(4); break;
        case (0x7A): AmIMP(); OpNOP(); tick(2); break;
        case (0x7B): AmABY(); OpRRA(); tick(7); break;
        case (0x7C): AmABX_C(); OpTOP(); tick(4); break;
        case (0x7D): AmABX_C(); OpADC(); tick(4); break;
        case (0x7E): AmABX(); OpROR(); tick(7); break;
        case (0x7F): AmABX(); OpRRA(); tick(7); break;
        case (0x80): AmIMM(); OpDOP(); tick(2); break;
        case (0x81): AmINX(); OpSTA(); tick(6); break;
        case (0x82): AmIMM(); OpDOP(); tick(2); break;
        case (0x83): AmINX(); OpSAX(); tick(6); break;
        case (0x84): AmZPG(); OpSTY(); tick(3); break;
        case (0x85): AmZPG(); OpSTA(); tick(3); break;
        case (0x86): AmZPG(); OpSTX(); tick(3); break;
        case (0x87): AmZPG(); OpSAX(); tick(3); break;
        case (0x88): AmIMP(); OpDEY(); tick(2); break;
        case (0x89): AmIMM(); OpDOP(); tick(2); break;
        case (0x8A): AmIMP(); OpTXA(); tick(2); break;
        case (0x8B): AmIMM(); OpXAA(); tick(2); break;
        case (0x8C): AmABS(); OpSTY(); tick(4); break;
        case (0x8D): AmABS(); OpSTA(); tick(4); break;
        case (0x8E): AmABS(); OpSTX(); tick(4); break;
        case (0x8F): AmABS(); OpSAX(); tick(4); break;
        case (0x90): AmIMM(); OpBCC(); tick(2); break;
        case (0x91): AmINY(); OpSTA(); tick(6); break;
        case (0x92): AmIMP(); OpJAM(); tick(0); break;
        case (0x93): AmINY(); OpAHX(); tick(6); break;
        case (0x94): AmZPX(); OpSTY(); tick(4); break;
        case (0x95): AmZPX(); OpSTA(); tick(4); break;
        case (0x96): AmZPY(); OpSTX(); tick(4); break;
        case (0x97): AmZPY(); OpSAX(); tick(4); break;
        case (0x98): AmIMP(); OpTYA(); tick(2); break;
        case (0x99): AmABY(); OpSTA(); tick(5); break;
        case (0x9A): AmIMP(); OpTXS(); tick(2); break;
        case (0x9B): AmABY(); OpXAS(); tick(5); break;
        case (0x9C): AmABX(); OpSHY(); tick(5); break;
        case (0x9D): AmABX(); OpSTA(); tick(5); break;
        case (0x9E): AmABY(); OpSHX(); tick(5); break;
        case (0x9F): AmABY(); OpAHX(); tick(5); break;
        case (0xA0): AmIMM(); OpLDY(); tick(2); break;
        case (0xA1): AmINX(); OpLDA(); tick(6); break;
        case (0xA2): AmIMM(); OpLDX(); tick(2); break;
        case (0xA3): AmINX(); OpLAX(); tick(6); break;
        case (0xA4): AmZPG(); OpLDY(); tick(3); break;
        case (0xA5): AmZPG(); OpLDA(); tick(3); break;
        case (0xA6): AmZPG(); OpLDX(); tick(3); break;
        case (0xA7): AmZPG(); OpLAX(); tick(3); break;
        case (0xA8): AmIMP(); OpTAY(); tick(2); break;
        case (0xA9): AmIMM(); OpLDA(); tick(2); break;
        case (0xAA): AmIMP(); OpTAX(); tick(2); break;
        case (0xAB): AmIMM(); OpLAX(); tick(2); break;
        case (0xAC): AmABS(); OpLDY(); tick(4); break;
        case (0xAD): AmABS(); OpLDA(); tick(4); break;
        case (0xAE): AmABS(); OpLDX(); tick(4); break;
        case (0xAF): AmABS(); OpLAX(); tick(4); break;
        case (0xB0): AmIMM(); OpBCS(); tick(2); break;
        case (0xB1): AmINY_C(); OpLDA(); tick(5); break;
        case (0xB2): AmIMP(); OpJAM(); tick(0); break;
        case (0xB3): AmINY(); OpLAX(); tick(5); break;
        case (0xB4): AmZPX(); OpLDY(); tick(4); break;
        case (0xB5): AmZPX(); OpLDA(); tick(4); break;
        case (0xB6): AmZPY(); OpLDX(); tick(4); break;
        case (0xB7): AmZPY(); OpLAX(); tick(4); break;
        case (0xB8): AmIMP(); OpCLV(); tick(2); break;
        case (0xB9): AmABY_C(); OpLDA(); tick(4); break;
        case (0xBA): AmIMP(); OpTSX(); tick(2); break;
        case (0xBB): AmABY(); OpLAR(); tick(4); break;
        case (0xBC): AmABX_C(); OpLDY(); tick(4); break;
        case (0xBD): AmABX_C(); OpLDA(); tick(4); break;
        case (0xBE): AmABY_C(); OpLDX(); tick(4); break;
        case (0xBF): AmABY(); OpLAX(); tick(4); break;
        case (0xC0): AmIMM(); OpCPY(); tick(2); break;
        case (0xC1): AmINX(); OpCMP(); tick(6); break;
        case (0xC2): AmIMM(); OpDOP(); tick(2); break;
        case (0xC3): AmINX(); OpDCP(); tick(8); break;
        case (0xC4): AmZPG(); OpCPY(); tick(3); break;
        case (0xC5): AmZPG(); OpCMP(); tick(3); break;
        case (0xC6): AmZPG(); OpDEC(); tick(5); break;
        case (0xC7): AmZPG(); OpDCP(); tick(5); break;
        case (0xC8): AmIMP(); OpINY(); tick(2); break;
        case (0xC9): AmIMM(); OpCMP(); tick(2); break;
        case (0xCA): AmIMP(); OpDEX(); tick(2); break;
        case (0xCB): AmIMM(); OpAXS(); tick(2); break;
        case (0xCC): AmABS(); OpCPY(); tick(4); break;
        case (0xCD): AmABS(); OpCMP(); tick(4); break;
        case (0xCE): AmABS(); OpDEC(); tick(6); break;
        case (0xCF): AmABS(); OpDCP(); tick(6); break;
        case (0xD0): AmIMM(); OpBNE(); tick(2); break;
        case (0xD1): AmINY_C(); OpCMP(); tick(5); break;
        case (0xD2): AmIMP(); OpJAM(); tick(0); break;
        case (0xD3): AmINY(); OpDCP(); tick(8); break;
        case (0xD4): AmZPX(); OpDOP(); tick(4); break;
        case (0xD5): AmZPX(); OpCMP(); tick(4); break;
        case (0xD6): AmZPX(); OpDEC(); tick(6); break;
        case (0xD7): AmZPX(); OpDCP(); tick(6); break;
        case (0xD8): AmIMP(); OpCLD(); tick(2); break;
        case (0xD9): AmABY_C(); OpCMP(); tick(4); break;
        case (0xDA): AmIMP(); OpNOP(); tick(2); break;
        case (0xDB): AmABY(); OpDCP(); tick(7); break;
        case (0xDC): AmABX_C(); OpTOP(); tick(4); break;
        case (0xDD): AmABX_C(); OpCMP(); tick(4); break;
        case (0xDE): AmABX(); OpDEC(); tick(7); break;
        case (0xDF): AmABX(); OpDCP(); tick(7); break;
        case (0xE0): AmIMM(); OpCPX(); tick(2); break;
        case (0xE1): AmINX(); OpSBC(); tick(6); break;
        case (0xE2): AmIMM(); OpDOP(); tick(2); break;
        case (0xE3): AmINX(); OpISC(); tick(8); break;
        case (0xE4): AmZPG(); OpCPX(); tick(3); break;
        case (0xE5): AmZPG(); OpSBC(); tick(3); break;
        case (0xE6): AmZPG(); OpINC(); tick(5); break;
        case (0xE7): AmZPG(); OpISC(); tick(5); break;
        case (0xE8): AmIMP(); OpINX(); tick(2); break;
        case (0xE9): AmIMM(); OpSBC(); tick(2); break;
        case (0xEA): AmIMP(); OpNOP(); tick(2); break;
        case (0xEB): AmIMM(); OpSBC(); tick(2); break;
        case (0xEC): AmABS(); OpCPX(); tick(4); break;
        case (0xED): AmABS(); OpSBC(); tick(4); break;
        case (0xEE): AmABS(); OpINC(); tick(6); break;
        case (0xEF): AmABS(); OpISC(); tick(6); break;
        case (0xF0): AmIMM(); OpBEQ(); tick(2); break;
        case (0xF1): AmINY_C(); OpSBC(); tick(5); break;
        case (0xF2): AmIMP(); OpJAM(); tick(0); break;
        case (0xF3): AmINY(); OpISC(); tick(8); break;
        case (0xF4): AmZPX(); OpDOP(); tick(4); break;
        case (0xF5): AmZPX(); OpSBC(); tick(4); break;
        case (0xF6): AmZPX(); OpINC(); tick(6); break;
        case (0xF7): AmZPX(); OpISC(); tick(6); break;
        case (0xF8): AmIMP(); OpSED(); tick(2); break;
        case (0xF9): AmABY_C(); OpSBC(); tick(4); break;
        case (0xFA): AmIMP(); OpNOP(); tick(2); break;
        case (0xFB): AmABY(); OpISC(); tick(7); break;
        case (0xFC): AmABX_C(); OpTOP(); tick(4); break;
        case (0xFD): AmABX_C(); OpSBC(); tick(4); break;
        case (0xFE): AmABX(); OpINC(); tick(7); break;
        case (0xFF): AmABX(); OpISC(); tick(7); break;
        default: printf("Invalid opcode\n");
    }
}

void CPU::tick(int c) {
    opCycles += c;
}

void CPU::signalNMI() {
    nmiSignal = 1;
}

void CPU::signalIRQ() {
    irqSignal = 1;
}

int CPU::executeNextOp() {
    opCycles = 0;
    runInstr(read(pc));
    handleInterrupts();
    return opCycles;
}
