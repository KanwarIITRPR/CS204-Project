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
    COMMITTED
};

struct Instruction {
    string machine_code, opcode, rs1, rs2, rd, immediate, funct3, funct7, literal;
    Format format;
    Stage stage;
};

struct InterStageRegisters {
    uint32_t RA, RB;
    uint32_t RM;
    uint32_t RY, RZ;
};

#define NULL_INSTRUCTION Instruction()
// class PMI {
//     public:
//         uint32_t GetValue(uint32_t location, int bytes);
//         void StoreValue(uint32_t location, uint32_t data, int bytes);

//     private:
//         map<uint32_t, uint8_t> data_map;
//         map<uint32_t, Instruction> text_map;
// };

// uint32_t PMI::GetValue(uint32_t location, int bytes = 0) {
//     if (location < DATA_ADDRESS) {
//         return stoul(text_map[location].machine_code, nullptr, 16);
//     } else {
//         uint32_t final_value = 0;
//         for (size_t i = 0; i < bytes; i++) {
//             final_value += data_map[location + i] << (8 * i);
//         }
//         return BinaryToDecimal(extendBits(DecimalToBinary(final_value, 8 * bytes)));
//     }
// }

// void PMI::StoreValue(uint32_t location, uint32_t data, int bytes = 0) {
//     for (size_t i = 0; i < bytes; i++) {
//         data_map[location + i] = data % 0x100;
//         data = data >> 8;
//     }
// }


class PipelinedSimulator {
    private:
        static const int REGISTER_COUNT = 32;
        uint32_t register_file[REGISTER_COUNT];
        static const int INSTRUCTION_SIZE = 4;
        static const int PIPELINE_STAGES = 5;
        string stage_name[PIPELINE_STAGES] = {"Fetch", "Decode", "Execute", "Memory Access", "Writeback"};

        map<uint32_t, uint8_t> data_map;
        map<uint32_t, Instruction> text_map;

        InterStageRegisters inter_stage;
        InterStageRegisters buffer;
        
        // uint32_t RA = 0x00000000;
        // uint32_t RB = 0x00000000;
        // uint32_t RM = 0x00000000;
        // uint32_t RY = 0x00000000;
        // uint32_t RZ = 0x00000000;
        uint32_t MAR = 0x00000000;
        uint32_t MDR = 0x00000000;
        uint32_t IR = 0x00000000;
        uint32_t PC = 0x00000000;
        uint32_t PC_temp = 0x00000000;

        bool reached_end = false;
        bool started = false;
        bool finished = false;

        void Fetch();
        void Decode();
        void Execute();
        void MemoryAccess();
        void Writeback();

        bool hasPipeline = true;
        bool hasDataForwarding = true;
        bool printRegisterFile = true;
        bool printBufferRegisters = true;
        int specified_instruction = 0; // 1-based indexing, i.e., 0 represents disabled / no instruction
        bool printPredictionDetails = true;
    
    public:
        void Run(char** argV, bool each_stage);
        void Step(char** argV, bool each_stage);
        void RunInstruction(bool each_stage);
        
        void InitializeRegisters();
        void InitialParse(string mc_file);
        void Reset_x0();

        Instruction GetInstructionFromMemory(uint32_t location);
        uint32_t GetValueFromMemory(uint32_t location, int bytes);
        void StoreValueInMemory(uint32_t location, uint32_t data, int bytes);

        void RegisterState();

        void SetKnob1(bool set_value);
        void SetKnob2(bool set_value);
        void SetKnob3(bool set_value);
        void SetKnob4(bool set_value);
        void SetKnob5(int instruction_index);
        void SetKnob6(bool set_value);

        ControlCircuit control;
        Instruction instructions[PIPELINE_STAGES]; // 0 - Fetch, ..., 4 - Writeback

        PipelinedSimulator(char** argV) {
            ConvertToMachineLanguage(argV[1], argV[2]);
            
            InitializeRegisters();
            InitialParse(argV[2]);
            for (size_t i = 0; i < PIPELINE_STAGES; i++) instructions[i] = NULL_INSTRUCTION;

            control = ControlCircuit();
            control.simulator = this;
        };

        // Test Instructions
        void InstructionsInProcess() {
            cout << "------ Instructions ------" << endl;
            for (size_t i = 0; i < PIPELINE_STAGES; i++) cout << stage_name[i][0] << ": " << instructions[i].literal << endl;
            cout << endl;
        }

