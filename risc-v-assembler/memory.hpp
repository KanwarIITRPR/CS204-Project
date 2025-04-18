#ifndef MEMORY_H
#define MEMORY_H

#include "simulator.hpp"

struct MemoryRegisters {
    uint32_t MAR, MDR;
};

class ProcessorMemoryInterface {
    public:
        // Used for load/store instructions
        Instruction GetInstruction();
        void GetDataValue(int bytes = 0);
        void StoreDataValue(int bytes = 0);

        // Used for initial parsing
        void AddInstruction(uint32_t location, Instruction instruction);
        void AddData(uint32_t location, uint32_t data, int bytes = 0);

        map<uint32_t, uint8_t> data_map;
        map<uint32_t, Instruction> text_map;

        MemoryRegisters instruction_memory;
        MemoryRegisters data_memory;
};

#endif