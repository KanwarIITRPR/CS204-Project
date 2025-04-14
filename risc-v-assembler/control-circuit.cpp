#include "utils.cpp"
#include "main.cpp"
#include "simulator.hpp"

class ControlCircuit {
    public:
        void UpdateControlSignals();
        void IncrementClock() { clock += 1; }
        void UpdateDecodeSignals();
        void UpdateExecuteSignals();
        void UpdateMemorySignals();
        void UpdateWritebackSignals();

        uint32_t CyclesExecuted() { return clock; }

        uint8_t MuxA = 0;
        uint8_t MuxB = 0;
        uint8_t ALU = 0;
        uint8_t MuxZ = 0;
        uint8_t MuxY = 0;
        uint8_t MuxMA = 0;
        uint8_t MuxMD = 0;
        uint8_t DemuxMD = 0;
        uint8_t MuxINC = 0;
        uint8_t MuxPC = 0;
        uint8_t EnableRegisterFile = 1;

        PipelinedSimulator simulator;

    private:
        uint32_t clock = 0;
};

void ControlCircuit::UpdateControlSignals() {
    UpdateDecodeSignals();
    UpdateExecuteSignals();
    UpdateMemorySignals();
    UpdateWritebackSignals();
}

void ControlCircuit::UpdateDecodeSignals() {
    
    string command = simulator.instructions[1].literal.substr(0, simulator.instructions[1].literal.find(" "));
    if (simulator.instructions[1].format == R || simulator.instructions[1].format == I || simulator.instructions[1].format == S || simulator.instructions[1].format == SB) MuxA = 0b1; // Register Value
    else if (command == "auipc") MuxA = 0b10; // PC
    else if (command == "lui") MuxA = 0b11; // Interchange with immediate
    else MuxA = 0b0;
    
    if (simulator.instructions[1].format == R || simulator.instructions[1].format == SB) MuxB = 0b1; // Register Value
    else if (command == "lui") MuxB = 0b100; // 12 ("Interchanged")
    else if (simulator.instructions[1].format == U || simulator.instructions[1].format == I) MuxB = 0b10; // Immediate
    else if (simulator.instructions[1].format == S) MuxB = 0b11; // Immediate in RB and Register Value in RM
    else MuxB = 0b0;
}

void ControlCircuit::UpdateExecuteSignals() {
    string command = simulator.instructions[2].literal.substr(0, simulator.instructions[2].literal.find(" "));
    
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
    else if (simulator.instructions[2].format == I || simulator.instructions[2].format == S) ALU = 0b1;
    else ALU = 0b0;
}

void ControlCircuit::UpdateMemorySignals() {
    string command = simulator.instructions[3].literal.substr(0, simulator.instructions[3].literal.find(" "));

    if (simulator.instructions[3].stage == FETCH) MuxMA = 0b1;
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld" || simulator.instructions[3].format == S) MuxMA = 0b10;
    else MuxMA = 0b0;

    if (simulator.instructions[3].stage == FETCH || command == "lb" || command == "lh" || command == "lw" || command == "ld") MuxMD = 0b1;
    else if (simulator.instructions[3].format == S) MuxMD = 0b10;
    else MuxMD = 0b0;
    
    if (simulator.instructions[3].stage == FETCH) DemuxMD = 0b1;
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld") DemuxMD = 0b10;
    else DemuxMD = 0b0;

    if (command == "jalr") MuxPC = 0b1; // Select RZ
    else MuxPC = 0b0; // Select INSTRUCTION_SIZE
    
    if (command == "jal") MuxINC = 0b1; // Select immediate
    else if (command == "beq" || command == "bne" || command == "blt" || command == "bge") MuxINC = MuxINC; // Already calculated
    else MuxINC = 0b0; // Select 4

    if (simulator.instructions[3].format == R) MuxY = 0b1; // Select RZ
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld") MuxY = 0b10; // Select MDR
    else if (command == "jalr" || simulator.instructions[3].format == UJ) MuxY = 0b11; // Select Return Address
    else if (simulator.instructions[3].format == I || simulator.instructions[3].format == U) MuxY = 0b1; // Select RZ
    else MuxY = 0b0;
}

void ControlCircuit::UpdateWritebackSignals() {
    string command = simulator.instructions[4].literal.substr(0, simulator.instructions[4].literal.find(" "));
    
    if (simulator.instructions[4].format == R) EnableRegisterFile = 0b1; // Select RZ
    else if (command == "lb" || command == "lh" || command == "lw" || command == "ld") EnableRegisterFile = 0b10; // Select MDR
    else if (command == "jalr" || simulator.instructions[4].format == UJ) EnableRegisterFile = 0b11; // Select Return Address
    else if (simulator.instructions[4].format == I || simulator.instructions[4].format == U) EnableRegisterFile = 0b1; // Select RZ
    else MuxY = 0b0;
}