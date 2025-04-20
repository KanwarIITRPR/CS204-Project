#include "components.hpp"

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
    // Register File
    for (size_t i = 0; i < REGISTER_COUNT; i++) register_file[i] = 0x00000000;
    register_file[2] = STACK_ADDRESS;
    register_file[3] = 0x10000000;
    register_file[10] = 0x00000001;
    register_file[11] = STACK_ADDRESS;

    // Pipeline Registers and Buffers
    inter_stage.RA = buffer.RA = 0;
    inter_stage.RB = buffer.RB = 0;
    inter_stage.RM = buffer.RM = 0;
    inter_stage.RY = buffer.RY = 0;
    inter_stage.RZ = buffer.RZ = 0;

    // Memory Registers
    memory.instruction_memory.MAR = memory.data_memory.MAR = 0;
    memory.instruction_memory.MDR = memory.data_memory.MDR = 0;

    // IAG Register and Buffer
    iag.PC = iag.buffer_PC = 0;
}

void PipelinedSimulator::InitialParse() {
    string machine_line;
    vector<string> tokens;
    bool text_mode = true;
    while (getline(fin, machine_line)) {
        if (machine_line.empty()) { Debug::log("Machine Line Empty"); break; }
        else if (machine_line == ".data" || machine_line == TEXT_END) { text_mode = false; Debug::log(".data occurred"); continue; }
        else if (machine_line == ".text" || machine_line == DATA_END) { text_mode = true; Debug::log(".text occurred"); continue; }

        Debug::log("Machine line: \"" + machine_line + "\"");
        
        stringstream data(machine_line);
        string address_string, stored_code, command;
        data >> address_string >> stored_code;
        Debug::log("Address: \"" + address_string + "\", Stored value: \"" + stored_code + "\"");
        uint32_t current_address = GetDecimalNumber(address_string);

        getline(data, command);
        if (!command.empty()) {
            command = command.substr(1, command.length() - 1);
            Debug::log("Command: \"" + command + "\"");
        }

        if (text_mode) {
            Instruction current_instruction = ExtractInstruction(stored_code);
            current_instruction.literal = command;
            Debug::log("Extracted Instruction");
            memory.AddInstruction(current_address, current_instruction);
            Debug::log("Added Instruction to text map");
        } else {
            int bytes_stored = (stored_code.length() / 2) - 1;
            memory.AddData(current_address, GetDecimalNumber(stored_code), bytes_stored);
            Debug::log("Added Data to data map");
        }
    }

    fin.clear();
    fin.seekg(0);
    Debug::log("Reset machine file as input");
}

uint32_t PipelinedSimulator::GenerateMask(uint8_t length) { return (1 << length) - 1; }

