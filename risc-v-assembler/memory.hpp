#ifndef MEMORY_H
#define MEMORY_H

#include "simulator.hpp"

struct MemoryRegisters {
    uint32_t MAR, MDR;
};

class ProcessorMemoryInterface {
    public:
        void GetInstructionValue(int bytes = 0);
        void GetDataValue(int bytes = 0);
        Instruction GetInstruction(uint32_t location);
        void StoreValue(int bytes = 0);

        map<uint32_t, uint8_t> data_map;
        map<uint32_t, Instruction> text_map;

        MemoryRegisters instruction_memory;
        MemoryRegisters data_memory;
};

#endif