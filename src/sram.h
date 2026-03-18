#ifndef sram_h
#define sram_h

#include "block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class SramCompute;
class SramRouter;

// [read requests, write requests] (not device but sram)
typedef Block<2, SramCompute, SramRouter> Sram;

class SramRouter {
public:
	SramRouter(Device& device) : device(device) {}

	bool router(Request request) {
		return false;
	}

private:
	Device& device;
};

class SramCompute {
public:
	SramCompute(Sram& sram) : sram(sram) { }

	void update() {
		// do reads before writes
		std::optional<Request> request = sram.getIputInterface().getNextRequest(1);
		if (!request.has_value()) request = sram.getIputInterface().getNextRequest(0);

		// if we found any requests then give it to the router
		if (request.has_value()) {
			sram.outputRouter.router(request.value()); // we just assume it works
		}
	}
private:
	Sram& sram;
};

#endif /* sram_h */