        friend class ControlCircuit;
};

bool isNullInstruction(Instruction instruction) { return instruction.machine_code == NULL_INSTRUCTION.machine_code; }
void PipelinedSimulator::Reset_x0() { register_file[0] = 0;}

void PipelinedSimulator::Run(char** argV, bool each_stage = true) {
    while (!reached_end) RunInstruction(each_stage);
    cout << "Program Ran Successfully!" << endl;
}

void PipelinedSimulator::Step(char** argV, bool each_stage = true) {
    while (!reached_end) {
        cin.get();
        RunInstruction(each_stage);
    }
    cout << "Program Ran Successfully!" << endl;
}

void PipelinedSimulator::InitializeRegisters() {
    // Picked up from venus, online RISC-V editor
    for (size_t i = 0; i < REGISTER_COUNT; i++) register_file[i] = 0x00000000;
    register_file[2] = STACK_ADDRESS;
    register_file[3] = 0x10000000;
    register_file[10] = 0x00000001;
    register_file[11] = STACK_ADDRESS;
}

void PipelinedSimulator::InitialParse(string mc_file) {
    ifstream machine_file(mc_file);

    string machine_line;
    bool text_mode = true;
    while (getline(machine_file, machine_line)) {
        machine_line = trimString(machine_line);
        if (Tokenize(machine_line, false, false).empty()) break;
        if (tokens[0] == ".data") {
            text_mode = false;
            continue;
        } else if (tokens[0] == ".text") {
            text_mode = true;
            continue;
        }

        if (text_mode) {
            if (tokens[1] == TEXT_SEGMENT_END) continue;

            Instruction new_instruction;
            new_instruction.machine_code = tokens[1];
            new_instruction.format = instruction_format_mapping[tokens[2]];
            new_instruction.stage = QUEUED;

            int opcode_split_index;
            for (opcode_split_index = 2; opcode_split_index < tokens.size(); opcode_split_index++) {
                if (tokens[opcode_split_index] == "#") break;
                new_instruction.literal += tokens[opcode_split_index] + " ";
            }

            new_instruction.opcode = tokens[opcode_split_index + 1];
            new_instruction.funct3 = tokens[opcode_split_index + 3];
            new_instruction.funct7 = tokens[opcode_split_index + 5];
            new_instruction.rd = tokens[opcode_split_index + 7];
            new_instruction.rs1 = tokens[opcode_split_index + 9];
            new_instruction.rs2 = tokens[opcode_split_index + 11];
            new_instruction.immediate = tokens[opcode_split_index + 13];

            text_map[stoul(tokens[0], nullptr, 16)] = new_instruction;
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

void PipelinedSimulator::RunInstruction(bool each_stage = true) {
    instructions[4].stage = COMMITTED;
    for (size_t i = PIPELINE_STAGES - 1; i > 0; i--) {
        instructions[i] = instructions[i - 1];
    }
    control.UpdateControlSignals();

    if (!isNullInstruction(instructions[4])) {
        Writeback();
        instructions[4].stage = WRITEBACK;
    }
    
    if (!isNullInstruction(instructions[3])) {
        MemoryAccess();
        instructions[3].stage = MEMORY_ACCESS;
    }
    
    if (!isNullInstruction(instructions[2])) {
        Execute();
        instructions[2].stage = EXECUTE;
    }
    
    if (!isNullInstruction(instructions[1])) {
        Decode();
        instructions[1].stage = DECODE;
    }
    
    if ((!finished && !isNullInstruction(instructions[0])) || (!started && isNullInstruction(instructions[0]))) {
        Fetch();
        instructions[0].stage = FETCH;
        started = true;
    } else finished = true;
    
    control.IncrementClock();
}

void PipelinedSimulator::SetKnob1(bool set_value) { hasPipeline = set_value; }
void PipelinedSimulator::SetKnob2(bool set_value) { hasDataForwarding = set_value; }
void PipelinedSimulator::SetKnob3(bool set_value) { printRegisterFile = set_value; }
void PipelinedSimulator::SetKnob4(bool set_value) { printBufferRegisters = set_value; }
void PipelinedSimulator::SetKnob5(int instruction_index) { SetKnob4(true); specified_instruction = instruction_index; }
void PipelinedSimulator::SetKnob6(bool set_value) { hasPipeline = set_value; }

void PipelinedSimulator::Fetch() {
    // if (text_map.find(PC) == text_map.end()) { reached_end = true; return; }

    PC_temp = PC;
    instructions[0] = GetInstructionFromMemory(PC);
    IR = GetValueFromMemory(PC, INSTRUCTION_SIZE);
    PC += INSTRUCTION_SIZE;

    Reset_x0();
    
    // cout << "Fetch Completed" << endl;
    // instructions[0].stage = control.current_instruction.stage = DECODE;
}

void PipelinedSimulator::Decode() {
    Instruction decode_instruction = instructions[1];

    switch (control.MuxA) {
        case 0b1:
            inter_stage.RA = register_file[stoi(decode_instruction.rs1, nullptr, 2)];
            break;
        case 0b10:
            inter_stage.RA = PC_temp;
            break;
        case 0b11:
            inter_stage.RA = stoll(decode_instruction.immediate, nullptr, 2);
            break;
        default: break;
    }

    switch (control.MuxB) {
        case 0b1:
            inter_stage.RB = register_file[stoi(decode_instruction.rs2, nullptr, 2)];
            break;
        case 0b10:
            inter_stage.RB = BinaryToDecimal(extendBits(DecimalToBinary(stoll(decode_instruction.immediate, nullptr, 2), 12)));
            break;
        case 0b11:
            inter_stage.RB = stoll(decode_instruction.immediate, nullptr, 2);
            inter_stage.RM = register_file[stoi(decode_instruction.rs2, nullptr, 2)];
            break;
        case 0b100:
            inter_stage.RB = 12;
            break;
        default: break;
    }

    Reset_x0();

    // control.IncrementClock();
    // cout << "Decode Completed" << endl;
    // decode_instruction.stage = control.current_instruction.stage = EXECUTE;
    // control.UpdateExecuteSignals();
}

void PipelinedSimulator::Execute() {
    // Instruction execute_instruction = instructions[2];
    // if (reached_end && isNullInstruction(execute_instruction)) return;

    switch (control.ALU) {
        case 0b1:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA + buffer.RB))); break;
        case 0b10:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA - buffer.RB))); break;
        case 0b11:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA * buffer.RB))); break;
        case 0b100:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA / buffer.RB))); break;
        case 0b101:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA % buffer.RB))); break;
        case 0b110:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA ^ buffer.RB))); break;
        case 0b111:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA | buffer.RB))); break;
        case 0b1000:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA & buffer.RB))); break;
        case 0b1001:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA << buffer.RB))); break;
        case 0b1010:
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA >> buffer.RB))); break;
        case 0b1011:
            inter_stage.RZ = arithmeticRightShift(buffer.RA, buffer.RB); break;
        case 0b1100:
            inter_stage.RZ = ((int32_t) buffer.RA < (int32_t) buffer.RB) ? 1 : 0; break;
        case 0b1101:
            control.MuxINC = (int32_t) buffer.RA == (int32_t) buffer.RB; break;
        case 0b1110:
            control.MuxINC = (int32_t) buffer.RA != (int32_t) buffer.RB; break;
        case 0b1111:
            control.MuxINC = (int32_t) buffer.RA < (int32_t) buffer.RB; break;
        case 0b10000:
            control.MuxINC = (int32_t) buffer.RA >= (int32_t) buffer.RB; break;
        case 0b10001:
            inter_stage.RB = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RB << 12)));
            inter_stage.RZ = BinaryToDecimal(extendBits(DecimalToBinary(buffer.RA + buffer.RB)));
            break;
        default: break;
    }

    Reset_x0();

    // control.IncrementClock();
    // cout << "Execute Completed" << endl;
    // execute_instruction.stage = control.current_instruction.stage = MEMORY_ACCESS;
    // control.UpdateMemorySignals();
}

