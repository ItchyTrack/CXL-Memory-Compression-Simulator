#ifndef compressedStorage_h
#define compressedStorage_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class CompressedStorageCompute;
class CompressedStorageRouter;

// [read requests, write requests]
typedef Block<2, CompressedStorageCompute, CompressedStorageRouter> CompressedStorage;

class CompressedStorageRouter {
public:
	CompressedStorageRouter(Device& device) : device(device) {}

	bool route(const Request& request);

private:
	Device& device;
};

class CompressedStorageCompute {
public:
	CompressedStorageCompute(CompressedStorage& compressedStorage) : compressedStorage(compressedStorage) { }

	void update() {
		// do reads before writes
		std::optional<Request> request = compressedStorage.blockInput.getNextRequest(0);
		if (!request.has_value()) request = compressedStorage.blockInput.getNextRequest(1);

		// if we found any requests then give it to the router
		if (request.has_value()) {
			compressedStorage.outputRouter.route(request.value()); // we just assume it works
		}
	}
private:
	CompressedStorage& compressedStorage;
};

#endif /* compressedStorage_h */
