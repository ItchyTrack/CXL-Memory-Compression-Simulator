#ifndef loggerBlock_h
#define loggerBlock_h

#include "block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

 template <unsigned int INPUT_COUNT>
class LoggerBlockCompute;
class LoggerBlockRouter;

// [read requests, write requests] (not device but sram)
template <unsigned int INPUT_COUNT>
using LoggerBlock = Block<INPUT_COUNT, LoggerBlockCompute<INPUT_COUNT>, LoggerBlockRouter>;

class LoggerBlockRouter {
public:
	LoggerBlockRouter(Device& device) {}
	bool route(const Request& request) { return false; }
};

template <unsigned int INPUT_COUNT>
class LoggerBlockCompute {
public:
	LoggerBlockCompute(LoggerBlock<INPUT_COUNT>& loggerBlock) : loggerBlock(loggerBlock) { }

	void update() {
		// do reads before writes
		for (unsigned int i = 0; i < INPUT_COUNT; i++) {
			std::optional<Request> request = loggerBlock.getIputInterface().getNextRequest(i);
			if (!request.has_value()) continue;
			printf("INPUT %d: ", i);
			request->printInfo();
		}
	}
private:
	LoggerBlock<INPUT_COUNT>& loggerBlock;
};

#endif /* loggerBlock_h */
