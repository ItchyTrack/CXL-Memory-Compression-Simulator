#ifndef dramDataCache_h
#define dramDataCache_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class DramDataCacheCompute;
class DramDataCacheRouter;

// [read requests, write requests]
typedef Block<2, DramDataCacheCompute, DramDataCacheRouter> DramDataCache;

class DramDataCacheRouter {
public:
	DramDataCacheRouter(Device& device) : device(device) {}

	bool route(const Request& request);

private:
	Device& device;
};

class DramDataCacheCompute {
public:
	DramDataCacheCompute(DramDataCache& dramDataCache) : dramDataCache(dramDataCache) { }

	void update() {
		// do reads before writes
		std::optional<Request> request = dramDataCache.blockInput.getNextRequest(0);
		if (!request.has_value()) request = dramDataCache.blockInput.getNextRequest(1);

		// if we found any requests then give it to the router
		if (request.has_value()) {
			dramDataCache.outputRouter.route(request.value()); // we just assume it works
		}
	}
private:
	DramDataCache& dramDataCache;
};

#endif /* dramDataCache_h */
