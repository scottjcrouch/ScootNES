#ifndef DIVIDER_H
#define DIVIDER_H

class Divider {
public:
    Divider(int interval = 1) {
	setInterval(interval);
    }
    void setInterval(int interval) {
	this->interval = interval;
	clockCounter = interval - 1;
    }
    void tick() {
	clockCounter++;
	if (clockCounter == interval) {
	    clockCounter = 0;
	}
    }
    bool hasClocked() {
	return clockCounter == 0;
    }

private:
    int interval;
    int clockCounter;
};

#endif
