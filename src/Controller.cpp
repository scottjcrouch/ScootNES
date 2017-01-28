#include <cstdint>

#include <Controller.h>

void Controller::setStrobe(bool b) {
    strobe = b;
    if(strobe) {
        shiftReg = -1;
    }
}

uint8_t Controller::poll() {
    if(strobe) {
        shiftReg = -1;
    }
    shiftReg++;
    if(shiftReg >= 8) {
        return 1;
    }
    else {
        return buttons[shiftReg];
    }
}

void Controller::press(Button b) {
    buttons[b] = 1;
}

void Controller::release(Button b) {
    buttons[b] = 0;
}
