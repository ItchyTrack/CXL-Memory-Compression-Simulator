#ifndef blockInput_h
#define blockInput_h

#include "request.h"
#include <array>
#include <iostream>
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

template <unsigned int INPUT_COUNT, class Compute>
bool BlockInput<INPUT_COUNT, Compute>::canAcceptRequest(unsigned int inputIndex) const {
	if (INPUT_COUNT <= inputIndex) {
		std::cout << "ERROR: wrong input index passed to BlockInput::canAcceptRequest!\n";
		return false;
	}
	return true;
}

template <unsigned int INPUT_COUNT, class Compute>
bool BlockInput<INPUT_COUNT, Compute>::pushRequest(unsigned int inputIndex, const Request& request) {
	if (!canAcceptRequest(inputIndex)) return false;
	inputBuffers[inputIndex].push(request);
	return true;
}

template <unsigned int INPUT_COUNT, class Compute>
std::optional<Request> BlockInput<INPUT_COUNT, Compute>::getNextRequest(unsigned int inputIndex) {
	if (INPUT_COUNT <= inputIndex) {
		std::cout << "ERROR: wrong input index passed to BlockInput::getNextRequest!\n";
		return std::nullopt;
	}
	if (inputBuffers[inputIndex].empty()) return std::nullopt;
	Request request = std::move(inputBuffers[inputIndex].front());
	inputBuffers[inputIndex].pop();
	return request;
}

#endif /* blockInput_h */
