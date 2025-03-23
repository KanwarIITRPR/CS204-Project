#include <iostream>
#include "main.cpp"

#define TEXT_SEGMENT_END "END-OF-TEXT-SEGMENT"
#define DATA_SEGMENT_END "END-OF-DATA-SEGMENT"

enum Stage {
    QUEUED,
    FETCH,
    DECODE,
    EXECUTE,
    MEMORY_ACCESS,
    WRITEBACK,
    FINISHED
};

struct Instruction {
    string machine_code, opcode, rs1, rs2, rd, immediate, funct3, funct7, literal;
    Format format;
    Stage stage;
};

class ControlCircuit {
    public:
        void UpdateControlSignals();
        void IncrementClock() { clock += 1; }
        void UpdateDecodeSignals();
        void UpdateExecuteSignals();
        void UpdateMemorySignals();

        Instruction current_instruction;
        uint8_t MuxA = 0;
        uint8_t MuxB = 0;
        uint8_t ALU = 0;
        uint8_t MuxZ = 0;
        uint8_t MuxY = 0;
        uint8_t MuxMA = 0;
        uint8_t DemuxMD = 0;
        uint8_t MuxInc = 0;
        uint8_t MuxPC = 0;

        uint32_t clock = 0;
    private:
};

void ControlCircuit::UpdateControlSignals() {
    UpdateDecodeSignals();
    UpdateExecuteSignals();
    UpdateMemorySignals();
}

void ControlCircuit::UpdateDecodeSignals() {
    string command = current_instruction.literal.substr(0, current_instruction.literal.find(" "));
    if (current_instruction.format == R || current_instruction.format == I || current_instruction.format == S || current_instruction.format == SB) MuxA = 0b1; // Register Value
    else if (command == "auipc") MuxA = 0b10; // PC
    else if (command == "lui") MuxA = 0b11; // Interchange with immediate
    else MuxA = 0b0;
    
    if (current_instruction.format == R || current_instruction.format == SB) MuxB = 0b1; // Register Value
    else if (command == "lui") MuxB = 0b100; // 12 ("Interchanged")
    else if (current_instruction.format == U || current_instruction.format == I) MuxB = 0b10; // Immediate
    else if (current_instruction.format == S) MuxB = 0b11; // Immediate in RB and Register Value in RM
    else MuxB = 0b0;
}

void ControlCircuit::UpdateExecuteSignals() {
    string command = current_instruction.literal.substr(0, current_instruction.literal.find(" "));
    
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
    else if (current_instruction.format == I || current_instruction.format == S || current_instruction.format == U) ALU = 0b1;
    else ALU = 0b0;
}

void ControlCircuit::UpdateMemorySignals() {
    string command = current_instruction.literal.substr(0, current_instruction.literal.find(" "));

    if (command == "lb" || command == "lh" || command == "lw" || command == "ld" || current_instruction.format == S) MuxMA = 0b1;
    else MuxMA = 0b0;
    
    if (command == "lb" || command == "lh" || command == "lw" || command == "ld") DemuxMD = 0b1;
    else DemuxMD = 0b0;
}

class Simulator {
    public:
        void Initialize(string mc_file);
        void Fetch();
        void Decode();
        void Execute();
        void MemoryAccess();
        void RegisterState();
        void RunInstruction();
        void InitialParse(string mc_file);
        void InitializeRegisters();

        ControlCircuit control;
        
    private:
        static const int REGISTER_COUNT = 32;
        uint32_t registers[REGISTER_COUNT];
        static const int INSTRUCTION_SIZE = 4;
        Instruction current_instruction;

        map<uint32_t, uint8_t> data_map;
        map<uint32_t, Instruction> text_map;
        
        uint32_t RA = 0x00000000;
        uint32_t RB = 0x00000000;
        uint32_t RM = 0x00000000;
        uint32_t RY = 0x00000000;
        uint32_t RZ = 0x00000000;
        uint32_t MAR = 0x00000000;
        uint32_t MDR = 0x00000000;
        uint32_t IR = 0x00000000;
        uint32_t PC = 0x00000000;

};

void Simulator::Initialize(string mc_file) {
    InitializeInstructions();
    InitializeRegisters();
    InitialParse(mc_file);
    control = ControlCircuit();
}

