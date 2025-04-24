#include "components.hpp"

void HazardDetectionUnit::ExtractDataDependencies() {
    Instruction instructions[3];
    Debug::log("Entered HDU");

    uint32_t current_address = 0;

    if (simulator -> memory.text_map.find(current_address) != simulator -> memory.text_map.end()) instructions[0] = simulator -> memory.text_map.at(current_address);
    else return;
    
    while (!IsNullInstruction(instructions[0])) {
        pair<bool, bool> dependency_bits = {false, false};
        pair<uint32_t, uint32_t> dependency_address;
        instructions[1] = NextinstructionForDependency(instructions[0]);
        instructions[2] = NextinstructionForDependency(instructions[1]);

        if (!IsNullInstruction(instructions[1]) && HasDataDependency(instructions[0], instructions[1])) {
            dependency_bits.first = true;
            dependency_address.first = instructions[1].address;
            simulator -> data_hazards += 1;
        }
        
        if (!IsNullInstruction(instructions[2]) && HasDataDependency(instructions[0], instructions[2])) {
            dependency_bits.second = true;
            dependency_address.second = instructions[2].address;
            simulator -> data_hazards += 1;
        }

        if (dependency_bits.first || dependency_bits.second) {
            data_dependency_bits[current_address] = dependency_bits;
            data_dependency_map[current_address] = dependency_address;
        }

        Debug::log("Chained: \"" + instructions[0].literal + "\", \"" + instructions[1].literal + "\", \"" + instructions[2].literal + "\"");

        current_address += simulator -> iag.INSTRUCTION_SIZE;
        auto next_instruction = simulator -> memory.text_map.find(current_address);
        if (next_instruction != simulator -> memory.text_map.end()) instructions[0] = (*next_instruction).second;
        else instructions[0] = NULL_INSTRUCTION;
    }
}

bool HazardDetectionUnit::HasDataDependency(Instruction premier_instruction, Instruction former_instruction) {
    if (premier_instruction.format == Format::S || premier_instruction.format == Format::SB) return false;
    if (former_instruction.format == Format::U || former_instruction.format == Format::UJ) return false;

    bool rs1_dependent, rs2_dependent;
    rs1_dependent = premier_instruction.rd == former_instruction.rs1 && premier_instruction.rd != 0;
    if (former_instruction.format != Format::I) rs2_dependent = premier_instruction.rd == former_instruction.rs2 && premier_instruction.rd != 0;
    else rs2_dependent = false;

    return rs1_dependent || rs2_dependent;
}

Instruction HazardDetectionUnit::NextinstructionForDependency(Instruction current_instruction) {
    if (IsNullInstruction(current_instruction)) return NULL_INSTRUCTION;
    
    uint32_t address = current_instruction.address;
    if (!simulator -> btb.hasEntry(address) || !simulator -> btb.isUnconditionalBranch(address)) address += simulator -> iag.INSTRUCTION_SIZE;
    else address = simulator -> btb.getTargetAddress(address);

    auto next_instruction = simulator -> memory.text_map.find(address);
    if (next_instruction != simulator -> memory.text_map.end()) return (*next_instruction).second;
    else return NULL_INSTRUCTION;
}