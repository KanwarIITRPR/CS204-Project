#include "components.hpp"

bool IsNullInstruction(Instruction instruction) { return instruction.machine_code == NULL_INSTRUCTION.machine_code; }

void PipelinedSimulator::Reset_x0() { register_file[0] = 0;}

// 
void PipelinedSimulator::Run(char** argV, bool each_stage = true) {
    while (!finished) RunInstruction(each_stage);
    cout << "Program Ran Successfully!" << endl;
}
// 
void PipelinedSimulator::Step(char** argV, bool each_stage = true) {
    while (!finished) {
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
            current_instruction.address = current_address;
            Debug::log("Extracted Instruction");
            memory.AddInstruction(current_address, current_instruction);
            Debug::log("Added Instruction to text map");

            if (current_instruction.format == Format::SB) btb.updateEntry(current_address, (int32_t) current_address + (int32_t) current_instruction.immediate, 0);
            else if (current_instruction.format == Format::UJ) {
                Debug::log("> Address: " + to_string((int32_t) current_address) + ", Immediate: " + to_string((int32_t) current_instruction.immediate) + ", Result: " + to_string((int32_t) current_address + (int32_t) current_instruction.immediate));
                btb.updateEntry(current_address, (int32_t) current_address + (int32_t) current_instruction.immediate, 1);
            }
        } else {
            int bytes_stored = (stored_code.length() / 2) - 1;
            memory.AddData(current_address, GetDecimalNumber(stored_code), bytes_stored);
            Debug::log("Added " + stored_code + " to data map");
        }
    }

    fin.clear();
    fin.seekg(0);
    Debug::log("Reset machine file as input");

    btb.printTable();
    // hdu.ExtractDataDependencies();

    // for (auto pair: hdu.data_dependency_map) Debug::log(to_string(pair.first) + ": (" + to_string(pair.second.first) + ", " + to_string(pair.second.second) + ")");
    memory.PrintDataMemory();
}

uint32_t PipelinedSimulator::GenerateMask(int length) { return (1 << length) - 1; }

void PipelinedSimulator::ShiftInstructionsStage() {
    if (!instructions[3].is_stalled) instructions[4] = instructions[3];
    else instructions[4] = NULL_INSTRUCTION;
    if (!instructions[2].is_stalled) instructions[3] = instructions[2];
    else instructions[3] = NULL_INSTRUCTION;
    if (!instructions[1].is_stalled) instructions[2] = instructions[1];
    else instructions[2] = NULL_INSTRUCTION;
    if (!instructions[0].is_stalled) instructions[1] = instructions[0];
    else instructions[1] = NULL_INSTRUCTION;

    // if (hdu.stall_index == -1) hdu.stall_index = 1;
}

Instruction PipelinedSimulator::ExtractInstruction(string machine_code) {
    uint32_t bit_code = GetDecimalNumber(machine_code);
    
    Instruction current_instruction;
    current_instruction.machine_code = bit_code;
    current_instruction.opcode = bit_code & 0x0000007F;
    current_instruction.format = GetFormat(current_instruction.opcode);
    current_instruction.stage = Stage::QUEUED;
    current_instruction.is_stalled = false;
    
    // Helper variables required to format immediate values of the Formats
    uint8_t lower_immediate, upper_immediate, immediate_12, immediate_10_5, immediate_4_1, immediate_11, immediate_20, immediate_19_12;
    uint16_t immediate_10_1;
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

            immediate_12 = (upper_immediate >> 6) & GenerateMask(1);
            immediate_10_5 = upper_immediate & GenerateMask(6);
            immediate_4_1 = (lower_immediate >> 1) & GenerateMask(4);
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
            
            immediate_20 = (current_instruction.immediate >> 19) & GenerateMask(1);
            immediate_10_1 = (current_instruction.immediate >> 9) & GenerateMask(10);
            immediate_11 = (current_instruction.immediate >> 8) & GenerateMask(1);
            immediate_19_12 = (current_instruction.immediate) & GenerateMask(8);
            
            immediate_shift = IMMEDIATE_STORAGE_LENGTH - immediate_bits.at(Format::UJ);
            current_instruction.immediate = (immediate_20 << 20) | (immediate_19_12 << 12) | (immediate_11 << 11) | (immediate_10_1 << 1);
            current_instruction.immediate = (int32_t)(current_instruction.immediate << immediate_shift) >> immediate_shift;
            break;
        default:
            return NULL_INSTRUCTION;
    }
    return current_instruction;
}

