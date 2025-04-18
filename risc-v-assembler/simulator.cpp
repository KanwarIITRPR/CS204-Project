#include "simulator.hpp"

bool IsNullInstruction(Instruction instruction) { return instruction.machine_code == NULL_INSTRUCTION.machine_code; }

void PipelinedSimulator::Reset_x0() { register_file[0] = 0;}

// 
void PipelinedSimulator::Run(char** argV, bool each_stage = true) {
    while (!reached_end) RunInstruction(each_stage);
    cout << "Program Ran Successfully!" << endl;
}
// 
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

void PipelinedSimulator::InitialParse() {
    string machine_line;
    vector<string> tokens;
    bool text_mode = true;
    while (getline(fin, machine_line)) {
        if (machine_line.empty()) break;
        else if (machine_line == ".data" || machine_line == TEXT_END) { text_mode = false; continue; }
        else if (machine_line == ".text" || machine_line == DATA_END) { text_mode = true; continue; }
        
        stringstream data(machine_line);
        string address_string, stored_code, command;
        data >> address_string >> stored_code;
        uint32_t current_address = GetDecimalNumber(address_string);
        getline(data, command);
        command = command.substr(1, command.length() - 1);

        if (text_mode) {
            Instruction current_instruction = ExtractInstruction(stored_code);
            current_instruction.literal = command;
            memory.AddInstruction(current_address, current_instruction);
        } else {
            int bytes_stored = (stored_code.length() / 2) - 1;
            memory.AddData(current_address, GetDecimalNumber(stored_code), bytes_stored);
        }
    }

    fin.clear();
    fin.seekg(0);
}

uint32_t PipelinedSimulator::GenerateMask(uint8_t length) { return (1 << length) - 1; }

Instruction PipelinedSimulator::ExtractInstruction(string machine_code) {
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
            uint8_t immediate_10_1 = (immediate >> 9) & GenerateMask(10);
            uint8_t immediate_11 = (immediate >> 8) & GenerateMask(1);
            uint8_t immediate_19_12 = immediate & GenerateMask(8);

            current_instruction.immediate = (immediate_20 << 20) | (immediate_19_12 << 12) | (immediate_11 << 11) | (immediate_10_1 << 1);
            break;
        default:
            return NULL_INSTRUCTION;
    }
    return current_instruction;
}
// 
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
    iag.UpdateBuffer();

    memory.instruction_memory.MAR = iag.PC;
    instructions[0] = memory.GetInstruction();
    IR = instructions[0].machine_code;
    iag.UpdatePC();

    Reset_x0();
}

// 
void PipelinedSimulator::Decode() {
    Instruction decode_instruction = instructions[1];

    switch (control.MuxA) {
        case 0b1:
            inter_stage.RA = register_file[decode_instruction.rs1];
            break;
        case 0b10:
            inter_stage.RA = iag.buffer_PC;
            break;
        case 0b11:
            inter_stage.RA = decode_instruction.immediate;
            break;
        default: break;
    }

    switch (control.MuxB) {
        case 0b1:
            inter_stage.RB = register_file[decode_instruction.rs2];
            break;
        case 0b10:
            inter_stage.RB = decode_instruction.immediate;
            break;
        case 0b11:
            inter_stage.RB = decode_instruction.immediate;
            inter_stage.RM = register_file[decode_instruction.rs2];
            break;
        case 0b100:
            inter_stage.RB = iag.INSTRUCTION_SIZE * BYTE_SIZE - immediate_bits.at(Format::U);
            break;
        default: break;
    }

    Reset_x0();
}

