#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

class Controller {
public:
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
  bool strobe = false;
  bool buttons[BUTTON_TOTAL] = {false};
  int shiftReg = -1;
};

#endif
