#include "blockInput.h"
#include <iostream>

template <unsigned int INPUT_COUNT>
bool BlockInput<INPUT_COUNT>::canAcceptRequest(unsigned int inputIndex) const {
	if (INPUT_COUNT <= inputIndex) {
		std::cout << "ERROR: wrong input index passed to BlockInput::canAcceptRequest!\n";
		return false;
	}
	return true;
}

template <unsigned int INPUT_COUNT>
bool BlockInput<INPUT_COUNT>::pushRequest(unsigned int inputIndex, const Request& request) {
	if (!canAcceptRequest(inputIndex)) return false;
	inputBuffers[inputIndex].push(request);
}

template <unsigned int INPUT_COUNT>
std::optional<Request> BlockInput<INPUT_COUNT>::getNextRequest(unsigned int inputIndex) {
	if (INPUT_COUNT <= inputIndex) {
		std::cout << "ERROR: wrong input index passed to BlockInput::getNextRequest!\n";
		return std::nullopt;
	}
	Request request = std::move(inputBuffers[inputIndex].front());
	inputBuffers[inputIndex].pop();
	return request;
}
