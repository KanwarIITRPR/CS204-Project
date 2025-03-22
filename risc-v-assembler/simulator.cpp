#include <iostream>
#include "main.cpp"

#define TEXT_SEGMENT_END "END-OF-TEXT-SEGMENT"
#define DATA_SEGMENT_END "END-OF-DATA-SEGMENT"

enum Stage {
    FETCH,
    DECODE,
    EXECUTE,
    MEMORY_ACCESS,
    WRITEBACK
};

struct Instruction {
    string PC, machine_code, opcode, rs1, rs2, rd, immediate, funct3, funct7;
    Format format;
    Stage stage;
}; 

class Simulator {
    public:
        void Initialize();
        void Fetch();
        void Decode();
        void RegisterState();
        
        private:
        static const int REGISTER_COUNT = 32;
        uint32_t registers[REGISTER_COUNT];
        Instruction current_instruction;
        
        uint32_t RA = 0x00000000;
        uint32_t RB = 0x00000000;
        uint32_t RM = 0x00000000;
        uint32_t RY = 0x00000000;
        uint32_t RZ = 0x00000000;
        uint32_t MAR = 0x00000000;
        uint32_t MDR = 0x00000000;
        uint32_t IR = 0x00000000;
        uint32_t PC = 0x00000000;
        unsigned long long clock;

        void InitializeRegisters();
        Instruction NextInstruction();
};

void Simulator::Initialize() {
    InitializeInstructions();
    InitializeRegisters();
}

Instruction Simulator::NextInstruction() {
    getline(fin, instruction);
    if (Tokenize(false).empty()) { current_instruction.machine_code = "0x00000000"; return current_instruction; }

    if (tokens[0] == ".data" || tokens[0] == ".text") { return NextInstruction(); }
    if (stoul(tokens[0], nullptr, 16) >= DATA_ADDRESS) { return NextInstruction(); }

    current_instruction.stage = FETCH;
    current_instruction.PC = tokens[0];
    current_instruction.machine_code = tokens[1];
    current_instruction.format = instruction_format_mapping[tokens[2]];

    int opcode_split_index;
    for (opcode_split_index = 0; opcode_split_index < tokens.size(); opcode_split_index++) {
        if (tokens[opcode_split_index] == "#") break;
    }

    current_instruction.opcode = tokens[opcode_split_index + 1];
    current_instruction.funct3 = tokens[opcode_split_index + 3];
    current_instruction.funct7 = tokens[opcode_split_index + 5];
    current_instruction.rd = tokens[opcode_split_index + 7];
    current_instruction.rs1 = tokens[opcode_split_index + 9];
    current_instruction.rs2 = tokens[opcode_split_index + 11];
    current_instruction.immediate = tokens[opcode_split_index + 13];

    return current_instruction;
}

void Simulator::InitializeRegisters() {
    for (size_t i = 0; i < REGISTER_COUNT; i++) registers[i] = 0x00000000;
    // Picked up from venus, online RISC-V editor
    registers[2] = STACK_ADDRESS;
    registers[3] = 0x10000000;
    registers[10] = 0x00000001;
    registers[11] = STACK_ADDRESS;
}

void Simulator::Fetch() {
    NextInstruction();
    if (stoul(current_instruction.machine_code, nullptr, 16) == 0) return;

    IR = stoul(current_instruction.machine_code, nullptr, 16);
    PC += 4;

    cout << "Fetch Completed" << endl;
}

void Simulator::Decode() {
    if (stoul(current_instruction.machine_code, nullptr, 16) == 0) return;
    switch (current_instruction.format) {
        case R:
            RA = registers[stoi(current_instruction.rs1, nullptr, 2)];
            RB = registers[stoi(current_instruction.rs2, nullptr, 2)];
            break;
        case I:
            RA = registers[stoi(current_instruction.rs1, nullptr, 2)];
            RB = stoll(current_instruction.immediate, nullptr, 2);
            break;
        case S:
            RA = registers[stoi(current_instruction.rs1, nullptr, 2)];
            RB = stoll(current_instruction.immediate, nullptr, 2);
            break;
        case SB:
            RA = registers[stoi(current_instruction.rs1, nullptr, 2)];
            RB = registers[stoi(current_instruction.rs2, nullptr, 2)];
            break;
        case U:
            RA = (tokens[0] == "lui") ? registers[stoi(current_instruction.rs1, nullptr, 2)] << 12 : current_address;
            RB = stoll(current_instruction.immediate, nullptr, 2);
            break;
        case UJ:
            RA = current_address;
            RB = stoll(current_instruction.immediate, nullptr, 2);
            break;
        default:
            break;
    }

    cout << "Decode Completed" << endl;
}

void Simulator::RegisterState() {
    cout << "RA: " << RA << endl;
    cout << "RB: " << RB << endl;
    cout << "IR: " << IR << endl;
    cout << "PC: " << PC << endl;
    for (size_t i = 0; i < REGISTER_COUNT; i++) cout << "x" << i << ": " << registers[i] << endl;

}

int main(int argC, char** argV) {
    if (argC < 2) {
        cerr << "Usage: " << argV[0] << " <output.mc>" << endl;
        return 1;
    }

    fin.open(argV[1]);
    if (!fin.is_open()) {
        cerr << "Error: Unable to open files" << endl;
        return 1;
    }

    Simulator sim;
    sim.Initialize();
    sim.Fetch();
    sim.Decode();
    sim.RegisterState();
    cout << endl;
    sim.Fetch();
    sim.Decode();
    sim.RegisterState();

    fin.close();
}