#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

class Controller {
public:
  Controller();

  enum Button {
    BUTTON_A,
    BUTTON_B,
    BUTTON_SELECT,
    BUTTON_START,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_TOTAL
  };

  void setStrobe(bool b);
  uint8_t poll();
  void press(Button b);
  void release(Button b);

private:
  bool strobe;
  bool buttons[BUTTON_TOTAL];
  int shiftReg;
};

#endif
