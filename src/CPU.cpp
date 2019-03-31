#include <cstdint>

#include <CPU.h>

void CPU::reset()
{
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
}

void CPU::push8(uint8_t value)
{
    write(sp | 0x0100, value);
    sp -= 1;
}

void CPU::push16(uint16_t value)
{
    push8((value >> 8) & 0xFF);
    push8(value & 0xFF);
}

uint8_t CPU::pop8()
{
    sp += 1;
    return read(sp | 0x0100);
}

uint16_t CPU::pop16()
{
    uint8_t low = pop8();
    uint8_t high = pop8();
    return low | (high << 8);
}

uint16_t CPU::loadAddr(uint16_t addr)
{
    uint8_t low = read(addr);
    uint8_t high = read(addr+1);
    return low | (high << 8);
}

uint8_t CPU::getStatus()
{
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

void CPU::setStatus(uint8_t value)
{
    carry       = !!(value & 0x01);
    zero        = !!(value & 0x02);
    intdisable  = !!(value & 0x04);
    decmode     = !!(value & 0x08);
    brk         = !!(value & 0x10);
    // bit 5 unused
    overflow    = !!(value & 0x40);
    negative    = !!(value & 0x80);
}

void CPU::handleInterrupts()
{
    if(nmiSignal) {
        push16(pc);
        push8(getStatus() & ~(0x20));
        intdisable = 1;
        pc = loadAddr(NMI_VECTOR);
        suspend(7);
        nmiSignal = 0;
    } else if(irqSignal && !intdisable) {
        push16(pc);
        push8(getStatus() & ~(0x20));
        intdisable = 1;
        pc = loadAddr(IRQ_VECTOR);
        suspend(7);
        irqSignal = 0;
    }
}

void CPU::branch()
{
    // the offset is stored as a signed byte
    int8_t offset = (int8_t)read(targetAddr);
    uint16_t branchAddr = pc + offset;
    if((branchAddr & 0xFF00) != (pc & 0xFF00)) {
        // page boundary crossed
        suspend(2);
    } else {
        suspend(1);
    }
    pc = branchAddr;
}

void CPU::suspend(int cycles)
{
    cyclesLeft += cycles;
}

void CPU::tick()
{
    if (cyclesLeft == 0) {
	executeNextOp();
    }
    cyclesLeft -= 1;
}

void CPU::signalNMI()
{
    nmiSignal = 1;
}

void CPU::signalIRQ()
{
    irqSignal = 1;
}

void CPU::executeNextOp()
{
    runInstr(read(pc));
    handleInterrupts();
}

/* Instructions */

void CPU::OpADC()
{
    int memAdd = read(targetAddr);
    int tempAcc = acc + memAdd + (carry ? 1 : 0);
    zero = (tempAcc & 0xFF) == 0;
    negative = !!(tempAcc & 0x80);
    carry = (tempAcc >> 8) != 0;
    overflow = (((acc ^ tempAcc) & (memAdd ^ tempAcc)) & 0x80) != 0;
    acc = tempAcc & 0xFF;
}

void CPU::OpAND()
{
    acc &= read(targetAddr);
    zero = (acc == 0);
    negative = !!(acc & 0x80);
}

void CPU::OpASL()
{
    uint8_t result = 0;
    if(useAcc) {
        carry = !!(acc & 0x80);
        result = acc << 1;
        acc = result;
        useAcc = 0;
    } else {
        uint8_t target = read(targetAddr);
        carry = !!(target & 0x80);
        result = target << 1;
        write(targetAddr, result);
    }
    zero = (result == 0);
    negative = !!(result & 0x80);
}

void CPU::OpBCC()
{
    if(!carry) {
        branch();
    }
}

void CPU::OpBCS()
{
    if(carry) {
        branch();
    }
}

void CPU::OpBEQ()
{
    if(zero) {
        branch();
    }
}

void CPU::OpBIT()
{
    uint8_t fetched = read(targetAddr);
    uint8_t result = acc & fetched;
    zero = (result == 0);
    overflow = !!(fetched & 0x40);
    negative = !!(fetched & 0x80);
}

void CPU::OpBMI()
{
    if(negative) {
        branch();
    }
}

void CPU::OpBNE()
{
    if(!zero) {
        branch();
    }
}

void CPU::OpBPL()
{
    if(!negative) {
        branch();
    }
}

void CPU::OpBRK()
{
    push16(pc+1);
    push8(getStatus() | 0x10);
    intdisable = 1;
    pc = loadAddr(IRQ_VECTOR);
}

void CPU::OpBVC()
{
    if(!overflow) {
        branch();
    }
}

void CPU::OpBVS()
{
    if(overflow) {
        branch();
    }
}

void CPU::OpCLC()
{
    carry = 0;
}

void CPU::OpCLD()
{
    decmode = 0;
}

void CPU::OpCLI()
{
    intdisable = 0;
}

void CPU::OpCLV()
{
    overflow = 0;
}

void CPU::OpCMP()
{
    uint8_t fetched = read(targetAddr);
    uint8_t result = acc - fetched;
    carry = acc >= fetched;
    zero = acc == fetched;
    negative = !!(result & 0x80);
}

void CPU::OpCPX()
{
    uint8_t fetched = read(targetAddr);
    uint8_t result = x - fetched;
    carry = x >= fetched;
    zero = x == fetched;
    negative = !!(result & 0x80);
}

void CPU::OpCPY()
{
    uint8_t fetched = read(targetAddr);
    uint8_t result = y - fetched;
    carry = y >= fetched;
    zero = y == fetched;
    negative = !!(result & 0x80);
}

void CPU::OpDEC()
{
    uint8_t result = read(targetAddr) - 1;
    write(targetAddr, result);
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpDEX()
{
    x = x - 1;
    zero = x == 0;
    negative = !!(x & 0x80);
}

void CPU::OpDEY()
{
    y = y - 1;
    zero = y == 0;
    negative = !!(y & 0x80);
}

void CPU::OpEOR()
{
    acc = acc ^ read(targetAddr);
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpINC()
{
    uint8_t result = read(targetAddr) + 1;
    write(targetAddr, result);
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpINX()
{
    x = x + 1;
    zero = x == 0;
    negative = !!(x & 0x80);
}

void CPU::OpINY()
{
    y = y + 1;
    zero = y == 0;
    negative = !!(y & 0x80);
}

void CPU::OpJMP()
{
    pc = targetAddr;
}

void CPU::OpJSR()
{
    push16(pc - 1);
    pc = targetAddr;
}

void CPU::OpLDA()
{
    acc = read(targetAddr);
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpLDX()
{
    x = read(targetAddr);
    zero = x == 0;
    negative = !!(x & 0x80);
}

void CPU::OpLDY()
{
    y = read(targetAddr);
    zero = y == 0;
    negative = !!(y & 0x80);
}

void CPU::OpLSR()
{
    uint8_t result = 0;
    if(useAcc) {
        carry = !!(acc & 0x01);
        result = acc >> 1;
        acc = result;
        useAcc = 0;
    } else {
        uint8_t target = read(targetAddr);
        carry = !!(target & 0x01);
        result = target >> 1;
        write(targetAddr, result);
    }
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpNOP() { }

void CPU::OpORA()
{
    acc = acc | read(targetAddr);
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpPHA()
{
    push8(acc);
}

void CPU::OpPHP()
{
    push8(getStatus() | 0x10);
}

void CPU::OpPLA()
{
    acc = pop8();
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpPLP()
{
    setStatus(pop8() & 0xEF);
}

void CPU::OpROL()
{
    uint8_t result = 0;
    if(useAcc) {
        result = (acc << 1) | (carry ? 1 : 0);
        carry = !!(acc & 0x80);
        acc = result;
        useAcc = 0;
    } else {
        uint8_t target = read(targetAddr);
        result = (target << 1) | (carry ? 1 : 0);
        carry = !!(target & 0x80);
        write(targetAddr, result);
    }
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpROR()
{
    uint8_t result = 0;
    if(useAcc) {
        result = (acc >> 1) | (carry ? 0x80 : 0);
        carry = !!(acc & 0x01);
        acc = result;
        useAcc = 0;
    } else {
        uint8_t target = read(targetAddr);
        result = (target >> 1) | (carry ? 0x80 : 0);
        carry = !!(target & 0x01);
        write(targetAddr, result);
    }
    zero = result == 0;
    negative = !!(result & 0x80);
}

void CPU::OpRTI()
{
    setStatus(pop8() & 0xEF);
    pc = pop16();
}

void CPU::OpRTS()
{
    pc = pop16() + 1;
}

void CPU::OpSBC()
{
    uint8_t memAdd = read(targetAddr) ^ 0xFF;
    uint16_t tempAcc = acc + memAdd + (carry ? 1 : 0);
    zero = (tempAcc & 0xFF) == 0;
    negative = !!(tempAcc & 0x80);
    carry = (tempAcc >> 8) != 0;
    overflow = (((acc ^ tempAcc) & (memAdd ^ tempAcc)) & 0x80) != 0;
    acc = (uint8_t)(tempAcc & 0xFF);
}

void CPU::OpSEC()
{
    carry = 1;
}

void CPU::OpSED()
{
    decmode = 1;
}

void CPU::OpSEI()
{
    intdisable = 1;
}

void CPU::OpSTA()
{
    write(targetAddr, acc);
}

void CPU::OpSTX()
{
    write(targetAddr, x);
}

void CPU::OpSTY()
{
    write(targetAddr, y);
}

void CPU::OpTAX()
{
    x = acc;
    zero = x == 0;
    negative = !!(x & 0x80);
}

void CPU::OpTAY()
{
    y = acc;
    zero = y == 0;
    negative = !!(y & 0x80);
}

void CPU::OpTSX()
{
    x = sp;
    zero = sp == 0;
    negative = !!(sp & 0x80);
}

void CPU::OpTXA()
{
    acc = x;
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

void CPU::OpTXS()
{
    sp = x;
}

void CPU::OpTYA()
{
    acc = y;
    zero = acc == 0;
    negative = !!(acc & 0x80);
}

/* Undocumented Ops */

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

void CPU::AmABS()
{
    targetAddr = loadAddr(pc + 1);
    pc += 3;
}

void CPU::AmABX()
{
    targetAddr = loadAddr(pc + 1) + x;
    pc += 3;
}

void CPU::AmABX_C()
{
    uint8_t low = read(pc + 1) + x;
    uint8_t high = read(pc + 2);
    targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    if(low < x) { // page boundary crossed
        read(targetAddr); // dummy read
        high += 1;
        targetAddr = (uint16_t)low | ((uint16_t)high << 8);
        suspend(1);
    }
    pc += 3;
}

void CPU::AmABY()
{
    targetAddr = loadAddr(pc + 1) + y;
    pc += 3;
}

void CPU::AmABY_C()
{
    uint8_t low = read(pc + 1) + y;
    uint8_t high = read(pc + 2);
    targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    if(low < y) { // page boundary crossed
        read(targetAddr); // dummy read
        high += 1;
        targetAddr = (uint16_t)low | ((uint16_t)high << 8);
        suspend(1);
    }
    pc += 3;
}

void CPU::AmACC()
{
    useAcc = 1;
    pc += 1;
}

void CPU::AmIMM()
{
    targetAddr = pc + 1;
    pc += 2;
}

void CPU::AmIMP()
{
    pc += 1;
}

void CPU::AmIND()
{
    uint16_t addrOperand = loadAddr(pc + 1);
    if((addrOperand & 0xFF) == 0xFF) { // force low byte to wrap
        uint8_t low = read(addrOperand);
        uint8_t high = read(addrOperand - 0xFF);
        targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    } else {
        targetAddr = loadAddr(addrOperand);
    }
    pc += 1; // no effect since only used for OpJMP()
}

void CPU::AmINX()
{
    uint8_t addrOperand = x + read(pc + 1);
    // below is a modified loadAddr() such that
    // addrOperand wraps to the zero page
    uint8_t low = read(addrOperand);
    addrOperand += 1;
    uint8_t high = read(addrOperand);
    targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    pc += 2;
}

void CPU::AmINY()
{
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

void CPU::AmINY_C()
{
    uint8_t addrOperand = read(pc + 1);
    uint8_t low = read(addrOperand) + y;
    addrOperand += 1;
    uint8_t high = read(addrOperand);
    targetAddr = (uint16_t)low | ((uint16_t)high << 8);
    if(low < y) { // page boundary crossed
        read(targetAddr); // dummy read
        high += 1;
        targetAddr = (uint16_t)low | ((uint16_t)high << 8);
        suspend(1);
    }
    pc += 2;
}

void CPU::AmZPG()
{
    targetAddr = read(pc + 1);
    pc += 2;
}

void CPU::AmZPX()
{
    uint8_t addr = read(pc + 1) + x;
    targetAddr = addr;
    pc += 2;
}

void CPU::AmZPY()
{
    uint8_t addr = read(pc + 1) + y;
    targetAddr = addr;
    pc += 2;
}

void CPU::runInstr(uint8_t opCode)
{
    switch(opCode) {
        case (0x00): AmIMP();   OpBRK(); suspend(7); break;
        case (0x01): AmINX();   OpORA(); suspend(6); break;
        case (0x02): AmIMP();   OpJAM(); suspend(0); break;
        case (0x03): AmINX();   OpSLO(); suspend(8); break;
        case (0x04): AmZPG();   OpDOP(); suspend(3); break;
        case (0x05): AmZPG();   OpORA(); suspend(3); break;
        case (0x06): AmZPG();   OpASL(); suspend(5); break;
        case (0x07): AmZPG();   OpSLO(); suspend(5); break;
        case (0x08): AmIMP();   OpPHP(); suspend(3); break;
        case (0x09): AmIMM();   OpORA(); suspend(2); break;
        case (0x0A): AmACC();   OpASL(); suspend(2); break;
        case (0x0B): AmIMM();   OpANC(); suspend(2); break;
        case (0x0C): AmABS();   OpTOP(); suspend(4); break;
        case (0x0D): AmABS();   OpORA(); suspend(4); break;
        case (0x0E): AmABS();   OpASL(); suspend(6); break;
        case (0x0F): AmABS();   OpSLO(); suspend(6); break;
        case (0x10): AmIMM();   OpBPL(); suspend(2); break;
        case (0x11): AmINY_C(); OpORA(); suspend(5); break;
        case (0x12): AmIMP();   OpJAM(); suspend(0); break;
        case (0x13): AmINY();   OpSLO(); suspend(8); break;
        case (0x14): AmZPX();   OpDOP(); suspend(4); break;
        case (0x15): AmZPX();   OpORA(); suspend(4); break;
        case (0x16): AmZPX();   OpASL(); suspend(6); break;
        case (0x17): AmZPX();   OpSLO(); suspend(6); break;
        case (0x18): AmIMP();   OpCLC(); suspend(2); break;
        case (0x19): AmABY_C(); OpORA(); suspend(4); break;
        case (0x1A): AmIMP();   OpNOP(); suspend(2); break;
        case (0x1B): AmABY();   OpSLO(); suspend(7); break;
        case (0x1C): AmABX_C(); OpTOP(); suspend(4); break;
        case (0x1D): AmABX_C(); OpORA(); suspend(4); break;
        case (0x1E): AmABX();   OpASL(); suspend(7); break;
        case (0x1F): AmABX();   OpSLO(); suspend(7); break;
        case (0x20): AmABS();   OpJSR(); suspend(6); break;
        case (0x21): AmINX();   OpAND(); suspend(6); break;
        case (0x22): AmIMP();   OpJAM(); suspend(0); break;
        case (0x23): AmINX();   OpRLA(); suspend(8); break;
        case (0x24): AmZPG();   OpBIT(); suspend(3); break;
        case (0x25): AmZPG();   OpAND(); suspend(3); break;
        case (0x26): AmZPG();   OpROL(); suspend(5); break;
        case (0x27): AmZPG();   OpRLA(); suspend(5); break;
        case (0x28): AmIMP();   OpPLP(); suspend(4); break;
        case (0x29): AmIMM();   OpAND(); suspend(2); break;
        case (0x2A): AmACC();   OpROL(); suspend(2); break;
        case (0x2B): AmIMM();   OpANC(); suspend(2); break;
        case (0x2C): AmABS();   OpBIT(); suspend(4); break;
        case (0x2D): AmABS();   OpAND(); suspend(4); break;
        case (0x2E): AmABS();   OpROL(); suspend(6); break;
        case (0x2F): AmABS();   OpRLA(); suspend(6); break;
        case (0x30): AmIMM();   OpBMI(); suspend(2); break;
        case (0x31): AmINY_C(); OpAND(); suspend(5); break;
        case (0x32): AmIMP();   OpJAM(); suspend(0); break;
        case (0x33): AmINY();   OpRLA(); suspend(8); break;
        case (0x34): AmZPX();   OpDOP(); suspend(4); break;
        case (0x35): AmZPX();   OpAND(); suspend(4); break;
        case (0x36): AmZPX();   OpROL(); suspend(6); break;
        case (0x37): AmZPX();   OpRLA(); suspend(6); break;
        case (0x38): AmIMP();   OpSEC(); suspend(2); break;
        case (0x39): AmABY_C(); OpAND(); suspend(4); break;
        case (0x3A): AmIMP();   OpNOP(); suspend(2); break;
        case (0x3B): AmABY();   OpRLA(); suspend(7); break;
        case (0x3C): AmABX_C(); OpTOP(); suspend(4); break;
        case (0x3D): AmABX_C(); OpAND(); suspend(4); break;
        case (0x3E): AmABX();   OpROL(); suspend(7); break;
        case (0x3F): AmABX();   OpRLA(); suspend(7); break;
        case (0x40): AmIMP();   OpRTI(); suspend(6); break;
        case (0x41): AmINX();   OpEOR(); suspend(6); break;
        case (0x42): AmIMP();   OpJAM(); suspend(0); break;
        case (0x43): AmINX();   OpSRE(); suspend(8); break;
        case (0x44): AmZPG();   OpDOP(); suspend(3); break;
        case (0x45): AmZPG();   OpEOR(); suspend(3); break;
        case (0x46): AmZPG();   OpLSR(); suspend(5); break;
        case (0x47): AmZPG();   OpSRE(); suspend(5); break;
        case (0x48): AmIMP();   OpPHA(); suspend(3); break;
        case (0x49): AmIMM();   OpEOR(); suspend(2); break;
        case (0x4A): AmACC();   OpLSR(); suspend(2); break;
        case (0x4B): AmIMM();   OpALR(); suspend(2); break;
        case (0x4C): AmABS();   OpJMP(); suspend(3); break;
        case (0x4D): AmABS();   OpEOR(); suspend(4); break;
        case (0x4E): AmABS();   OpLSR(); suspend(6); break;
        case (0x4F): AmABS();   OpSRE(); suspend(6); break;
        case (0x50): AmIMM();   OpBVC(); suspend(2); break;
        case (0x51): AmINY_C(); OpEOR(); suspend(5); break;
        case (0x52): AmIMP();   OpJAM(); suspend(0); break;
        case (0x53): AmINY();   OpSRE(); suspend(8); break;
        case (0x54): AmZPX();   OpDOP(); suspend(4); break;
        case (0x55): AmZPX();   OpEOR(); suspend(4); break;
        case (0x56): AmZPX();   OpLSR(); suspend(6); break;
        case (0x57): AmZPX();   OpSRE(); suspend(6); break;
        case (0x58): AmIMP();   OpCLI(); suspend(2); break;
        case (0x59): AmABY_C(); OpEOR(); suspend(4); break;
        case (0x5A): AmIMP();   OpNOP(); suspend(2); break;
        case (0x5B): AmABY();   OpSRE(); suspend(7); break;
        case (0x5C): AmABX_C(); OpTOP(); suspend(4); break;
        case (0x5D): AmABX_C(); OpEOR(); suspend(4); break;
        case (0x5E): AmABX();   OpLSR(); suspend(7); break;
        case (0x5F): AmABX();   OpSRE(); suspend(7); break;
        case (0x60): AmIMP();   OpRTS(); suspend(6); break;
        case (0x61): AmINX();   OpADC(); suspend(6); break;
        case (0x62): AmIMP();   OpJAM(); suspend(0); break;
        case (0x63): AmINX();   OpRRA(); suspend(8); break;
        case (0x64): AmZPG();   OpDOP(); suspend(3); break;
        case (0x65): AmZPG();   OpADC(); suspend(3); break;
        case (0x66): AmZPG();   OpROR(); suspend(5); break;
        case (0x67): AmZPG();   OpRRA(); suspend(5); break;
        case (0x68): AmIMP();   OpPLA(); suspend(4); break;
        case (0x69): AmIMM();   OpADC(); suspend(2); break;
        case (0x6A): AmACC();   OpROR(); suspend(2); break;
        case (0x6B): AmIMM();   OpARR(); suspend(2); break;
        case (0x6C): AmIND();   OpJMP(); suspend(5); break;
        case (0x6D): AmABS();   OpADC(); suspend(4); break;
        case (0x6E): AmABS();   OpROR(); suspend(6); break;
        case (0x6F): AmABS();   OpRRA(); suspend(6); break;
        case (0x70): AmIMM();   OpBVS(); suspend(2); break;
        case (0x71): AmINY_C(); OpADC(); suspend(5); break;
        case (0x72): AmIMP();   OpJAM(); suspend(0); break;
        case (0x73): AmINY();   OpRRA(); suspend(8); break;
        case (0x74): AmZPX();   OpDOP(); suspend(4); break;
        case (0x75): AmZPX();   OpADC(); suspend(4); break;
        case (0x76): AmZPX();   OpROR(); suspend(6); break;
        case (0x77): AmZPX();   OpRRA(); suspend(6); break;
        case (0x78): AmIMP();   OpSEI(); suspend(2); break;
        case (0x79): AmABY_C(); OpADC(); suspend(4); break;
        case (0x7A): AmIMP();   OpNOP(); suspend(2); break;
        case (0x7B): AmABY();   OpRRA(); suspend(7); break;
        case (0x7C): AmABX_C(); OpTOP(); suspend(4); break;
        case (0x7D): AmABX_C(); OpADC(); suspend(4); break;
        case (0x7E): AmABX();   OpROR(); suspend(7); break;
        case (0x7F): AmABX();   OpRRA(); suspend(7); break;
        case (0x80): AmIMM();   OpDOP(); suspend(2); break;
        case (0x81): AmINX();   OpSTA(); suspend(6); break;
        case (0x82): AmIMM();   OpDOP(); suspend(2); break;
        case (0x83): AmINX();   OpSAX(); suspend(6); break;
        case (0x84): AmZPG();   OpSTY(); suspend(3); break;
        case (0x85): AmZPG();   OpSTA(); suspend(3); break;
        case (0x86): AmZPG();   OpSTX(); suspend(3); break;
        case (0x87): AmZPG();   OpSAX(); suspend(3); break;
        case (0x88): AmIMP();   OpDEY(); suspend(2); break;
        case (0x89): AmIMM();   OpDOP(); suspend(2); break;
        case (0x8A): AmIMP();   OpTXA(); suspend(2); break;
        case (0x8B): AmIMM();   OpXAA(); suspend(2); break;
        case (0x8C): AmABS();   OpSTY(); suspend(4); break;
        case (0x8D): AmABS();   OpSTA(); suspend(4); break;
        case (0x8E): AmABS();   OpSTX(); suspend(4); break;
        case (0x8F): AmABS();   OpSAX(); suspend(4); break;
        case (0x90): AmIMM();   OpBCC(); suspend(2); break;
        case (0x91): AmINY();   OpSTA(); suspend(6); break;
        case (0x92): AmIMP();   OpJAM(); suspend(0); break;
        case (0x93): AmINY();   OpAHX(); suspend(6); break;
        case (0x94): AmZPX();   OpSTY(); suspend(4); break;
        case (0x95): AmZPX();   OpSTA(); suspend(4); break;
        case (0x96): AmZPY();   OpSTX(); suspend(4); break;
        case (0x97): AmZPY();   OpSAX(); suspend(4); break;
        case (0x98): AmIMP();   OpTYA(); suspend(2); break;
        case (0x99): AmABY();   OpSTA(); suspend(5); break;
        case (0x9A): AmIMP();   OpTXS(); suspend(2); break;
        case (0x9B): AmABY();   OpXAS(); suspend(5); break;
        case (0x9C): AmABX();   OpSHY(); suspend(5); break;
        case (0x9D): AmABX();   OpSTA(); suspend(5); break;
        case (0x9E): AmABY();   OpSHX(); suspend(5); break;
        case (0x9F): AmABY();   OpAHX(); suspend(5); break;
        case (0xA0): AmIMM();   OpLDY(); suspend(2); break;
        case (0xA1): AmINX();   OpLDA(); suspend(6); break;
        case (0xA2): AmIMM();   OpLDX(); suspend(2); break;
        case (0xA3): AmINX();   OpLAX(); suspend(6); break;
        case (0xA4): AmZPG();   OpLDY(); suspend(3); break;
        case (0xA5): AmZPG();   OpLDA(); suspend(3); break;
        case (0xA6): AmZPG();   OpLDX(); suspend(3); break;
        case (0xA7): AmZPG();   OpLAX(); suspend(3); break;
        case (0xA8): AmIMP();   OpTAY(); suspend(2); break;
        case (0xA9): AmIMM();   OpLDA(); suspend(2); break;
        case (0xAA): AmIMP();   OpTAX(); suspend(2); break;
        case (0xAB): AmIMM();   OpLAX(); suspend(2); break;
        case (0xAC): AmABS();   OpLDY(); suspend(4); break;
        case (0xAD): AmABS();   OpLDA(); suspend(4); break;
        case (0xAE): AmABS();   OpLDX(); suspend(4); break;
        case (0xAF): AmABS();   OpLAX(); suspend(4); break;
        case (0xB0): AmIMM();   OpBCS(); suspend(2); break;
        case (0xB1): AmINY_C(); OpLDA(); suspend(5); break;
        case (0xB2): AmIMP();   OpJAM(); suspend(0); break;
        case (0xB3): AmINY();   OpLAX(); suspend(5); break;
        case (0xB4): AmZPX();   OpLDY(); suspend(4); break;
        case (0xB5): AmZPX();   OpLDA(); suspend(4); break;
        case (0xB6): AmZPY();   OpLDX(); suspend(4); break;
        case (0xB7): AmZPY();   OpLAX(); suspend(4); break;
        case (0xB8): AmIMP();   OpCLV(); suspend(2); break;
        case (0xB9): AmABY_C(); OpLDA(); suspend(4); break;
        case (0xBA): AmIMP();   OpTSX(); suspend(2); break;
        case (0xBB): AmABY();   OpLAR(); suspend(4); break;
        case (0xBC): AmABX_C(); OpLDY(); suspend(4); break;
        case (0xBD): AmABX_C(); OpLDA(); suspend(4); break;
        case (0xBE): AmABY_C(); OpLDX(); suspend(4); break;
        case (0xBF): AmABY();   OpLAX(); suspend(4); break;
        case (0xC0): AmIMM();   OpCPY(); suspend(2); break;
        case (0xC1): AmINX();   OpCMP(); suspend(6); break;
        case (0xC2): AmIMM();   OpDOP(); suspend(2); break;
        case (0xC3): AmINX();   OpDCP(); suspend(8); break;
        case (0xC4): AmZPG();   OpCPY(); suspend(3); break;
        case (0xC5): AmZPG();   OpCMP(); suspend(3); break;
        case (0xC6): AmZPG();   OpDEC(); suspend(5); break;
        case (0xC7): AmZPG();   OpDCP(); suspend(5); break;
        case (0xC8): AmIMP();   OpINY(); suspend(2); break;
        case (0xC9): AmIMM();   OpCMP(); suspend(2); break;
        case (0xCA): AmIMP();   OpDEX(); suspend(2); break;
        case (0xCB): AmIMM();   OpAXS(); suspend(2); break;
        case (0xCC): AmABS();   OpCPY(); suspend(4); break;
        case (0xCD): AmABS();   OpCMP(); suspend(4); break;
        case (0xCE): AmABS();   OpDEC(); suspend(6); break;
        case (0xCF): AmABS();   OpDCP(); suspend(6); break;
        case (0xD0): AmIMM();   OpBNE(); suspend(2); break;
        case (0xD1): AmINY_C(); OpCMP(); suspend(5); break;
        case (0xD2): AmIMP();   OpJAM(); suspend(0); break;
        case (0xD3): AmINY();   OpDCP(); suspend(8); break;
        case (0xD4): AmZPX();   OpDOP(); suspend(4); break;
        case (0xD5): AmZPX();   OpCMP(); suspend(4); break;
        case (0xD6): AmZPX();   OpDEC(); suspend(6); break;
        case (0xD7): AmZPX();   OpDCP(); suspend(6); break;
        case (0xD8): AmIMP();   OpCLD(); suspend(2); break;
        case (0xD9): AmABY_C(); OpCMP(); suspend(4); break;
        case (0xDA): AmIMP();   OpNOP(); suspend(2); break;
        case (0xDB): AmABY();   OpDCP(); suspend(7); break;
        case (0xDC): AmABX_C(); OpTOP(); suspend(4); break;
        case (0xDD): AmABX_C(); OpCMP(); suspend(4); break;
        case (0xDE): AmABX();   OpDEC(); suspend(7); break;
        case (0xDF): AmABX();   OpDCP(); suspend(7); break;
        case (0xE0): AmIMM();   OpCPX(); suspend(2); break;
        case (0xE1): AmINX();   OpSBC(); suspend(6); break;
        case (0xE2): AmIMM();   OpDOP(); suspend(2); break;
        case (0xE3): AmINX();   OpISC(); suspend(8); break;
        case (0xE4): AmZPG();   OpCPX(); suspend(3); break;
        case (0xE5): AmZPG();   OpSBC(); suspend(3); break;
        case (0xE6): AmZPG();   OpINC(); suspend(5); break;
        case (0xE7): AmZPG();   OpISC(); suspend(5); break;
        case (0xE8): AmIMP();   OpINX(); suspend(2); break;
        case (0xE9): AmIMM();   OpSBC(); suspend(2); break;
        case (0xEA): AmIMP();   OpNOP(); suspend(2); break;
        case (0xEB): AmIMM();   OpSBC(); suspend(2); break;
        case (0xEC): AmABS();   OpCPX(); suspend(4); break;
        case (0xED): AmABS();   OpSBC(); suspend(4); break;
        case (0xEE): AmABS();   OpINC(); suspend(6); break;
        case (0xEF): AmABS();   OpISC(); suspend(6); break;
        case (0xF0): AmIMM();   OpBEQ(); suspend(2); break;
        case (0xF1): AmINY_C(); OpSBC(); suspend(5); break;
        case (0xF2): AmIMP();   OpJAM(); suspend(0); break;
        case (0xF3): AmINY();   OpISC(); suspend(8); break;
        case (0xF4): AmZPX();   OpDOP(); suspend(4); break;
        case (0xF5): AmZPX();   OpSBC(); suspend(4); break;
        case (0xF6): AmZPX();   OpINC(); suspend(6); break;
        case (0xF7): AmZPX();   OpISC(); suspend(6); break;
        case (0xF8): AmIMP();   OpSED(); suspend(2); break;
        case (0xF9): AmABY_C(); OpSBC(); suspend(4); break;
        case (0xFA): AmIMP();   OpNOP(); suspend(2); break;
        case (0xFB): AmABY();   OpISC(); suspend(7); break;
        case (0xFC): AmABX_C(); OpTOP(); suspend(4); break;
        case (0xFD): AmABX_C(); OpSBC(); suspend(4); break;
        case (0xFE): AmABX();   OpINC(); suspend(7); break;
        case (0xFF): AmABX();   OpISC(); suspend(7); break;
    }
}
