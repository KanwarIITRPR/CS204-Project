#include "components.hpp"

void ControlCircuit::UpdateControlSignals() {
    UpdateDecodeSignals();
    UpdateExecuteSignals();
    UpdateMemorySignals();
    UpdateWritebackSignals();
}

void ControlCircuit::UpdateDecodeSignals() {
    bool data_forwarding_A_EX = false, data_forwarding_B_EX = false, data_forwarding_A_MEM = false, data_forwarding_B_MEM = false;
    if (simulator -> hasDataForwarding && !IsNullInstruction(simulator -> instructions[2])) {
        Debug::log("Signalling data forwarding EX to EX: " + simulator -> instructions[2].literal + " to " + simulator -> instructions[1].literal);
        auto dependent_pair = simulator -> hdu.data_dependency_bits.find(simulator -> instructions[2].address);
        if (dependent_pair != simulator -> hdu.data_dependency_bits.end()) {
            Debug::log(to_string((*dependent_pair).second.first));
            Debug::log("Forwarder address: " + to_string(simulator -> hdu.data_dependency_map.at(simulator -> instructions[2].address).first));
            Debug::log("Current address: " + to_string(simulator -> instructions[1].address));
        }
        if (simulator -> instructions[1].rs1 == simulator -> instructions[2].rd || simulator -> instructions[1].rs2 == simulator -> instructions[2].rd) {
            data_forwarding_A_EX = simulator -> forwarding_unit.CheckForwardA_EX(simulator -> instructions[1].rs1);
            data_forwarding_B_EX = simulator -> forwarding_unit.CheckForwardB_EX(simulator -> instructions[1].rs2);
        }
        Debug::log("Data forwarding EX to EX: " + to_string(data_forwarding_A_EX) + " (rd and rs1) & " + to_string(data_forwarding_B_EX) + " (rd and rs2)");
    }
    
    if (simulator -> hasDataForwarding && !IsNullInstruction(simulator -> instructions[3])) {
        Debug::log("Signalling data forwarding MEM to EX: " + simulator -> instructions[3].literal + " to " + simulator -> instructions[1].literal);
        auto dependent_pair = simulator -> hdu.data_dependency_bits.find(simulator -> instructions[3].address);
        if (dependent_pair != simulator -> hdu.data_dependency_bits.end()) {
            Debug::log(to_string((*dependent_pair).second.second));
            Debug::log("Forwarder address: " + to_string(simulator -> hdu.data_dependency_map.at(simulator -> instructions[3].address).second));
            Debug::log("Current address: " + to_string(simulator -> instructions[1].address));
        }
        Debug::log("Decode's rs1: " + to_string(simulator -> instructions[1].rs1) + "Decode's rs2: " + to_string(simulator -> instructions[1].rs2) + "Memory Access's rd: " + to_string(simulator -> instructions[3].rd));
        if (simulator -> instructions[1].rs1 == simulator -> instructions[3].rd || simulator -> instructions[1].rs2 == simulator -> instructions[3].rd) {
                data_forwarding_A_MEM = simulator -> forwarding_unit.CheckForwardA_MEM(simulator -> instructions[1].rs1);
                data_forwarding_B_MEM = simulator -> forwarding_unit.CheckForwardB_MEM(simulator -> instructions[1].rs2);
        }
        Debug::log("Data forwarding MEM to EX: " + to_string(data_forwarding_A_MEM) + " (rd and rs1) & " + to_string(data_forwarding_B_MEM) + " (rd and rs2)");
    }

    if (data_forwarding_A_EX) MuxA = 0b100;
    else if (data_forwarding_A_MEM) MuxA = 0b101;
    else
        switch (simulator -> instructions[1].opcode) {
        case 0b0110011: // R-Type
            MuxA = 0b1;
            break;
        case 0b0010011: // I-Type (Normal Immediate)
            MuxA = 0b1;
            break;
        case 0b0000011: // I-Type (Load)
            MuxA = 0b1;
            break;
        case 0b1100111: // I-Type (Return)
            MuxA = 0b1;
            break;
        case 0b0100011: // S-Type
            MuxA = 0b1;
            break;;
        case 0b1100011: // SB-Type
            MuxA = 0b1;
            break;
        case 0b0110111: // U-Type (Normal Upper Immediate)
            MuxA = 0b10;
            break;
        case 0b0010111: // U-Type (PC-added Upper Immediate)
            MuxA = 0b11;
            break;
        case 0b1101111: // UJ-Type
            MuxA = 0b0;
            break;
        default: return;
        }

    if (data_forwarding_B_EX)
        switch (simulator -> instructions[1].opcode) {
        case 0b0100011: // S-Type
            MuxB = 0b110;
            break;
        default:
            MuxB = 0b101;
            break;
        }
    else if (data_forwarding_B_MEM)
        switch (simulator -> instructions[1].opcode) {
        case 0b0100011: // S-Type
            MuxB = 0b1000;
            break;
        default:
            MuxB = 0b111;
            break;
        }
    else
        switch (simulator -> instructions[1].opcode) {
        case 0b0110011: // R-Type
            MuxB = 0b1;
            break;
        case 0b0010011: // I-Type (Normal Immediate)
            MuxB = 0b10;
            break;
        case 0b0000011: // I-Type (Load)
            MuxB = 0b10;
            break;
        case 0b1100111: // I-Type (Return)
            MuxB = 0b10;
            break;
        case 0b0100011: // S-Type
            MuxB = 0b11;
            break;
        case 0b1100011: // SB-Type
            MuxB = 0b1;
            break;
        case 0b0110111: // U-Type (Normal Upper Immediate)
            MuxB = 0b100;
            break;
        case 0b0010111: // U-Type (PC-added Upper Immediate)
            MuxB = 0b10;
            break;
        case 0b1101111: // UJ-Type
            MuxB = 0b0;
            break;
        default: return;
        }
}

