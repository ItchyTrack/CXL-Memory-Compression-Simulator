#ifndef block_h
#define block_h

#include "blockInput.h"

class Device;

template <unsigned int INPUT_COUNT, class Compute, class OutputRouter>
class Block {
	friend Compute;
public:
	Block(Device& device) : compute(*this), outputRouter(device) { }

	BlockInput<INPUT_COUNT, Compute>& getIputInterface() { return blockInput; }
	const BlockInput<INPUT_COUNT, Compute>& getIputInterface() const { return blockInput; }

	void update();

private:
	BlockInput<INPUT_COUNT, Compute> blockInput;
	Compute compute;
	OutputRouter outputRouter;
};

template <unsigned int INPUT_COUNT, class Compute, class OutputRouter>
void Block<INPUT_COUNT, Compute, OutputRouter>::update() {
	compute.update();
}

#endif /* block_h */
