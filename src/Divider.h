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
	    counterResetValue = 0;
	}
	else {
	    counterResetValue = interval - 1;
	}
    }
    void reset() {
	counter = counterResetValue;
    }
    void tick() {
	if (counter < 1) {
	    counter = counterResetValue;
	}
	else {
	    --counter;
	}
    }
    bool hasClocked() {
	return counter == counterResetValue;
    }

private:
    int counterResetValue;
    int counter;
};

#endif
