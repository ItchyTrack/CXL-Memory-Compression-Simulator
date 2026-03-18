#ifndef blockInput_h
#define blockInput_h

#include "request.h"
#include <queue>
#include <array>
#include <optional>

template <unsigned int INPUT_COUNT>
class BlockInput {
public:
	// -------------------------- External API --------------------------

	// unlimited sized queues for now and so returns true
	bool canAcceptRequest(unsigned int inputIndex) const;
	bool pushRequest(unsigned int inputIndex, const Request& request);

	// -------------------------- Internal API --------------------------

	std::optional<Request> getNextRequest(unsigned int inputIndex);

private:
	std::array<std::queue<Request>, INPUT_COUNT> inputBuffers;
};

#endif /* blockInput_h */
