#ifndef compressedStorage_h
#define compressedStorage_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class CompressedStorageCompute;
class CompressedStorageRouter;

// [read requests, write requests]
typedef Block<2, CompressedStorageCompute, CompressedStorageRouter, "CompressedStorage"> CompressedStorage;

class CompressedStorageRouter {
public:
	CompressedStorageRouter(Device& device) : device(device) {}

	bool route(const Request& request);
	void debugPrint() const { /*printf("CompressedStorageRouter\n");*/ }
private:
	Device& device;
};

class CompressedStorageCompute {
public:
	CompressedStorageCompute(CompressedStorage& compressedStorage) : compressedStorage(compressedStorage) { }

	const unsigned int READ_TIME = 10; // clock cycels
	const unsigned int WRITE_TIME = 20; // clock cycels

	void update() {
		if (currentRequest.has_value()) { // if the dram is doing anything
			timeLeft -= 1;
			if (timeLeft == 0) {
				compressedStorage.outputRouter.route(currentRequest.value()); // we just assume it works
				currentRequest = std::nullopt;
			}
		} else {
			// do reads before writes
			currentRequest = compressedStorage.blockInput.getNextRequest(0);
			if (currentRequest.has_value()) {
				timeLeft = READ_TIME;
			} else {
				currentRequest = compressedStorage.blockInput.getNextRequest(1);
				if (currentRequest.has_value()) {
					timeLeft = WRITE_TIME;
				}
			}
		}
	}

	void debugPrint() const {
		printf("CompressedStorageCompute:\t\t");
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
	CompressedStorage& compressedStorage;
};

#endif /* compressedStorage_h */