void Simulator::InitialParse(string mc_file) {
    ifstream machine_file(mc_file);

    string machine_line;
    bool text_mode = true;
    while (getline(machine_file, machine_line)) {
        if (Tokenize(false).empty()) break;
        if (tokens[0] == ".data") {
            text_mode = false;
            continue;
        } else if (tokens[0] == ".text") {
            text_mode = true;
            continue;
        }

        if (text_mode) {
            if (tokens[1] == TEXT_SEGMENT_END) continue;

            Instruction curr_instruction;
            curr_instruction.machine_code = tokens[1];
            curr_instruction.format = instruction_format_mapping[tokens[2]];
            curr_instruction.stage = FETCH;

            int opcode_split_index;
            for (opcode_split_index = 2; opcode_split_index < tokens.size(); opcode_split_index++) {
                if (tokens[opcode_split_index] == "#") break;
                curr_instruction.literal += tokens[opcode_split_index] + " ";
            }

            current_instruction.opcode = tokens[opcode_split_index + 1];
            current_instruction.funct3 = tokens[opcode_split_index + 3];
            current_instruction.funct7 = tokens[opcode_split_index + 5];
            current_instruction.rd = tokens[opcode_split_index + 7];
            current_instruction.rs1 = tokens[opcode_split_index + 9];
            current_instruction.rs2 = tokens[opcode_split_index + 11];
            current_instruction.immediate = tokens[opcode_split_index + 13];

            text_map[stoul(tokens[1], nullptr, 16)] = curr_instruction;
        } else {
            if (tokens[1] == DATA_SEGMENT_END) continue;

            int bytes_stored = (tokens[1].length() / 2) - 1;
            uint32_t initial_storage = stoul(tokens[0], nullptr, 16);
            for (size_t i = 0; i < bytes_stored; i++) {
                data_map[initial_storage + i] = stoi(tokens[1].substr(tokens[1].length() - 2 * (i + 1), 2), nullptr, 16);
            }
        }
    }

    machine_file.close();
}

void Simulator::RunInstruction() {
    Fetch();
    Decode();
    Execute();
    MemoryAccess();
    RegisterState();
    cout << endl;
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
    if (text_map.find(PC) == text_map.end()) {
        current_instruction = Instruction();
        IR = 0;
        return;
    }
    
    current_instruction = text_map[PC];
    IR = stoul(current_instruction.machine_code, nullptr, 16);
    PC += INSTRUCTION_SIZE;
    
    control.current_instruction = current_instruction;
    control.UpdateControlSignals();
    control.IncrementClock();
    cout << "Fetch Completed" << endl;
}

void Simulator::Decode() {
    if (!IR) return;

    switch (control.MuxA) {
        case 0b1:
            RA = registers[stoi(current_instruction.rs1, nullptr, 2)];
            break;
        case 0b10:
            RA = PC;
            break;
        case 0b11:
            RA = stoll(current_instruction.immediate, nullptr, 16);
            break;
        default: break;
    }

    switch (control.MuxB) {
        case 0b1:
            RB = registers[stoi(current_instruction.rs2, nullptr, 2)];
            break;
        case 0b10:
            RB = stoll(current_instruction.immediate, nullptr, 16);
            break;
        case 0b11:
            RB = stoll(current_instruction.immediate, nullptr, 16);
            RM = registers[stoi(current_instruction.rs2, nullptr, 2)];
            break;
        case 0b100:
            RB = 12;
            break;
        default: break;
    }

    control.IncrementClock();
    cout << "Decode Completed" << endl;
}

void Simulator::Execute() {
    if (!IR) return;

    switch (control.ALU) {
        case 0b1:
            RZ = RA + RB; break;
        case 0b10:
            RZ = RA - RB; break;
        case 0b11:
            RZ = RA * RB; break;
        case 0b100:
            RZ = RA / RB; break;
        case 0b101:
            RZ = RA % RB; break;
        case 0b110:
            RZ = RA ^ RB; break;
        case 0b111:
            RZ = RA | RB; break;
        case 0b1000:
            RZ = RA & RB; break;
        case 0b1001:
            RZ = RA << RB; break;
        case 0b1010:
            RZ = RA >> RB; break;
        case 0b1011:
            RZ = arithmeticRightShift(RA, RB); break;
        case 0b1100:
            RZ = (RA < RB) ? 1 : 0; break;
        case 0b1101:
            RZ = RA == RB; break;
        case 0b1110:
            RZ = RA != RB; break;
        case 0b1111:
            RZ = RA < RB; break;
        case 0b10000:
            RZ = RA >= RB; break;
        default: break;
    }

    control.IncrementClock();
    cout << "Decode Completed" << endl;
}

void Simulator::MemoryAccess() {
    if (!IR) return;


}

void Simulator::RegisterState() {
    cout << "RA: " << RA << endl;
    cout << "RB: " << RB << endl;
    cout << "IR: " << IR << endl;
    cout << "PC: " << PC << endl;
    for (size_t i = 0; i < REGISTER_COUNT; i++) cout << "x" << i << ": " << registers[i] << endl;

}

int main(int argC, char** argV) {
    if (argC < 3) {
        cerr << "Usage: " << argV[0] << " <input.asm> <output.mc>" << endl;
        return 1;
    }

    ConvertToMachineLanguage(argV[1], argV[2]);

    Simulator sim;
    sim.Initialize(argV[2]);
    sim.RunInstruction();
    sim.RunInstruction();
    sim.RunInstruction();
    sim.RunInstruction();
    sim.RunInstruction();
    sim.RunInstruction();
    sim.RunInstruction();
    sim.RunInstruction();

    fin.close();
}