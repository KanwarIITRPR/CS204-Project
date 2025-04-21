#include "components.hpp"

Instruction ProcessorMemoryInterface::GetInstruction() {
    if (instruction_memory.MAR >= DATA_ADDRESS) {
        error_stream << "Can't access instructions from data memory: " << instruction_memory.MAR << endl;
        return NULL_INSTRUCTION;
    }

    auto instruction_pair = text_map.find(instruction_memory.MAR);
    if (instruction_pair == text_map.end()) return NULL_INSTRUCTION;
    
    instruction_memory.MDR = (*instruction_pair).second.machine_code;
    return (*instruction_pair).second;
}

void ProcessorMemoryInterface::GetDataValue(int bytes) {
    if (data_memory.MAR < DATA_ADDRESS) {
        error_stream << "Can't access data from instruction memory: " << data_memory.MAR << endl;
        return;
    }
    
    int32_t final_value = 0;
    for (size_t i = 0; i < bytes; i++) {
        final_value += data_map[data_memory.MAR + i] << (BYTE_SIZE * i);
    }
    data_memory.MDR = final_value;
}

void ProcessorMemoryInterface::StoreDataValue(int bytes) {
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

void ProcessorMemoryInterface::AddInstruction(uint32_t location, Instruction instruction) { text_map[location] = instruction; }
void ProcessorMemoryInterface::AddData(uint32_t location, uint32_t data, int bytes) {
    for (size_t i = 0; i < bytes; i++) {
        data_map[location + i] = data & 0x000000FF;
        data = data >> BYTE_SIZE;
    }
}