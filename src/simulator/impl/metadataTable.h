#ifndef metadataTable_h
#define metadataTable_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class MetadataTableCompute;
class MetadataTableRouter;

// [read requests, write requests]
typedef Block<2, MetadataTableCompute, MetadataTableRouter, "MetadataTable"> MetadataTable;

class MetadataTableRouter {
public:
	MetadataTableRouter(Device& device) : device(device) {}

	bool route(const Request& request, bool DRC_valid, bool CSA_valid);

	void debugPrint() const { /* printf("MetadataTableRouter\n"); */ }
private:
	Device& device;
};

class MetadataTableCompute {
public:
	MetadataTableCompute(MetadataTable& metadataTable) : metadataTable(metadataTable) { }

	const unsigned int READ_TIME = 10; // clock cycels
	const unsigned int WRITE_TIME = 20; // clock cycels

	void update() {
		if (currentRequest.has_value()) { // if the dram is doing anything
			timeLeft -= 1;
			if (timeLeft == 0) {
				metadataTable.outputRouter.route(currentRequest.value(), rand() % 2 == 0, rand() % 2 == 0); // we just assume it works
				currentRequest = std::nullopt;
			}
		} else {
			// do reads before writes
			currentRequest = metadataTable.blockInput.getNextRequest(0);
			if (currentRequest.has_value()) {
				timeLeft = READ_TIME;
				doingRead = true;
			} else {
				currentRequest = metadataTable.blockInput.getNextRequest(1);
				if (currentRequest.has_value()) {
					timeLeft = WRITE_TIME;
					doingRead = false;
				}
			}
		}
	}

	void debugPrint() const {
		printf("MetadataTableCompute:\t\t");
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
	MetadataTable& metadataTable;
};

#endif /* metadataTable_h */
