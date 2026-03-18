#ifndef blockInput_h
#define blockInput_h

#include "request.h"
#include <array>
#include <optional>
#include <queue>

template <unsigned int INPUT_COUNT, class Compute>
class BlockInput {
	friend Compute;
public:
	// unlimited sized queues for now and so returns true
	bool canAcceptRequest(unsigned int inputIndex) const;
	bool pushRequest(unsigned int inputIndex, const Request& request);

private:
	std::optional<Request> getNextRequest(unsigned int inputIndex);

	std::array<std::queue<Request>, INPUT_COUNT> inputBuffers;
};

#endif /* blockInput_h */
