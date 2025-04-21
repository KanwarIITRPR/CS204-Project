#include "components.hpp"

void IAG::UpdateBuffer() { buffer_PC = PC; }

void IAG::UpdatePC() { PC += INSTRUCTION_SIZE; }