Instruction PipelinedSimulator::ExtractInstruction(string machine_code) {
    uint32_t bit_code = GetDecimalNumber(machine_code);
    
    Instruction current_instruction;
    current_instruction.machine_code = bit_code;
    current_instruction.opcode = bit_code & 0x0000007F;
    current_instruction.format = GetFormat(current_instruction.opcode);
    current_instruction.stage = Stage::QUEUED;
    
    // Helper variables required to format immediate values of the Formats
    uint8_t lower_immediate, upper_immediate, immediate_12, immediate_10_5, immediate_4_1, immediate_11, immediate_20, immediate_10_1, immediate_19_12;
    const uint8_t IMMEDIATE_STORAGE_LENGTH = 32;
    uint32_t immediate_shift;

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

            immediate_shift = IMMEDIATE_STORAGE_LENGTH - immediate_bits.at(Format::I);
            current_instruction.immediate = (int32_t)(bit_code << immediate_shift) >> immediate_shift;
            break;
        case Format::S:
            lower_immediate = bit_code & GenerateMask(S_LOWER_IMMEDIATE_LENGTH);
            bit_code = bit_code >> S_LOWER_IMMEDIATE_LENGTH;
            current_instruction.funct3 = bit_code & GenerateMask(FUNCT3_LENGTH);
            bit_code = bit_code >> FUNCT3_LENGTH;
            current_instruction.rs1 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.rs2 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            upper_immediate = bit_code;

            immediate_shift = IMMEDIATE_STORAGE_LENGTH - immediate_bits.at(Format::S);
            current_instruction.immediate = (upper_immediate << S_LOWER_IMMEDIATE_LENGTH) + lower_immediate;
            current_instruction.immediate = (int32_t)(current_instruction.immediate << immediate_shift) >> immediate_shift;
            break;
        case Format::SB:
            lower_immediate = bit_code & GenerateMask(SB_LOWER_IMMEDIATE_LENGTH);
            bit_code = bit_code >> SB_LOWER_IMMEDIATE_LENGTH;
            current_instruction.funct3 = bit_code & GenerateMask(FUNCT3_LENGTH);
            bit_code = bit_code >> FUNCT3_LENGTH;
            current_instruction.rs1 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.rs2 = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            upper_immediate = bit_code;

            immediate_12 = upper_immediate >> 6;
            immediate_10_5 = upper_immediate & GenerateMask(6);
            immediate_4_1 = lower_immediate >> 1;
            immediate_11 = lower_immediate & GenerateMask(1);

            immediate_shift = IMMEDIATE_STORAGE_LENGTH - immediate_bits.at(Format::SB);
            current_instruction.immediate = (immediate_12 << 12) | (immediate_11 << 11) | (immediate_10_5 << 5) | (immediate_4_1 << 1);
            current_instruction.immediate = (int32_t)(current_instruction.immediate << immediate_shift) >> immediate_shift;
            break;
        case Format::U:
            current_instruction.rd = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            immediate_shift = IMMEDIATE_STORAGE_LENGTH - immediate_bits.at(Format::U);
            current_instruction.immediate = (int32_t)(bit_code << immediate_shift) >> immediate_shift;
            break;
        case Format::UJ:
            current_instruction.rd = bit_code & GenerateMask(REGISTER_LENGTH);
            bit_code = bit_code >> REGISTER_LENGTH;
            current_instruction.immediate = bit_code;
            
            immediate_20 = current_instruction.immediate >> 19;
            immediate_10_1 = (current_instruction.immediate >> 9) & GenerateMask(10);
            immediate_11 = (current_instruction.immediate >> 8) & GenerateMask(1);
            immediate_19_12 = current_instruction.immediate & GenerateMask(8);

            immediate_shift = IMMEDIATE_STORAGE_LENGTH - immediate_bits.at(Format::UJ);
            current_instruction.immediate = (immediate_20 << 20) | (immediate_19_12 << 12) | (immediate_11 << 11) | (immediate_10_1 << 1);
            current_instruction.immediate = (int32_t)(current_instruction.immediate << immediate_shift) >> immediate_shift;
            break;
        default:
            return NULL_INSTRUCTION;
    }
    return current_instruction;
}
// 
void PipelinedSimulator::RunInstruction(bool each_stage = true) {
    for (size_t i = PIPELINE_STAGES - 1; i > 0; i--) {
        instructions[i] = instructions[i - 1];
    }
    control.UpdateControlSignals();

    if (!IsNullInstruction(instructions[4])) {
        Writeback();
        instructions[4].stage = Stage::COMMITTED;
    }
    
    if (!IsNullInstruction(instructions[3])) {
        MemoryAccess();
        instructions[3].stage = Stage::WRITEBACK;
    }
    
    if (!IsNullInstruction(instructions[2])) {
        Execute();
        instructions[2].stage = Stage::MEMORY_ACCESS;
    }
    
    if (!IsNullInstruction(instructions[1])) {
        Decode();
        instructions[1].stage = Stage::EXECUTE;
    }
    
    if ((!finished && !IsNullInstruction(instructions[0])) || (!started && IsNullInstruction(instructions[0]))) {
        Fetch();
        instructions[0].stage = Stage::DECODE;
        started = true;
    } else finished = true;
    
    control.IncrementClock();
    if (printInstructions) PrintInstructions();
    if (specified_instruction) PrintPipelineRegisters();
    else if (printPipelineRegisters) PrintPipelineRegisters();
    if (printRegisterFile) PrintRegisterFile();
    
    UpdateBufferRegisters();
}

void PipelinedSimulator::SetKnob1(bool set_value) { hasPipeline = set_value; }
void PipelinedSimulator::SetKnob2(bool set_value) { hasDataForwarding = set_value; }
void PipelinedSimulator::SetKnob3(bool set_value) { printRegisterFile = set_value; }
void PipelinedSimulator::SetKnob4(bool set_value) { printPipelineRegisters = set_value; }
void PipelinedSimulator::SetKnob5(uint32_t instruction_index) {
    if (!printPipelineRegisters) return;
    if (!instruction_index) {
        if (previouslyPrintingPipelineRegisters) SetKnob4(true);
        else SetKnob4(false);
    } else previouslyPrintingPipelineRegisters = printPipelineRegisters;
    specified_instruction = instruction_index;
}
void PipelinedSimulator::SetKnob6(bool set_value) { hasPipeline = set_value; }
void PipelinedSimulator::SetKnob7(bool set_value) { printInstructions = set_value; }

