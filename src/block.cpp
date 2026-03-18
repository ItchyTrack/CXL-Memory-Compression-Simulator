#include "block.h"

template <unsigned int INPUT_COUNT, class Compute, class OutputRouter>
void Block<INPUT_COUNT, Compute, OutputRouter>::update() {
	compute.update(this);
}
