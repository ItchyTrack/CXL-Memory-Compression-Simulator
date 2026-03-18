#ifndef block_h
#define block_h

#include "blockInput.h"

template <unsigned int INPUT_COUNT>
class Block {
public:
	BlockInput<INPUT_COUNT>& getIputInterface() { return blockInput; }
	const BlockInput<INPUT_COUNT>& getIputInterface() const { return blockInput; }

private:
	BlockInput<INPUT_COUNT> blockInput;
};

#endif /* block_h */