void PipelinedSimulator::UpdateBufferRegisters() {
    iag.UpdateBuffer();

    buffer.RA = inter_stage.RA;
    buffer.RB = inter_stage.RB;
    buffer.RM = inter_stage.RM;
    buffer.RY = inter_stage.RY;
    buffer.RZ = inter_stage.RZ;
}

uint32_t PipelinedSimulator::GetInstructionNumber(uint32_t address) {
    if (address >= DATA_ADDRESS) {
        error_stream << "Instructions are not present in data memory: " << address << endl;
        return 0;
    }
    return (address / iag.INSTRUCTION_SIZE) + 1;
}

void PipelinedSimulator::Fetch() {
    memory.instruction_memory.MAR = iag.PC;
    instructions[0] = memory.GetInstruction();
    IR = memory.instruction_memory.MDR;
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

void PipelinedSimulator::PrintRegisterFile() {
    uint8_t columns = 4, rows = REGISTER_COUNT / columns;
    cout << "\n------------------------------ Register File ------------------------------\n";
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < columns; j++) {
            size_t current_register = i + 8 * j;
            cout << dec << setw(5) << setfill(' ') << "x" + to_string(current_register) + ": " << flush;
            cout << hex << "0x" << setw(8) << setfill('0') << register_file[current_register] << flush;
            if (j != columns - 1) cout << "  |  ";
            else cout << endl;
        }
    }
    cout << "---------------------------------------------------------------------------" << endl;
}

void PipelinedSimulator::PrintPipelineRegisters() {
    cout << "\n------- Pipeline Registers -------\n";
    cout << "Clock Cycles: " << control.CyclesExecuted() << endl;

    cout << "\nInter-Stage Registers\n" << hex;
    cout << "> RA: 0x" << setw(8) << setfill('0') << inter_stage.RA << endl;
    cout << "> RB: 0x" << setw(8) << setfill('0') << inter_stage.RB << endl;
    cout << "> RM: 0x" << setw(8) << setfill('0') << inter_stage.RM << endl;
    cout << "> RZ: 0x" << setw(8) << setfill('0') << inter_stage.RZ << endl;
    cout << "> RY: 0x" << setw(8) << setfill('0') << inter_stage.RY << endl;
    
    cout << "\nPMI Registers\n" << hex;
    cout << "> Instruction MAR: 0x" << setw(8) << setfill('0') << memory.instruction_memory.MAR << endl;
    cout << "> Instruction MDR: 0x" << setw(8) << setfill('0') << memory.instruction_memory.MDR << endl;
    cout << "> Data MAR: 0x" << setw(8) << setfill('0') << memory.data_memory.MAR << endl;
    cout << "> Data MDR: 0x" << setw(8) << setfill('0') << memory.data_memory.MDR << endl;
    
    cout << "\nIAG Registers\n" << hex;
    cout << "> IR: 0x" << setw(8) << setfill('0') << IR << endl;
    cout << "> PC: 0x" << setw(8) << setfill('0') << iag.PC << endl;
    cout << "----------------------------------" << endl;
}

void PipelinedSimulator::PrintInstructions() {
    cout << "\n------ Instructions ------" << endl;
    for (size_t i = 0; i < PIPELINE_STAGES; i++) cout << stage_name[i][0] << ": " << instructions[i].literal << endl;
    cout << "--------------------------" << endl;
}

bool Debug::debug = true;
int Debug::debug_count = 0;

int main(int argC, char** argV) {
    // if (argC < 3) {
    //     cerr << "Usage: " << argV[0] << " <input.asm> <output.mc>" << endl;
    //     return 1;
    // }

    // PipelinedSimulator sim(argV[1], argV[2]);
    int run_time = (argC > 1) ? GetDecimalNumber(argV[1]) : 2;
    if (argC > 2) (GetDecimalNumber(argV[2]) != 0) ? Debug::set(1) : Debug::set(0);
    PipelinedSimulator sim(std_input_file, std_output_file);

    // sim.Run(argV, false);
    // sim.Step(argV, false);
    for (size_t i = 0; i < run_time; i++) sim.RunInstruction();

    sim.fin.close();
}