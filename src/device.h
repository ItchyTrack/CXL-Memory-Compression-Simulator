#ifndef device_h
#define device_h

#include "sram.h"

class Device {
public:
	Device() : sram(*this) { }

private:
	Sram sram;
};

#endif /* device_h */
