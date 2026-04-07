#ifndef sramCache_h
#define sramCache_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class SramCacheCompute;

// [read requests, write requests] (not device but sram)
typedef Block<2, SramCacheCompute, "SramCache"> SramCache;

class SramCacheCompute {
	friend class SimulatorPanel;
	friend class RouterEditor;
public:
	SramCacheCompute(SramCache& sramCache) : sramCache(sramCache) { }

	void update() {
		// do reads before writes
		std::optional<Request> request = sramCache.blockInput.getNextRequest(0);
		if (!request.has_value()) request = sramCache.blockInput.getNextRequest(1);

		// if we found any requests then give it to the router
		if (request.has_value()) {
			sramCache.outputRouter.route(
				request.value(),
				RouteArgs{ { "DRC_valid", rand() % 2 == 0 }, { "CSA_valid", rand() % 2 == 0 } }
			); // we just assume it works because there is no input buffer limit
		}
	}

	void debugPrint() const { /* printf("SramCacheCompute\n"); */ }
private:
	SramCache& sramCache;
};

#endif /* sramCache_h */