Instruction PipelinedSimulator::GetInstructionFromMemory(uint32_t location) {
    if (location >= DATA_ADDRESS) {
        cerr << "Exist doesn't exist in test memory!" << endl;
        return NULL_INSTRUCTION;
    }

    return text_map[location];
}

uint32_t PipelinedSimulator::GetValueFromMemory(uint32_t location, int bytes = 0) {
    if (location < DATA_ADDRESS) {
        if (text_map[location].machine_code == NULL_INSTRUCTION.machine_code) return 0;
        return stoul(text_map[location].machine_code, nullptr, 16);
    } else {
        uint32_t final_value = 0;
        for (size_t i = 0; i < bytes; i++) {
            final_value += data_map[location + i] << (8 * i);
        }
        return BinaryToDecimal(extendBits(DecimalToBinary(final_value, 8 * bytes)));
    }
}

void PipelinedSimulator::StoreValueInMemory(uint32_t location, uint32_t data, int bytes = 0) {
    for (size_t i = 0; i < bytes; i++) {
        data_map[location + i] = data % 0x100;
        data = data >> 8;
    }
}

void PipelinedSimulator::MemoryAccess() {
    Instruction memory_instruction = instructions[3];
    // if (reached_end && isNullInstruction(memory_instruction)) return;

    switch (control.MuxMA) {
        case 0b1:
            MAR = PC;
            break;
        case 0b10:
            MAR = buffer.RZ;
            break;
        default: break;
    }

    string command = memory_instruction.literal.substr(0, memory_instruction.literal.find(" "));
    switch (control.MuxMD) {
        case 0b1:
            MDR = GetValueFromMemory(MAR, bytes[command]);
            break;
        case 0b10:
            StoreValueInMemory(MAR, buffer.RM, bytes[command]);
            break;
        default: break;
    }

    switch (control.DemuxMD) {
        case 0b1:
            IR = MDR;
            break;
        case 0b10:
            break;
        default: break;
    }

    switch (control.MuxPC) {
        case 0b0:
            PC = PC;
            break;
        case 0b1:
            PC = buffer.RZ;
            break;
        default: break;
    }

    switch (control.MuxINC) {
        case 0b0:
            break;
        case 0b1:
            PC = PC_temp + BinaryToDecimal(memory_instruction.immediate);
            break;
        default: break;
    }

    switch (control.MuxY) {
        case 0b1:
            inter_stage.RY = buffer.RZ;
            break;
        case 0b10:
            inter_stage.RY = MDR;
            break;
        case 0b11:
            inter_stage.RY = PC_temp + INSTRUCTION_SIZE;
            break;
        default: break;
    }

    Reset_x0();

    // control.IncrementClock();
    // cout << "Memory Access Completed" << endl;
    // memory_instruction.stage = control.current_instruction.stage = WRITEBACK;
    // control.UpdateWritebackSignals();
}

