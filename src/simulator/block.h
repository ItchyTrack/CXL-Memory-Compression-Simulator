#ifndef block_h
#define block_h

#include "blockInput.h"

class Device;

template <auto N>
struct string_litteral {
    constexpr string_litteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }

    char value[N];
};

template <unsigned int INPUT_COUNT, class Compute, class OutputRouter, string_litteral NAME>
class Block {
	friend Compute;
public:
	Block(Device& device) : compute(*this), outputRouter(device) { }

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

template <unsigned int INPUT_COUNT, class Compute, class OutputRouter, string_litteral NAME>
void Block<INPUT_COUNT, Compute, OutputRouter, NAME>::update() {
	compute.update();
}

#endif /* block_h */