// 
void PipelinedSimulator::Execute() {
    switch (control.ALU) {
        case 0b1:
            inter_stage.RZ = buffer.RA + buffer.RB; break;
        case 0b10:
            inter_stage.RZ = buffer.RA - buffer.RB; break;
        case 0b11:
            inter_stage.RZ = buffer.RA * buffer.RB; break;
        case 0b100:
            inter_stage.RZ = buffer.RA / buffer.RB; break;
        case 0b101:
            inter_stage.RZ = buffer.RA % buffer.RB; break;
        case 0b110:
            inter_stage.RZ = buffer.RA ^ buffer.RB; break;
        case 0b111:
            inter_stage.RZ = buffer.RA | buffer.RB; break;
        case 0b1000:
            inter_stage.RZ = buffer.RA & buffer.RB; break;
        case 0b1001:    
            inter_stage.RZ = buffer.RA << buffer.RB; break;
        case 0b1010:
            inter_stage.RZ = buffer.RA >> buffer.RB; break;
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
            inter_stage.RZ = buffer.RA + (buffer.RB << (iag.INSTRUCTION_SIZE * BYTE_SIZE - immediate_bits.at(Format::U)));
            break;
        default: break;
    }

    Reset_x0();
}

// 
void PipelinedSimulator::MemoryAccess() {
    Instruction memory_instruction = instructions[3];

    switch (control.MuxMA) {
        case 0b1:
            memory.instruction_memory.MAR = iag.PC;
            break;
        case 0b10:
            memory.data_memory.MAR = buffer.RZ;
            break;
        default: break;
    }

    // what do for IR?
    string command = memory_instruction.literal.substr(0, memory_instruction.literal.find(" "));
    switch (control.MuxMD) {
        case 0b1:
        // Separate it for all load instructions
        memory.GetDataValue();
        break;
        case 0b10:
        // Separate it for all store instructions
            memory.data_memory.MDR = buffer.RM;
            memory.StoreDataValue();
            break;
        default: break;
    }

    switch (control.DemuxMD) {
        case 0b1:
            IR = memory.instruction_memory.MDR;
            break;
        case 0b10:
            break;
        default: break;
    }

    switch (control.MuxPC) {
        case 0b0:
            // PC = PC;
            break;
        case 0b1:
            iag.PC = buffer.RZ;
            break;
        default: break;
    }

    switch (control.MuxINC) {
        case 0b0:
            break;
        case 0b1:
            iag.PC = iag.buffer_PC + memory_instruction.immediate;
            break;
        default: break;
    }

    switch (control.MuxY) {
        case 0b1:
            inter_stage.RY = buffer.RZ;
            break;
        case 0b10:
            inter_stage.RY = memory.data_memory.MDR;
            break;
        case 0b11:
            inter_stage.RY = iag.buffer_PC + iag.INSTRUCTION_SIZE;
            break;
        default: break;
    }

    Reset_x0();
}

void PipelinedSimulator::Writeback() {
    Instruction writeback_instruction = instructions[4];

    if (control.EnableRegisterFile) register_file[writeback_instruction.rd] = buffer.RY;

    Reset_x0();
}

void PipelinedSimulator::RegisterState() {
    cout << "RA: " << hex << inter_stage.RA << endl;
    cout << "RB: " << inter_stage.RB << endl;
    cout << "RM: " << inter_stage.RM << endl;
    cout << "RY: " << inter_stage.RY << endl;
    cout << "RZ: " << inter_stage.RZ << endl;
    cout << "Instruction MAR: " << memory.instruction_memory.MAR << endl;
    cout << "Instruction MDR: " << memory.instruction_memory.MDR << endl;
    cout << "Data MAR: " << memory.data_memory.MAR << endl;
    cout << "Data MDR: " << memory.data_memory.MDR << endl;
    cout << "IR: " << IR << endl;
    cout << "PC: " << iag.PC << endl;
    cout << "Clock Cycles: " << control.CyclesExecuted() << endl;
    for (size_t i = 0; i < REGISTER_COUNT; i++) cout << "x" << dec << i << ": " << hex << register_file[i] << endl;
    for (auto pair: memory.data_map) cout << hex << pair.first << " " << (int) pair.second << endl;

}

int main(int argC, char** argV) {
    if (argC < 3) {
        cerr << "Usage: " << argV[0] << " <input.asm> <output.mc>" << endl;
        return 1;
    }

    PipelinedSimulator sim(argV[1], argV[2]);
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

    sim.fin.close();
}