void PipelinedSimulator::RunInstruction(bool each_stage = true) {
    ShiftInstructionsStage();
    hdu.CalculateDataDependency();
    control.UpdateControlSignals();

    if (!IsNullInstruction(instructions[4]) && !instructions[4].is_stalled) {
        Writeback();
        instructions[4].stage = Stage::COMMITTED;
    }
    
    if (!IsNullInstruction(instructions[3]) && !instructions[3].is_stalled) {
        MemoryAccess();
        instructions[3].stage = Stage::WRITEBACK;
    }
    
    if (!IsNullInstruction(instructions[2]) && !instructions[2].is_stalled) {
        Execute();
        instructions[2].stage = Stage::MEMORY_ACCESS;
    }
    
    if (!IsNullInstruction(instructions[1]) && !instructions[1].is_stalled) {
        Decode();
        instructions[1].stage = Stage::EXECUTE;
    }
    
    if ((!finished && !instructions[0].is_stalled) || (!started && IsNullInstruction(instructions[0]))) {
        // instructions[0].stage = Stage::FETCH;
        Fetch();
        instructions[0].stage = Stage::DECODE;
        started = true;
    }
    iag.UpdateFlush();
    // else if (!instructions[0].is_stalled) finished = true;
    
    control.IncrementClock();
    if (hdu.cycles_to_stall) {
        hdu.cycles_to_stall -= 1;
        total_stalls += 1;
        stalls_data_hazards += 1;
        if (!hdu.cycles_to_stall) {
            for (size_t i = 0; i < PIPELINE_STAGES; i++) {
                if (!IsNullInstruction(instructions[i])) instructions[i].is_stalled = false;
            }
        }
    }

    if (printFetchedInstructionDetails) PrintInstructionInfo(instructions[0]);
    if (printInstructions) PrintInstructions();
    if (specified_instruction) PrintSpecifiedPipelineRegisters();
    else if (printPipelineRegisters) PrintPipelineRegisters();
    if (printRegisterFile) PrintRegisterFile();
    LogStats();

    if (hdu.next_cycle_stall) {
        instructions[0].is_stalled = true;
        hdu.cycles_to_stall = 1;
        hdu.next_cycle_stall = false;
    }

    if (hdu.IsNextDependent()) {
        if (!hasDataForwarding) {
            if (hdu.hasEXtoEXDependency()) {
                instructions[0].is_stalled = true;
                hdu.cycles_to_stall = 2;
            } else if (hdu.hasMEMtoEXDependency()) hdu.next_cycle_stall = true;
        } else if (IsLoadOperation(instructions[1].opcode) && hdu.hasEXtoEXDependency()) {
            instructions[0].is_stalled = true;
            hdu.cycles_to_stall = 1;
        }
    }
    
    UpdateBufferRegisters();

    finished = true;
    for (size_t i = 0; i < PIPELINE_STAGES; i++) {
        if (!IsNullInstruction(instructions[i])) finished = false;
    }
}

