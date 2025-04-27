#include "components.hpp"

pair<bool, bool> HazardDetectionUnit::GetDataDependency(Instruction premier_instruction, Instruction former_instruction) {
    if (premier_instruction.format == Format::S || premier_instruction.format == Format::SB) return {false, false};
    if (former_instruction.format == Format::U || former_instruction.format == Format::UJ) return {false, false};

    
    bool rs1_dependent, rs2_dependent;
    rs1_dependent = premier_instruction.rd == former_instruction.rs1 && premier_instruction.rd != 0;
    if (former_instruction.format != Format::I) rs2_dependent = premier_instruction.rd == former_instruction.rs2 && premier_instruction.rd != 0;
    else rs2_dependent = false;

    Debug::log("RS1 Dependence: " + premier_instruction.literal + " and " + former_instruction.literal + " is " + to_string(rs1_dependent));
    Debug::log("RS2 Dependence: " + premier_instruction.literal + " and " + former_instruction.literal + " is " + to_string(rs2_dependent));

    return {rs1_dependent, rs2_dependent};
}

void HazardDetectionUnit::CalculateDataDependency() {
    CalculateDataDependency(simulator -> instructions[1], simulator -> instructions[2], simulator -> instructions[3]);
}

void HazardDetectionUnit::CalculateDataDependency(Instruction current_instruction, Instruction EX_instruction, Instruction MEM_instruction) {
    if (IsNullInstruction(current_instruction)) return;
    
    pair<bool, bool> EX_forwarding = {false, false}, MEM_forwarding = {false, false};
    if (!IsNullInstruction(EX_instruction))
        Debug::log("EX to EX data forwarding from " + EX_instruction.literal + " to " + current_instruction.literal);
        EX_forwarding = GetDataDependency(EX_instruction, current_instruction);
        
    if (!IsNullInstruction(MEM_instruction))
        Debug::log("EX to EX data forwarding from " + EX_instruction.literal + " to " + current_instruction.literal);
        MEM_forwarding = GetDataDependency(MEM_instruction, current_instruction);

    dependency_A_EX = EX_forwarding.first;
    dependency_B_EX = EX_forwarding.second;
    dependency_A_MEM = MEM_forwarding.first;
    dependency_B_MEM = MEM_forwarding.second;

    if (dependency_A_EX || dependency_A_MEM || dependency_B_EX || dependency_B_MEM) simulator -> data_hazards += 1;
}

bool HazardDetectionUnit::IsNextDependent() {
    if (IsNullInstruction(simulator -> instructions[0])) return false;
    CalculateDataDependency(simulator -> instructions[0], simulator -> instructions[1], simulator -> instructions[2]);
    return dependency_A_EX || dependency_A_MEM || dependency_B_EX || dependency_B_MEM;
}

bool HazardDetectionUnit::hasEXtoEXDependency() {
    if (IsNullInstruction(simulator -> instructions[1])) return false;
    return dependency_A_EX || dependency_B_EX;
}

bool HazardDetectionUnit::hasMEMtoEXDependency() {
    if (IsNullInstruction(simulator -> instructions[1])) return false;
    return dependency_A_MEM || dependency_B_MEM;
}