#include "components.hpp"

void IAG::UpdateBuffer() { buffer_PC = PC; }

// int32_t increment = 0;
// switch (simulator -> control.MuxINC) {
//     case 0b0:
//         increment = simulator -> iag.INSTRUCTION_SIZE;
//         break;
//     case 0b1:
//         break;
//     default: break;
// }
void IAG::UpdatePC() {
    // Debug::log("MuxPC: " + to_string(simulator -> control.MuxPC));
    // Debug::log("Control Bit: " + to_string(control_bit));
    switch (simulator -> control.MuxPC) {
        case 0b0: // General Fetch
            PC = buffer_PC + INSTRUCTION_SIZE;
            // Debug::log("Normal PC Increment: " + to_string(buffer_PC) + " + " + to_string(INSTRUCTION_SIZE) + " = " + to_string(PC));
            break;
        case 0b1: // Effective Address (jalr)
            PC = simulator -> buffer.RZ;
            // Debug::log("In jalr");
            break;
        case 0b10: // Branches (SB and UJ)
            // Debug::log("Address: " + to_string(simulator -> instructions[0].address));
            // Debug::log("Valid: " + to_string(simulator -> btb.hasEntry(simulator -> instructions[0].address)));
            PC = simulator -> btb.getTargetAddress(simulator -> instructions[0].address);
            break;
        default: break;
    }
}