void PipelinedSimulator::Flush() {
    instructions[0] = NULL_INSTRUCTION;
    instructions[1] = NULL_INSTRUCTION;

    inter_stage.RA = buffer.RA = 0;
    inter_stage.RB = buffer.RB = 0;
    inter_stage.RM = buffer.RM = 0;

    IR = 0;
    Debug::log("Flushed");

    total_stalls += 2;
    stalls_control_hazards += 2;
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
void PipelinedSimulator::SetKnob8(bool set_value) { printFetchedInstructionDetails = set_value; }

void PipelinedSimulator::UpdateBufferRegisters() {
    iag.UpdateBuffer();

    memory.data_memory.MDR = buffer.RM;

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
    memory.instruction_memory.MAR = iag.buffer_PC;
    instructions[0] = memory.GetInstruction();
    IR = memory.instruction_memory.MDR;

    if (!IsNullInstruction(instructions[0])) {
        control.UpdateIAGSignals();
        iag.UpdatePC();
    }

    Reset_x0();
}

void PipelinedSimulator::Decode() {
    Instruction decode_instruction = instructions[1];

    switch (control.MuxA) {
        case 0b1:
            inter_stage.RA = register_file[decode_instruction.rs1];
            break;
        case 0b10:
            inter_stage.RA = decode_instruction.immediate;
            break;
        case 0b11:
            inter_stage.RA = iag.buffer_PC;
            break;
        case 0b100:
            inter_stage.RA = inter_stage.RZ;
            Debug::log("EX to EX Data Forward ~ RS1");
            break;
        case 0b101:
            inter_stage.RA = inter_stage.RY;
            Debug::log("MEM to EX Data Forward ~ RS1");
            break;
        // case 0b110:
        //     inter_stage.RM = inter_stage.RY;
        //     Debug::log("MEM to EX Data Forward ~ RS1 ~ Store");
        //     break;
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
        case 0b101:
            inter_stage.RB = inter_stage.RZ;
            Debug::log("EX to EX Data Forward ~ RS2");
            break;
        case 0b110:
            inter_stage.RB = decode_instruction.immediate;
            inter_stage.RM = inter_stage.RZ;
            Debug::log("EX to EX Data Forward ~ RS2 ~ Store");
            break;
        case 0b111:
            inter_stage.RB = inter_stage.RY;
            Debug::log("MEM to EX Data Forward ~ RS2");
            break;
        case 0b1000:
            inter_stage.RB = decode_instruction.immediate;
            inter_stage.RM = inter_stage.RY;
            Debug::log("MEM to EX Data Forward ~ RS2 ~ Store");
            break;
        default: break;
    }

    Reset_x0();
}

void PipelinedSimulator::Execute() {
    switch (control.ALU) {
        case 0b1:
            inter_stage.RZ = buffer.RA + buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b10:
            inter_stage.RZ = buffer.RA - buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b11:
            inter_stage.RZ = buffer.RA * buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b100:
            if (buffer.RB == 0) {
                error_stream << "Divide by zero isn't possible: " << instructions[2].literal << endl;
                return;
            }
            inter_stage.RZ = buffer.RA / buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b101:
            inter_stage.RZ = buffer.RA % buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b110:
            inter_stage.RZ = buffer.RA ^ buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b111:
            inter_stage.RZ = buffer.RA | buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b1000:
            inter_stage.RZ = buffer.RA & buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b1001:    
            inter_stage.RZ = buffer.RA << buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b1010:
            inter_stage.RZ = buffer.RA >> buffer.RB;
            ALU_instructions += 1;
            break;
        case 0b1011:
            inter_stage.RZ = arithmeticRightShift(buffer.RA, buffer.RB);
            ALU_instructions += 1;
            break;
        case 0b1100:
            inter_stage.RZ = ((int32_t) buffer.RA < (int32_t) buffer.RB) ? 1 : 0;
            ALU_instructions += 1;
            break;
        case 0b1101:
            actual_outcome = control.MuxINC = (int32_t) buffer.RA == (int32_t) buffer.RB;
            if (pht.isMisprediction(instructions[2].address, control.MuxINC)) { recently_flushed = true; mispredictions += 1; }
            pht.updatePrediction(instructions[2].address, control.MuxINC);
            Debug::log("Next prediction of " + instructions[2].literal + " will be " + to_string(pht.getPrediction(instructions[2].address)));
            ALU_instructions += 1;
            break;
        case 0b1110:
            actual_outcome = control.MuxINC = (int32_t) buffer.RA != (int32_t) buffer.RB;
            if (pht.isMisprediction(instructions[2].address, control.MuxINC)) { recently_flushed = true; mispredictions += 1; }
            pht.updatePrediction(instructions[2].address, control.MuxINC);
            Debug::log("Next prediction of " + instructions[2].literal + " will be " + to_string(pht.getPrediction(instructions[2].address)));
            ALU_instructions += 1;
            break;
        case 0b1111:
            actual_outcome = control.MuxINC = (int32_t) buffer.RA < (int32_t) buffer.RB;
            if (pht.isMisprediction(instructions[2].address, control.MuxINC)) { recently_flushed = true; mispredictions += 1; }
            pht.updatePrediction(instructions[2].address, control.MuxINC);
            Debug::log("Next prediction of " + instructions[2].literal + " will be " + to_string(pht.getPrediction(instructions[2].address)));
            ALU_instructions += 1;
            break;
        case 0b10000:
            actual_outcome = control.MuxINC = (int32_t) buffer.RA >= (int32_t) buffer.RB;
            if (pht.isMisprediction(instructions[2].address, control.MuxINC)) { recently_flushed = true; mispredictions += 1; }
            pht.updatePrediction(instructions[2].address, control.MuxINC);
            Debug::log("Next prediction of " + instructions[2].literal + " will be " + to_string(pht.getPrediction(instructions[2].address)));
            ALU_instructions += 1;
            break;
        case 0b10001:
            inter_stage.RZ = buffer.RA + (buffer.RB << (iag.INSTRUCTION_SIZE * BYTE_SIZE - immediate_bits.at(Format::U)));
            ALU_instructions += 1;
            break;
        case 0b10010:
            inter_stage.RZ = buffer.RA + buffer.RB;
            if (inter_stage.RZ != instructions[2].address + iag.INSTRUCTION_SIZE) {
                return_jump = true;
                recently_flushed = true;
            }
            ALU_instructions += 1;
            break;
        default: break;
    }

    Reset_x0();
}

void PipelinedSimulator::MemoryAccess() {
    Instruction memory_instruction = instructions[3];

    switch (control.MuxMA) {
        case 0b1:
            memory.data_memory.MAR = buffer.RZ;
            break;
        default: break;
    }

    switch (control.MuxMD) {
        case 0b1:
            memory.GetDataValue(1);
            data_transfer_instructions += 1;
            break;
        case 0b10:
            memory.GetDataValue(2);
            data_transfer_instructions += 1;
            break;
        case 0b11:
            memory.GetDataValue(4);
            data_transfer_instructions += 1;
            break;
        case 0b100:
            // memory.data_memory.MDR = buffer.RM;
            memory.StoreDataValue(1);
            memory.PrintDataMemory();
            data_transfer_instructions += 1;
            break;
        case 0b101:
            // memory.data_memory.MDR = buffer.RM;
            memory.StoreDataValue(2);
            memory.PrintDataMemory();
            data_transfer_instructions += 1;
            break;
        case 0b110:
            // memory.data_memory.MDR = buffer.RM;
            memory.StoreDataValue(4);
            memory.PrintDataMemory();
            data_transfer_instructions += 1;
            break;
        default: break;
    }

    // iag.UpdatePC();

    Debug::log("Control in MuxY: " + to_string(control.MuxY));
    switch (control.MuxY) {
        case 0b1:
            Debug::log("Standard Pass, RZ: " + to_string(buffer.RZ));
            inter_stage.RY = buffer.RZ;
            break;
        case 0b10:
            Debug::log("Retrieve from memory, RZ: " + to_string(buffer.RZ));
            inter_stage.RY = memory.data_memory.MDR;
            break;
        case 0b11:
            Debug::log("Next Address, RZ: " + to_string(buffer.RZ));
            inter_stage.RY = instructions[3].address + iag.INSTRUCTION_SIZE;
            break;
        default: break;
    }

    Reset_x0();
}

void PipelinedSimulator::Writeback() {
    Instruction writeback_instruction = instructions[4];

    if (control.EnableRegisterFile) register_file[writeback_instruction.rd] = buffer.RY;
    instructions_executed += 1;

    Reset_x0();
}

Instruction PipelinedSimulator::GetSpecifiedInstruction() {
    for (size_t i = 0; i < PIPELINE_STAGES; i++) {
        if (specified_instruction == GetInstructionNumber(instructions[i].address)) return instructions[i];
    }
    return NULL_INSTRUCTION;
}

string PipelinedSimulator::GetStageName(Stage stage) {
    switch (stage) {
        case Stage::QUEUED: return "Queued";
        case Stage::FETCH: return "Fetch";
        case Stage::DECODE: return "Decode";
        case Stage::EXECUTE: return "Execute";
        case Stage::MEMORY_ACCESS: return "Memory Access";
        case Stage::WRITEBACK: return "Writeback";
        case Stage::COMMITTED: return "Committed";
        default: return "";
    }
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
    cout << "Clock Cycles: " << dec << control.CyclesExecuted() << endl;

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

void PipelinedSimulator::PrintSpecifiedPipelineRegisters() {
    Instruction highlighted_instruction = GetSpecifiedInstruction();
    if (IsNullInstruction(highlighted_instruction)) return;

    cout << "\n---- Specified Pipeline Registers ----\n";
    cout << "Clock Cycles: " << dec << control.CyclesExecuted() << endl;
    cout << "Specified Instruction: " << highlighted_instruction.literal << endl;
    cout << "Current Stage: " << GetStageName(highlighted_instruction.stage) << endl;

    if (highlighted_instruction.stage == Stage::DECODE || highlighted_instruction.stage == Stage::EXECUTE || highlighted_instruction.stage == Stage::MEMORY_ACCESS || highlighted_instruction.stage == Stage::WRITEBACK) {
        cout << "\nInter-Stage Registers\n" << hex;
    }
    if (highlighted_instruction.stage == Stage::DECODE) {
        cout << "> RA: 0x" << setw(8) << setfill('0') << inter_stage.RA << endl;
        cout << "> RB: 0x" << setw(8) << setfill('0') << inter_stage.RB << endl;
    }
    if (highlighted_instruction.stage == Stage::MEMORY_ACCESS) cout << "> RM: 0x" << setw(8) << setfill('0') << inter_stage.RM << endl;
    if (highlighted_instruction.stage == Stage::EXECUTE) cout << "> RZ: 0x" << setw(8) << setfill('0') << inter_stage.RZ << endl;
    if (highlighted_instruction.stage == Stage::WRITEBACK) cout << "> RY: 0x" << setw(8) << setfill('0') << inter_stage.RY << endl;
    
    if (highlighted_instruction.stage == Stage::FETCH || highlighted_instruction.stage == Stage::MEMORY_ACCESS) {
        cout << "\nPMI Registers\n" << hex;
    }
    if (highlighted_instruction.stage == Stage::FETCH) {
        cout << "> Instruction MAR: 0x" << setw(8) << setfill('0') << memory.instruction_memory.MAR << endl;
        cout << "> Instruction MDR: 0x" << setw(8) << setfill('0') << memory.instruction_memory.MDR << endl;
    }
    if (highlighted_instruction.stage == Stage::MEMORY_ACCESS) {
        cout << "> Data MAR: 0x" << setw(8) << setfill('0') << memory.data_memory.MAR << endl;
        cout << "> Data MDR: 0x" << setw(8) << setfill('0') << memory.data_memory.MDR << endl;
    }
    
    if (highlighted_instruction.stage == Stage::FETCH) {
        cout << "\nIAG Registers\n" << hex;
        cout << "> IR: 0x" << setw(8) << setfill('0') << IR << endl;
        cout << "> PC: 0x" << setw(8) << setfill('0') << iag.PC << endl;
    }
    cout << "----------------------------------------" << endl;
}

void PipelinedSimulator::PrintInstructions() {
    cout << "\n------ Instructions ------" << endl;
    for (size_t i = 0; i < PIPELINE_STAGES; i++) cout << GetStageName((Stage)((int)Stage::FETCH + i))[0] << ": " << instructions[i].literal << endl;
    cout << "--------------------------" << endl;
}

void PipelinedSimulator::PrintInstructionInfo(Instruction instruction) {
    if (IsNullInstruction(instruction)) cout << "\n---- NULL Instruction ----" << endl;
    else cout << "\n---- " << instruction.literal << " ----" << endl;
    cout << "> Address: 0x" << setw(8) << setfill('0') << hex << instruction.address << endl;
    cout << "> Format: " << GetFormatName(instruction.format) << endl;
    cout << "> Stage: " << GetStageName(instruction.stage) << endl;
    cout << "> Is Stalled?: " << instruction.is_stalled << endl;

    cout << "\n> Machine Code: 0x" << setw(8) << setfill('0') << hex << instruction.machine_code << endl;
    cout << "> Opcode: 0b" << DecimalToBinary(instruction.opcode, OPCODE_LENGTH) << endl;
    cout << "> Funct3: 0b" << DecimalToBinary(instruction.funct3, FUNCT3_LENGTH) << endl;
    cout << "> Funct7: 0b" << DecimalToBinary(instruction.funct7, FUNCT7_LENGTH) << endl;
    cout << "> Rd: 0b" << DecimalToBinary(instruction.rd, REGISTER_LENGTH) << endl;
    cout << "> Rs1: 0b" << DecimalToBinary(instruction.rs1, REGISTER_LENGTH) << endl;
    cout << "> Rs2: 0b" << DecimalToBinary(instruction.rs2, REGISTER_LENGTH) << endl;
    if (instruction.format != Format::R) cout << "> Immediate: 0b" << DecimalToBinary(instruction.immediate, immediate_bits.at(instruction.format)) << endl;

    cout << "----------";
    if (IsNullInstruction(instruction)) cout << "----------------";
    else for (size_t i = 0; i < instruction.literal.length(); i++) cout << "-";
    cout << endl;
}

void PipelinedSimulator::LogStats() {
    stats_stream.open(stats_file);
    stats_stream << "Clock Cycles: " << control.CyclesExecuted() << "\n";
    stats_stream << "Instructions Executed: " << instructions_executed << "\n";
    stats_stream << "CPI: " << (double) control.CyclesExecuted() / instructions_executed << "\n";
    stats_stream << "Data Transfers: " << data_transfer_instructions << "\n";
    stats_stream << "ALU Instructions Executed: " << ALU_instructions << "\n";
    stats_stream << "Control Instructions Executed: " << control_instructions << "\n";
    stats_stream << "Stalls in the pipeline: " << total_stalls << "\n";
    stats_stream << "Data hazards: " << data_hazards << "\n";
    stats_stream << "Control hazards: " << control_hazards << "\n";
    stats_stream << "Branch Mispredictions: " << mispredictions << "\n";
    stats_stream << "Stalls due to data hazards: " << stalls_data_hazards << "\n";
    stats_stream << "Stalls due to control hazards: " << stalls_control_hazards << "\n";
    stats_stream.close();
}

bool Debug::debug = true;
int Debug::debug_count = 0;

int main(int argC, char** argV) {
    InitializeFileStreams();
    // if (argC < 3) {
    //     cerr << "Usage: " << argV[0] << " <input.asm> <output.mc>" << endl;
    //     return 1;
    // }

    // PipelinedSimulator sim(argV[1], argV[2]);
    int run_time = (argC > 1) ? GetDecimalNumber(argV[1]) : 2;
    if (argC > 2) (GetDecimalNumber(argV[2]) != 0) ? Debug::set(1) : Debug::set(0);
    PipelinedSimulator sim(std_input_file, std_output_file);
    sim.SetKnob3((argC > 3) ? GetDecimalNumber(argV[3]) : 1); // Register File
    sim.SetKnob4((argC > 4) ? GetDecimalNumber(argV[4]) : 1); // Pipeline Registers
    sim.SetKnob5((argC > 5) ? GetDecimalNumber(argV[5]) : 1); // Specific Instruction
    sim.SetKnob7((argC > 6) ? GetDecimalNumber(argV[6]) : 1); // Instrctions in Pipeline
    sim.SetKnob8((argC > 7) ? GetDecimalNumber(argV[7]) : 1); // Fetched Instruction Details

    if (argC > 8 && GetDecimalNumber(argV[8])) sim.Run(argV, false);
    else sim.Step(argV, false);
    // for (size_t i = 0; i < run_time; i++) {
    //     sim.RunInstruction();
    //     cout << "======================================================" << endl;
    // }

    sim.LogStats();

    sim.fin.close();
    CloseFileStreams();
}