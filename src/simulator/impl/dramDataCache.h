#ifndef dramDataCache_h
#define dramDataCache_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class DramDataCacheCompute;
class DramDataCacheRouter;

// [read requests, write requests]
typedef Block<2, DramDataCacheCompute, "DramDataCache"> DramDataCache;

class DramDataCacheCompute {
	friend class SimulatorPanel;
	friend class RouterEditor;
public:
	DramDataCacheCompute(DramDataCache& dramDataCache) : dramDataCache(dramDataCache) { }

	const unsigned int READ_TIME = 10;	// clock cycels
	const unsigned int WRITE_TIME = 20; // clock cycels

	void update() {
		if (currentRequest.has_value()) { // if the dram is doing anything
			timeLeft -= 1;
			if (timeLeft == 0) {
				dramDataCache.outputRouter.route(currentRequest.value(), RouteArgs{}); // we just assume it works
				currentRequest = std::nullopt;
			}
		} else {
			// do reads before writes
			currentRequest = dramDataCache.blockInput.getNextRequest(0);
			if (currentRequest.has_value()) {
				timeLeft = READ_TIME;
			} else {
				currentRequest = dramDataCache.blockInput.getNextRequest(1);
				if (currentRequest.has_value()) {
					timeLeft = WRITE_TIME;
				}
			}
		}
	}

	void debugPrint() const {
		printf("DramDataCacheCompute:\t\t");
		if (currentRequest.has_value()) {
			if (doingRead) {
				printf("READ Time Left %u, ", timeLeft);
				currentRequest.value().printInfo();
			} else {
				printf("WRITE Time Left %u, ", timeLeft);
				currentRequest.value().printInfo();
			}
		} else {
			printf("IDLE\n");
		}
	}
private:
	unsigned timeLeft = 0;
	std::optional<Request> currentRequest;
	bool doingRead = false;
	DramDataCache& dramDataCache;
};

#endif /* dramDataCache_h */
