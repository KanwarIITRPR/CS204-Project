#include "memory.hpp"

void ProcessorMemoryInterface::GetInstructionValue(int bytes) {
    if (instruction_memory.MAR >= DATA_ADDRESS) {
        error_stream << "Can't access instructions from data memory: " << instruction_memory.MAR << endl;
        return;
    }
    instruction_memory.MDR = text_map[instruction_memory.MAR].machine_code;
}

void ProcessorMemoryInterface::GetDataValue(int bytes) {
    if (data_memory.MAR < DATA_ADDRESS) {
        error_stream << "Can't access data from instruction memory: " << data_memory.MAR << endl;
        return;
    }
    
    int32_t final_value = 0;
    for (size_t i = 0; i < bytes; i++) {
        if (data_map.find(data_memory.MAR + i) == data_map.end()) break;
        final_value += data_map[data_memory.MAR + i] << (BYTE_SIZE * i);
    }
    data_memory.MDR = final_value;
}

Instruction ProcessorMemoryInterface::GetInstruction(uint32_t location) {
    if (location >= DATA_ADDRESS) {
        error_stream << "Can't get instruction from data memory: " << location << endl;
        return;
    }
    return text_map[location];
}

void ProcessorMemoryInterface::StoreValue(int bytes) {
    if (data_memory.MAR < DATA_ADDRESS) {
        error_stream << "Can't store data into instruction memory: " << data_memory.MAR << endl;
        return;
    }

    uint32_t data = data_memory.MDR;
    for (size_t i = 0; i < bytes; i++) {
        data_map[data_memory.MAR + i] = data & 0x000000FF;
        data = data >> BYTE_SIZE;
    }
}