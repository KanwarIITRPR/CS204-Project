#include "components.hpp"

void ControlCircuit::UpdateControlSignals() {
    UpdateDecodeSignals();
    UpdateExecuteSignals();
    UpdateMemorySignals();
    UpdateWritebackSignals();
}

void ControlCircuit::UpdateDecodeSignals() {
    string command = simulator -> instructions[1].literal.substr(0, simulator -> instructions[1].literal.find(" "));
    if (simulator -> instructions[1].format == Format::R || simulator -> instructions[1].format == Format::I || simulator -> instructions[1].format == Format::S || simulator -> instructions[1].format == Format::SB) MuxA = 0b1; // Register Value
    else if (command == "auipc") MuxA = 0b10; // PC
    else if (command == "lui") MuxA = 0b11; // Interchange with immediate
    else MuxA = 0b0;
    
    if (simulator -> instructions[1].format == Format::R || simulator -> instructions[1].format == Format::SB) MuxB = 0b1; // Register Value
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
    else if (simulator -> instructions[2].format == Format::I || simulator -> instructions[2].format == Format::S) ALU = 0b1;
    else ALU = 0b0;
}

void ControlCircuit::UpdateMemorySignals() {
    string command = simulator -> instructions[3].literal.substr(0, simulator -> instructions[3].literal.find(" "));

    if (simulator -> instructions[3].stage == Stage::FETCH) MuxMA = 0b1;
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld" || simulator -> instructions[3].format == Format::S) MuxMA = 0b10;
    else MuxMA = 0b0;

    if (simulator -> instructions[3].stage == Stage::FETCH || command == "lb" || command == "lh" || command == "lw" || command == "ld") MuxMD = 0b1;
    else if (simulator -> instructions[3].format == Format::S) MuxMD = 0b10;
    else MuxMD = 0b0;
    
    if (simulator -> instructions[3].stage == Stage::FETCH) DemuxMD = 0b1;
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld") DemuxMD = 0b10;
    else DemuxMD = 0b0;

    if (command == "jalr") MuxPC = 0b1; // Select RZ
    else MuxPC = 0b0; // Select INSTRUCTION_SIZE
    
    if (command == "jal") MuxINC = 0b1; // Select immediate
    else if (command == "beq" || command == "bne" || command == "blt" || command == "bge") MuxINC = MuxINC; // Already calculated
    else MuxINC = 0b0; // Select 4

    if (simulator -> instructions[3].format == Format::R) MuxY = 0b1; // Select RZ
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld") MuxY = 0b10; // Select MDR
    else if (command == "jalr" || simulator -> instructions[3].format == Format::UJ) MuxY = 0b11; // Select Return Address
    else if (simulator -> instructions[3].format == Format::I || simulator -> instructions[3].format == Format::U) MuxY = 0b1; // Select RZ
    else MuxY = 0b0;
}

void ControlCircuit::UpdateWritebackSignals() {
    string command = simulator -> instructions[4].literal.substr(0, simulator -> instructions[4].literal.find(" "));
    
    if (simulator -> instructions[4].format == Format::R) EnableRegisterFile = 0b1; // Select RZ
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld") EnableRegisterFile = 0b10; // Select MDR
    else if (command == "jalr" || simulator -> instructions[4].format == Format::UJ) EnableRegisterFile = 0b11; // Select Return Address
    else if (simulator -> instructions[4].format == Format::I || simulator -> instructions[4].format == Format::U) EnableRegisterFile = 0b1; // Select RZ
    else MuxY = 0b0;
}