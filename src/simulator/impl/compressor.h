#ifndef compressor_h
#define compressor_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class CompressorCompute;
class CompressorRouter;

// [read requests, write requests]
typedef Block<1, CompressorCompute, CompressorRouter, "Compressor"> Compressor;

class CompressorRouter {
public:
	CompressorRouter(Device& device) : device(device) {}

	bool route(const Request& request);
	void debugPrint() const { /* printf("CompressorRouter\n"); */ }
private:
	Device& device;
};

class CompressorCompute {
public:
	const unsigned int PIPELINE_DEPTH = 10;

	CompressorCompute(Compressor& compressor) : compressor(compressor), compressing(PIPELINE_DEPTH, std::nullopt) { }

	void update() {
		// grab from pipeline
		if (compressing[end].has_value()) {
			compressor.outputRouter.route(compressing[end].value()); // we just assume it works
		}
		// read into pipeline
		compressing[end] = compressor.blockInput.getNextRequest(0);
		// increment pipeline location
		end = (end + 1) % PIPELINE_DEPTH;
	}

	void debugPrint() const {
		printf("CompressorCompute:\t\tqueue");
		for (unsigned int i = 0; i < PIPELINE_DEPTH; i++) {
			const std::optional<Request>& request = compressing[(end + i + 1) % PIPELINE_DEPTH];
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
	std::vector<std::optional<Request>> compressing;
	Compressor& compressor;
};

#endif /* compressor_h */
