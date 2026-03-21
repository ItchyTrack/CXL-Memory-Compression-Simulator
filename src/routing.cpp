#include "sramCache.h"
#include "device.h"

// all the routing happens in here because then it is easier to find and update everything

bool SramCacheRouter::route(const Request& request) {
	device.loggerBlock.getIputInterface().pushRequest(0, request);
	return true;
}
