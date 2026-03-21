#ifndef device_h
#define device_h

#include "sramCache.h"
#include "loggerBlock.h"

class Device {
public:
	void read(const Request& request);
	void write(const Request& request);
	Device() : sramCache(*this), loggerBlock(*this) { }
	void update() {
		sramCache.update();
		loggerBlock.update();
	}
	// these are public for the router
	SramCache sramCache;
	LoggerBlock<1> loggerBlock;
};

#endif /* device_h */