void PipelinedSimulator::Writeback() {
    Instruction writeback_instruction = instructions[4];
    // if (reached_end && isNullInstruction(writeback_instruction)) return;

    if (control.EnableRegisterFile) {
        register_file[stoi(writeback_instruction.rd, nullptr, 2)] = buffer.RY;
    }

    // control.IncrementClock();
    // cout << "Writeback Completed" << endl;
    // writeback_instruction.stage = control.current_instruction.stage = COMMITTED;
    Reset_x0();
    // control.UpdateFetchSignals();
}

void PipelinedSimulator::RegisterState() {
    cout << "RA: " << hex << inter_stage.RA << endl;
    cout << "RB: " << inter_stage.RB << endl;
    cout << "RM: " << inter_stage.RM << endl;
    cout << "RY: " << inter_stage.RY << endl;
    cout << "RZ: " << inter_stage.RZ << endl;
    cout << "MAR: " << MAR << endl;
    cout << "MDR: " << MDR << endl;
    cout << "IR: " << IR << endl;
    cout << "PC: " << PC << endl;
    cout << "Clock Cycles: " << control.CyclesExecuted() << endl;
    for (size_t i = 0; i < REGISTER_COUNT; i++) cout << "x" << dec << i << ": " << hex << register_file[i] << endl;
    for (auto pair: data_map) cout << hex << pair.first << " " << (int) pair.second << endl;

}

int main(int argC, char** argV) {
    if (argC < 3) {
        cerr << "Usage: " << argV[0] << " <input.asm> <output.mc>" << endl;
        return 1;
    }

    PipelinedSimulator sim(argV);
    // sim.Run(argV, false);
    // sim.Step(argV, false);
    sim.RunInstruction();
    sim.InstructionsInProcess();
    sim.RunInstruction();
    sim.InstructionsInProcess();
    sim.RunInstruction();
    sim.InstructionsInProcess();
    sim.RunInstruction();
    sim.InstructionsInProcess();
    sim.RunInstruction();
    sim.InstructionsInProcess();
    sim.RunInstruction();
    sim.InstructionsInProcess();
    


    fin.close();
}