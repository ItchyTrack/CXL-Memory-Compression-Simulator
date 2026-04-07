#ifndef loggerBlock_h
#define loggerBlock_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

template <unsigned int INPUT_COUNT>
class LoggerBlockCompute;

// [read requests, write requests] (not device but sram)
template <unsigned int INPUT_COUNT>
using LoggerBlock = Block<INPUT_COUNT, LoggerBlockCompute<INPUT_COUNT>, "LoggerBlock">;

template <unsigned int INPUT_COUNT>
class LoggerBlockCompute {
	friend class SimulatorPanel;
	friend class RouterEditor;
public:
	LoggerBlockCompute(LoggerBlock<INPUT_COUNT>& loggerBlock) : loggerBlock(loggerBlock) { }

	void update() {
		// do reads before writes
		for (unsigned int i = 0; i < INPUT_COUNT; i++) {
			std::optional<Request> request = loggerBlock.blockInput.getNextRequest(i);
			if (!request.has_value()) continue;
			printf("INPUT %d: ", i);
			request->printInfo();
		}
	}

	void debugPrint() const { }
private:
	LoggerBlock<INPUT_COUNT>& loggerBlock;
};

#endif /* loggerBlock_h */