void ControlCircuit::UpdateExecuteSignals() {
    switch (simulator -> instructions[2].opcode) {
    case 0b0110011: // R-Type
        switch (simulator -> instructions[2].funct3) {
        case 0b000: // add, sub, mul
            switch (simulator -> instructions[2].funct7) {
            case 0b0000000: // add
                ALU = 0b1;
                break;
            case 0b0100000: // sub
                ALU = 0b10;
                break;
            case 0b0000001: // mul
                ALU = 0b11;
                break;
            }
            break;
        case 0b001: // sll
            ALU = 0b1001;
            break;
        case 0b010: // slt
            ALU = 0b1100;
            break;
        case 0b100:
            switch (simulator -> instructions[2].funct7) {
            case 0b0000001: // div
                ALU = 0b100;
                break;
            case 0b0000000: // xor
                ALU = 0b110;
                break;
            }
            break;
        case 0b101:
            switch (simulator -> instructions[2].funct7) {
            case 0b0100000: // sra
                ALU = 0b1011;
                break;
            case 0b0000000: // srl
                ALU = 0b1010;
                break;
            }
            break;
        case 0b110:
            switch (simulator -> instructions[2].funct7) {
            case 0b0000001: // rem
                ALU = 0b101;
                break;
            case 0b0000000: // or
                ALU = 0b111;
                break;
            }
            break;
        case 0b111: // and
            ALU = 0b1000;
            break;
        }
        break;
    case 0b0010011: // I-Type (Normal Immediate)
        switch (simulator -> instructions[2].funct3) {
        case 0b000: // addi
            ALU = 0b1;
            break;
        case 0b100: // xori
            ALU = 0b110;
            break;
        case 0b110: // ori
            ALU = 0b111;
            break;
        case 0b111: // andi
            ALU = 0b1000;
            break;
        }
        break;
    case 0b0000011: // I-Type (Load)
        ALU = 0b1;
        break;
    case 0b1100111: // I-Type (Return)
        ALU = 0b10010;
        break;
    case 0b0100011: // S-Type
        ALU = 0b1;
        break;
    case 0b1100011: // SB-Type
        switch (simulator -> instructions[2].funct3) {
        case 0b000: // beq
            ALU = 0b1101;
            break;
        case 0b001: // bne
            ALU = 0b1110;
            break;
        case 0b100: // blt
            ALU = 0b1111;
            break;
        case 0b101: // bge
            ALU = 0b10000;
            break;
        }
        break;
    case 0b0110111: // U-Type (Normal Upper Immediate)
        ALU = 0b1001;
        break;
    case 0b0010111: // U-Type (PC-added Upper Immediate)
        ALU = 0b10001;
        break;
    case 0b1101111: // UJ-Type
        ALU = 0b0;
        break;
    default: return;
    }
}

