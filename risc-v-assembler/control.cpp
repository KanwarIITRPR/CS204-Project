#include "components.hpp"

void ControlCircuit::UpdateControlSignals() {
    UpdateDecodeSignals();
    UpdateExecuteSignals();
    UpdateMemorySignals();
    UpdateWritebackSignals();
}

void ControlCircuit::UpdateDecodeSignals() {
    string command = simulator -> instructions[1].literal.substr(0, simulator -> instructions[1].literal.find(" "));

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
        // if (dependent_pair != simulator -> hdu.data_dependency_bits.end() && (*dependent_pair).second.first &&
        // simulator -> hdu.data_dependency_map.at(simulator -> instructions[2].address).first == simulator -> instructions[1].address) {
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
        // if (dependent_pair != simulator -> hdu.data_dependency_bits.end() && (*dependent_pair).second.second &&
        // simulator -> hdu.data_dependency_map.at(simulator -> instructions[3].address).second == simulator -> instructions[1].address) {
                data_forwarding_A_MEM = simulator -> forwarding_unit.CheckForwardA_MEM(simulator -> instructions[1].rs1);
                data_forwarding_B_MEM = simulator -> forwarding_unit.CheckForwardB_MEM(simulator -> instructions[1].rs2);
        }
        Debug::log("Data forwarding MEM to EX: " + to_string(data_forwarding_A_MEM) + " (rd and rs1) & " + to_string(data_forwarding_B_MEM) + " (rd and rs2)");
    }

    if (data_forwarding_A_EX) MuxA = 0b100;
    else if (data_forwarding_A_MEM) MuxA = 0b101;
    // else if (data_forwarding_A_MEM && simulator -> instructions[1].format != Format::S) MuxA = 0b101;
    // else if (data_forwarding_A_MEM && simulator -> instructions[1].format == Format::S) MuxA = 0b110;
    else if (simulator -> instructions[1].format == Format::R || simulator -> instructions[1].format == Format::I || simulator -> instructions[1].format == Format::S || simulator -> instructions[1].format == Format::SB) MuxA = 0b1; // Register Value
    else if (command == "auipc") MuxA = 0b10; // PC
    else if (command == "lui") MuxA = 0b11; // Interchange with immediate
    else MuxA = 0b0;
    
    if (data_forwarding_B_EX && simulator -> instructions[1].format != Format::S) MuxB = 0b101;
    else if (data_forwarding_B_EX && simulator -> instructions[1].format == Format::S) MuxB = 0b110;
    else if (data_forwarding_B_MEM && simulator -> instructions[1].format != Format::S) MuxB = 0b111;
    else if (data_forwarding_B_MEM && simulator -> instructions[1].format == Format::S) MuxB = 0b1000;
    else if (simulator -> instructions[1].format == Format::R || simulator -> instructions[1].format == Format::SB) MuxB = 0b1; // Register Value
    else if (command == "lui") MuxB = 0b100; // 12 ("Interchanged")
    else if (simulator -> instructions[1].format == Format::U || simulator -> instructions[1].format == Format::I) MuxB = 0b10; // Immediate
    else if (simulator -> instructions[1].format == Format::S) MuxB = 0b11; // Immediate in RB and Register Value in RM
    else MuxB = 0b0;
}

void ControlCircuit::UpdateExecuteSignals() {
    string command = simulator -> instructions[2].literal.substr(0, simulator -> instructions[2].literal.find(" "));
    
    if (command == "add" || command == "addi") ALU = 0b1;
    else if (command == "sub") ALU = 0b10;
    else if (command == "mul") ALU = 0b11;
    else if (command == "div") ALU = 0b100;
    else if (command == "rem") ALU = 0b101;
    else if (command == "xor" || command == "xori") ALU = 0b110;
    else if (command == "or" || command == "ori") ALU = 0b111;
    else if (command == "and" || command == "andi") ALU = 0b1000;
    else if (command == "sll" || command == "lui") ALU = 0b1001;
    else if (command == "srl") ALU = 0b1010;
    else if (command == "sra") ALU = 0b1011;
    else if (command == "slt") ALU = 0b1100;
    else if (command == "beq") ALU = 0b1101;
    else if (command == "bne") ALU = 0b1110;
    else if (command == "blt") ALU = 0b1111;
    else if (command == "bge") ALU = 0b10000;
    else if (command == "auipc") ALU = 0b10001;
    else if (command == "jalr") ALU = 0b10010;
    else if (simulator -> instructions[2].format == Format::I || simulator -> instructions[2].format == Format::S) ALU = 0b1;
    else ALU = 0b0;
}

void ControlCircuit::UpdateMemorySignals() {
    string command = simulator -> instructions[3].literal.substr(0, simulator -> instructions[3].literal.find(" "));

    if (command == "lb" || command == "lh" || command == "lw" || command == "ld" || simulator -> instructions[3].format == Format::S) MuxMA = 0b1;
    else MuxMA = 0b0;

    if (command == "lb") MuxMD = 0b1;
    else if (command == "lh") MuxMD = 0b10;
    else if (command == "lw") MuxMD = 0b11;
    else if (command == "sb") MuxMD = 0b100;
    else if (command == "sh") MuxMD = 0b101;
    else if (command == "sw") MuxMD = 0b110;
    else MuxMD = 0b0;

    Debug::log("Current instruction: " + simulator -> instructions[3].literal);

    if (simulator -> instructions[3].format == Format::R) { MuxY = 0b1; Debug::log("RZ prompted"); } // Select RZ
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld") { MuxY = 0b10; Debug::log("MDR prompted"); } // Select MDR
    else if (command == "jalr" || simulator -> instructions[3].format == Format::UJ) { MuxY = 0b11; Debug::log("Address prompted"); } // Select Return Address
    else if (simulator -> instructions[3].format == Format::I || simulator -> instructions[3].format == Format::U) { MuxY = 0b1; Debug::log("RZ prompted"); } // Select RZ
    else { MuxY = 0b0; Debug::log("Nothing coming up"); }

    Debug::log("MuxY: " + to_string(MuxY));
}

void ControlCircuit::UpdateWritebackSignals() {
    string command = simulator -> instructions[4].literal.substr(0, simulator -> instructions[4].literal.find(" "));
    
    if (simulator -> instructions[4].format == Format::R) EnableRegisterFile = 0b1; // Select RZ
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld") EnableRegisterFile = 0b10; // Select MDR
    else if (command == "jalr" || simulator -> instructions[4].format == Format::UJ) EnableRegisterFile = 0b11; // Select Return Address
    else if (simulator -> instructions[4].format == Format::I || simulator -> instructions[4].format == Format::U) EnableRegisterFile = 0b1; // Select RZ
    else EnableRegisterFile = 0b0;
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
// Debug::log("Command in Memory: \"" + command + "\", MuxPC: " + to_string(MuxPC));

// if (command == "jalr") MuxPC = 0b1; // Select RZ
// else MuxPC = 0b0; // Select INSTRUCTION_SIZE

// if (command == "jal") MuxINC = 0b1; // Select immediate
// else if (command == "beq" || command == "bne" || command == "blt" || command == "bge") MuxINC = MuxINC; // Already calculated
// else MuxINC = 0b0; // Select 4