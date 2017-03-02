#ifndef DIVIDER_H
#define DIVIDER_H

class Divider {
public:
    Divider(int interval = 1) {
	setInterval(interval);
	reset();
    }
    void setInterval(int interval) {
	if (interval < 1) {
	    resetValue = 0;
	}
	else {
	    resetValue = interval - 1;
	}
    }
    void reset() {
	clockCounter = resetValue;
    }
    void tick() {
	if (clockCounter == 0) {
	    clockCounter = resetValue;
	}
	else {
	    --clockCounter;
	}
    }
    bool hasClocked() {
	return clockCounter == resetValue;
    }

private:
    int resetValue;
    int clockCounter;
};

#endif
