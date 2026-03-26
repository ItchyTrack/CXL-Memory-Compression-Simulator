#ifndef decompressor_h
#define decompressor_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class DecompressorCompute;
class DecompressorRouter;

// [read requests, write requests]
typedef Block<2, DecompressorCompute, DecompressorRouter> Decompressor;

class DecompressorRouter {
public:
	DecompressorRouter(Device& device) : device(device) {}

	bool route(const Request& request);

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
private:
	unsigned int end = 0;
	std::vector<std::optional<Request>> decompressing;
	Decompressor& decompressor;
};

#endif /* decompressor_h */
