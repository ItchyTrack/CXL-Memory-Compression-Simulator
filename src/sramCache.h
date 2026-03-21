#ifndef sramCache_h
#define sramCache_h

#include "block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class SramCacheCompute;
class SramCacheRouter;

// [read requests, write requests] (not device but sram)
typedef Block<2, SramCacheCompute, SramCacheRouter> SramCache;

class SramCacheRouter {
public:
	SramCacheRouter(Device& device) : device(device) {}

	bool route(const Request& request);

private:
	Device& device;
};

class SramCacheCompute {
public:
	SramCacheCompute(SramCache& sramCache) : sramCache(sramCache) { }

	void update() {
		// do reads before writes
		std::optional<Request> request = sramCache.getIputInterface().getNextRequest(0);
		if (!request.has_value()) request = sramCache.getIputInterface().getNextRequest(1);

		// if we found any requests then give it to the router
		if (request.has_value()) {
			sramCache.outputRouter.route(request.value()); // we just assume it works
		}
	}
private:
	SramCache& sramCache;
};

#endif /* sramCache_h */
