#include "simulator.hpp"

bool IsNullInstruction(Instruction instruction) { return instruction.machine_code == NULL_INSTRUCTION.machine_code; }
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
    string machine_line;
    vector<string> tokens;
    bool text_mode = true;
    while (getline(fin, machine_line)) {
        tokens = assembler.lexer.Tokenize(machine_line);
        if (tokens.empty()) break;

        if (tokens[0] == ".data") { text_mode = false; continue; }
        else if (tokens[0] == ".text") { text_mode = true; continue; }

        if (text_mode) {
            if (tokens[0] == TEXT_END) continue;

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

    fin.clear();
    fin.seekg(0);
}

uint32_t PipelinedSimulator::GenerateMask(uint8_t length) { return (1 << length) - 1; }

void PipelinedSimulator::ExtractInstruction(string machine_code) {
    uint32_t bit_code = GetDecimalNumber(machine_code);

    Instruction current_instruction;
    current_instruction.machine_code = bit_code;
    current_instruction.opcode = bit_code & 0x0000007F;
    current_instruction.format = GetFormat(current_instruction.opcode);
    current_instruction.stage = Stage::QUEUED;

    bit_code = bit_code >> OPCODE_LENGTH;
    switch (current_instruction.format) {
        case Format::R:
            current_instruction.rd = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.funct3 = bit_code & GenerateMask(FUNCT3_LENGTH);
            bit_code = bit_code >> FUNCT3_LENGTH;
            current_instruction.rs1 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.rs2 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.funct7 = bit_code;
            break;
        case Format::I:
            current_instruction.rd = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.funct3 = bit_code & GenerateMask(FUNCT3_LENGTH);
            bit_code = bit_code >> FUNCT3_LENGTH;
            current_instruction.rs1 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.immediate = bit_code;
            break;
        case Format::S:
            uint8_t lower_immediate = bit_code & GenerateMask(S_LOWER_IMMEDIATE_LENGTH);
            bit_code = bit_code >> S_LOWER_IMMEDIATE_LENGTH;
            current_instruction.funct3 = bit_code & GenerateMask(FUNCT3_LENGTH);
            bit_code = bit_code >> FUNCT3_LENGTH;
            current_instruction.rs1 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.rs2 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            uint8_t upper_immediate = bit_code;
            current_instruction.immediate = (upper_immediate << S_LOWER_IMMEDIATE_LENGTH) + lower_immediate;
            break;
        case Format::SB:
            uint8_t lower_immediate = bit_code & GenerateMask(SB_LOWER_IMMEDIATE_LENGTH);
            bit_code = bit_code >> SB_LOWER_IMMEDIATE_LENGTH;
            current_instruction.funct3 = bit_code & GenerateMask(FUNCT3_LENGTH);
            bit_code = bit_code >> FUNCT3_LENGTH;
            current_instruction.rs1 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.rs2 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            uint8_t upper_immediate = bit_code;

            uint8_t immediate_12 = upper_immediate >> 6;
            uint8_t immediate_10_5 = upper_immediate & GenerateMask(6);
            uint8_t immediate_4_1 = lower_immediate >> 1;
            uint8_t immediate_11 = lower_immediate & GenerateMask(1);

            current_instruction.immediate = (immediate_12 << 12) | (immediate_11 << 11) | (immediate_10_5 << 5) | (immediate_4_1 << 1);
            break;
        case Format::U:
            current_instruction.rd = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.immediate = bit_code;
            break;
        case Format::UJ:
            current_instruction.rd = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            uint32_t immediate = bit_code;
            
            uint8_t immediate_20 = immediate >> 19;
            uint8_t immediate_10_1 = (immediate >> 9) & GenerateMask(9);
            uint8_t immediate_11 = (immediate >> 8) & GenerateMask(1);
            uint8_t immediate_19_12 = immediate & GenerateMask(8);

            current_instruction.immediate = (immediate_20 << 20) | (immediate_19_12 << 12) | (immediate_11 << 11) | (immediate_10_1 << 1);

            break;
        default:
            return;
    }
}

void PipelinedSimulator::RunInstruction(bool each_stage = true) {
    instructions[4].stage = Stage::COMMITTED;
    for (size_t i = PIPELINE_STAGES - 1; i > 0; i--) {
        instructions[i] = instructions[i - 1];
    }
    control.UpdateControlSignals();

    if (!IsNullInstruction(instructions[4])) {
        Writeback();
        instructions[4].stage = Stage::WRITEBACK;
    }
    
    if (!IsNullInstruction(instructions[3])) {
        MemoryAccess();
        instructions[3].stage = Stage::MEMORY_ACCESS;
    }
    
    if (!IsNullInstruction(instructions[2])) {
        Execute();
        instructions[2].stage = Stage::EXECUTE;
    }
    
    if (!IsNullInstruction(instructions[1])) {
        Decode();
        instructions[1].stage = Stage::DECODE;
    }
    
    if ((!finished && !IsNullInstruction(instructions[0])) || (!started && IsNullInstruction(instructions[0]))) {
        Fetch();
        instructions[0].stage = Stage::FETCH;
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
    // instructions[0].stage = control.current_instruction.stage = Stage::DECODE;
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
    // decode_instruction.stage = control.current_instruction.stage = Stage::EXECUTE;
    // control.UpdateExecuteSignals();
}

void PipelinedSimulator::Execute() {
    // Instruction execute_instruction = instructions[2];
    // if (reached_end && IsNullInstruction(execute_instruction)) return;

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
    // execute_instruction.stage = control.current_instruction.stage = Stage::MEMORY_ACCESS;
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
    // if (reached_end && IsNullInstruction(memory_instruction)) return;

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
    // memory_instruction.stage = control.current_instruction.stage = Stage::WRITEBACK;
    // control.UpdateWritebackSignals();
}

void PipelinedSimulator::Writeback() {
    Instruction writeback_instruction = instructions[4];
    // if (reached_end && IsNullInstruction(writeback_instruction)) return;

    if (control.EnableRegisterFile) {
        register_file[stoi(writeback_instruction.rd, nullptr, 2)] = buffer.RY;
    }

    // control.IncrementClock();
    // cout << "Writeback Completed" << endl;
    // writeback_instruction.stage = control.current_instruction.stage = Stage::COMMITTED;
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