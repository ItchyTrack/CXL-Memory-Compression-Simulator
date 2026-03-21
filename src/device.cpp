#include "device.h"

#include <cassert>

void Device::read(const Request& request) {
	assert(request.action == ActionType::READ);
	sramCache.getIputInterface().pushRequest(0, request);
}

void Device::write(const Request& request) {
	assert(request.action == ActionType::WRITE);
	sramCache.getIputInterface().pushRequest(1, request);
}