void ControlCircuit::UpdateMemorySignals() {
    switch (simulator -> instructions[3].opcode) {
        case 0b0110011: // R-Type
            MuxMA = 0b0;
            break;
        case 0b0010011: // I-Type (Normal Immediate)
            MuxMA = 0b0;
            break;
        case 0b0000011: // I-Type (Load)
            MuxMA = 0b1;
            break;
        case 0b1100111: // I-Type (Return)
            MuxMA = 0b0;
            break;
        case 0b0100011: // S-Type
            MuxMA = 0b1;
            break;
        case 0b1100011: // SB-Type
            MuxMA = 0b0;
            break;
        case 0b0110111: // U-Type (Normal Upper Immediate)
            MuxMA = 0b0;
            break;
        case 0b0010111: // U-Type (PC-added Upper Immediate)
            MuxMA = 0b0;
            break;
        case 0b1101111: // UJ-Type
            MuxMA = 0b0;
            break;
        default: return;
    }

    switch (simulator -> instructions[3].opcode) {
    case 0b0110011: // R-Type
        MuxMD = 0b0;
        break;
    case 0b0010011: // I-Type (Normal Immediate)
        MuxMD = 0b0;
        break;
    case 0b0000011: // I-Type (Load)
        switch (simulator -> instructions[3].funct3) {
        case 0b000: // lb
            MuxMD = 0b1;
            break;
        case 0b001: // lh
            MuxMD = 0b10;
            break;
        case 0b010: // lw
            MuxMD = 0b11;
            break;
        }
        break;
    case 0b1100111: // I-Type (Return)
        MuxMD = 0b0;
        break;
    case 0b0100011: // S-Type
        switch (simulator -> instructions[3].funct3) {
        case 0b000: // sb
            MuxMD = 0b100;
            break;
        case 0b001: // sh
            MuxMD = 0b101;
            break;
        case 0b010: // sw
            MuxMD = 0b110;
            break;
        }
        break;
    case 0b1100011: // SB-Type
        MuxMD = 0b0;
        break;
    case 0b0110111: // U-Type (Normal Upper Immediate)
        MuxMD = 0b0;
        break;
    case 0b0010111: // U-Type (PC-added Upper Immediate)
        MuxMD = 0b0;
        break;
    case 0b1101111: // UJ-Type
        MuxMD = 0b0;
        break;
    default: return;
    }

    switch (simulator -> instructions[3].opcode) {
        case 0b0110011: // R-Type
            MuxY = 0b1;
            break;
        case 0b0010011: // I-Type (Normal Immediate)
            MuxY = 0b1;
            break;
        case 0b0000011: // I-Type (Load)
            MuxY = 0b10;
            break;
        case 0b1100111: // I-Type (Return)
            MuxY = 0b11;
            break;
        case 0b0100011: // S-Type
            MuxY = 0b0;
            break;
        case 0b1100011: // SB-Type
            MuxY = 0b0;
            break;
        case 0b0110111: // U-Type (Normal Upper Immediate)
            MuxY = 0b1;
            break;
        case 0b0010111: // U-Type (PC-added Upper Immediate)
            MuxY = 0b1;
            break;
        case 0b1101111: // UJ-Type
            MuxY = 0b11;
            break;
        default: return;
    }
}

void ControlCircuit::UpdateWritebackSignals() {
    switch (simulator -> instructions[4].opcode) {
        case 0b0110011: // R-Type
            EnableRegisterFile = 0b1;
            break;
        case 0b0010011: // I-Type (Normal Immediate)
            EnableRegisterFile = 0b1;
            break;
        case 0b0000011: // I-Type (Load)
            EnableRegisterFile = 0b1;
            break;
        case 0b1100111: // I-Type (Return)
            EnableRegisterFile = 0b1;
            break;
        case 0b0100011: // S-Type
            EnableRegisterFile = 0b0;
            break;
        case 0b1100011: // SB-Type
            EnableRegisterFile = 0b0;
            break;
        case 0b0110111: // U-Type (Normal Upper Immediate)
            EnableRegisterFile = 0b1;
            break;
        case 0b0010111: // U-Type (PC-added Upper Immediate)
            EnableRegisterFile = 0b1;
            break;
        case 0b1101111: // UJ-Type
            EnableRegisterFile = 0b1;
            break;
        default: return;
    }
}

void ControlCircuit::UpdateIAGSignals() {
    if (simulator -> instructions[2].literal.substr(0, simulator -> instructions[2].literal.find(" ")) == "jalr") { 
        MuxPC = 0b1; 
        simulator -> control_instructions += 1;
        simulator -> control_hazards += 1;
        Debug::log("jalr in execute selected");
    } else if (simulator -> instructions[0].format == Format::SB || simulator -> instructions[0].format == Format::UJ) {
        simulator -> control_instructions += 1;
        if (simulator -> instructions[0].format == Format::SB) simulator -> control_hazards += 1;
        Debug::log("Entered PC Change: " + simulator -> instructions[0].literal);
        if (simulator -> btb.isUnconditionalBranch(simulator -> instructions[0].address)) { MuxPC = 0b10; Debug::log("Unconditional selected: " + to_string(MuxPC)); }
        else if (simulator -> pht.getPrediction(simulator -> instructions[0].address)) { MuxPC = 0b10; Debug::log("Conditional predicted taken"); }
        else { MuxPC = 0b0; Debug::log("Conditional predicted not-taken"); }
    } else { MuxPC = 0b0; Debug::log("Standard PC increment selected"); }
}