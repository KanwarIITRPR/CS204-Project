#include "components.hpp"

bool ForwardingUnit::CheckForwardA_EX(uint32_t rs1) {
    // Forward from EX/MEM stage
    if (simulator -> instructions[2].rd != 0 && simulator -> instructions[2].rd == rs1) {
        return true;
    }
    return false;
}

bool ForwardingUnit::CheckForwardB_EX(uint32_t rs2) {
    // Forward from EX/MEM stage
    if (simulator -> instructions[2].rd != 0 && simulator -> instructions[2].rd == rs2) {
        return true;
    }
    return false;
}

bool ForwardingUnit::CheckForwardA_MEM(uint32_t rs1) {
    // Forward from MEM/WB stage
    if (simulator -> instructions[3].rd != 0 && simulator -> instructions[3].rd == rs1) {
        return true;
    }
    // No forwarding needed
    return false;
}

bool ForwardingUnit::CheckForwardB_MEM(uint32_t rs2) {
    if (simulator -> instructions[3].rd != 0 && simulator -> instructions[3].rd == rs2) {
        return true;
    }
    // No forwarding needed
    return false;
}