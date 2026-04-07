#ifndef block_h
#define block_h

#include "blockInput.h"
#include "router.h"

class Device;

template <auto N>
struct string_litteral {
	constexpr string_litteral(const char (&str)[N]) { std::copy_n(str, N, value); }
	char value[N];
};

class OutputRouter {
public:
	OutputRouter(Device& device, const char* blockName) : device(device), blockName(blockName) { }
	const char* getBlockName() { return blockName; }
	bool route(const Request& request, const RouteArgs& args);
	void debugPrint() const { }
private:
	const char* blockName;
	Device& device;
};

template <unsigned int INPUT_COUNT, class Compute, string_litteral NAME>
class Block {
	friend class SimulatorPanel;
	friend class RouterEditor;
	friend Compute;
public:
	Block(Device& device) : compute(*this), outputRouter(device, NAME.value) { }

	BlockInput<INPUT_COUNT, Compute>& getIputInterface() { return blockInput; }
	const BlockInput<INPUT_COUNT, Compute>& getIputInterface() const { return blockInput; }

	void update();
	void debugPrint() const {
		printf("-- %s --\n", NAME.value);
		blockInput.debugPrint();
		compute.debugPrint();
		outputRouter.debugPrint();
	}

private:
	BlockInput<INPUT_COUNT, Compute> blockInput;
	Compute compute;
	OutputRouter outputRouter;
};

template <unsigned int INPUT_COUNT, class Compute, string_litteral NAME>
void Block<INPUT_COUNT, Compute, NAME>::update() {
	compute.update();
}

#endif /* block_h */
