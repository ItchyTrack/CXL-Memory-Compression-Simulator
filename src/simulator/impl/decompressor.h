#ifndef decompressor_h
#define decompressor_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class DecompressorCompute;
class DecompressorRouter;

// [read requests, write requests]
typedef Block<2, DecompressorCompute, DecompressorRouter, "Decompressor"> Decompressor;

class DecompressorRouter {
public:
	DecompressorRouter(Device& device) : device(device) {}

	bool route(const Request& request);
	void debugPrint() const { /* printf("DecompressorRouter\n"); */ }
private:
	Device& device;
};

class DecompressorCompute {
public:
	const unsigned int PIPELINE_DEPTH = 10;

	DecompressorCompute(Decompressor& decompressor) : decompressor(decompressor), decompressing(PIPELINE_DEPTH, std::nullopt) { }

	void update() {
		// grab from pipeline
		if (decompressing[end].has_value()) {
			decompressor.outputRouter.route(decompressing[end].value()); // we just assume it works
		}
		// read into pipeline
		decompressing[end] = decompressor.blockInput.getNextRequest(0);
		// increment pipeline location
		end = (end + 1) % PIPELINE_DEPTH;
	}

	void debugPrint() const {
		printf("DecompressorCompute:\t\tqueue");
		for (unsigned int i = 0; i < PIPELINE_DEPTH; i++) {
			const std::optional<Request>& request = decompressing[(end + i + 1) % PIPELINE_DEPTH];
			if (request.has_value()) {
				printf("[");
				request.value().printInfo();
				printf("]");
			} else {
				printf("[]");
			}
		}
		printf("\n");
	}
private:
	unsigned int end = 0;
	std::vector<std::optional<Request>> decompressing;
	Decompressor& decompressor;
};

#endif /* decompressor_h */
