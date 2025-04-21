#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "assembler.hpp"

enum class Stage {
    QUEUED,
    FETCH,
    DECODE,
    EXECUTE,
    MEMORY_ACCESS,
    WRITEBACK,
    COMMITTED
};

struct Instruction {
    uint32_t machine_code, immediate, address;
    uint8_t opcode, rd, rs1, rs2, funct3, funct7;
    string literal;
    Format format;
    Stage stage;
    bool is_stalled;
};

#define NULL_INSTRUCTION Instruction()
bool IsNullInstruction(Instruction instruction);

struct InterStageRegisters {
    uint32_t RA, RB;
    uint32_t RM;
    uint32_t RY, RZ;
};

class Debug {
    public:
        static int debug_count;
        static bool debug;
        static void log(string message) {
            if (!debug) return;
            debug_count++;
            cout << dec << "Debug " << debug_count << " ~ " << message << endl;
        }

        static void set(int bit) { debug = (bool) bit; }
};

struct MemoryRegisters { uint32_t MAR, MDR; };

class PipelinedSimulator;

class ForwardingUnit {
    private:
        PipelinedSimulator* simulator;
    
    public:
        ForwardingUnit(PipelinedSimulator* sim) : simulator(sim) {}
    
        // Checks if forwarding is needed and returns the source
        // 0: No forwarding needed (use register file)
        // 1: Forward from EX/MEM (RZ)
        // 2: Forward from MEM/WB (RY)
        bool CheckForwardA_EX(uint32_t rs1);
        bool CheckForwardB_EX(uint32_t rs2);
        bool CheckForwardA_MEM(uint32_t rs1);
        bool CheckForwardB_MEM(uint32_t rs2);
};

class IAG {
    private:
        
    public:
        const int INSTRUCTION_SIZE = 4;
        uint32_t PC, buffer_PC;
        void UpdateBuffer();

        void UpdatePC();
        void UpdateFlush();
        // void UpdatePC_Memory();

        PipelinedSimulator* simulator;
};

class ControlCircuit {
    public:
        void IncrementClock() { clock += 1; }
        uint32_t CyclesExecuted() { return clock; }

        void UpdateControlSignals();
        void UpdateDecodeSignals();
        void UpdateExecuteSignals();
        void UpdateMemorySignals();
        void UpdateWritebackSignals();
        void UpdateIAGSignals();

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
        
    private:
        PipelinedSimulator* simulator;
        uint32_t clock = 0;

    friend class PipelinedSimulator;
};

class ProcessorMemoryInterface {
    public:
        // Used for load/store instructions
        Instruction GetInstruction();
        void GetDataValue(int bytes = 0);
        void StoreDataValue(int bytes = 0);

        // Used for initial parsing
        void AddInstruction(uint32_t location, Instruction instruction);
        void AddData(uint32_t location, uint32_t data, int bytes = 0);

        map<uint32_t, uint8_t> data_map;
        map<uint32_t, Instruction> text_map;

        MemoryRegisters instruction_memory;
        MemoryRegisters data_memory;

        bool is_data_forwarded = false;
};

enum class PredictionState {
    STRONGLY_NOT_TAKEN,
    WEAKLY_NOT_TAKEN,
    WEAKLY_TAKEN,
    STRONGLY_TAKEN
};

class PHT {
    private:
        map<uint32_t, PredictionState> predictionTable;
        const PredictionState initialPrediction = PredictionState::WEAKLY_TAKEN;

    public:
        PredictionState getCurrentState(uint32_t pc);
        bool getPrediction(uint32_t pc);
        bool isMisprediction(uint32_t pc, bool actualOutcome);
        void updatePrediction(uint32_t pc, bool actualOutcome);
        void printTable();
};

class BTB {
    private:
        map<uint32_t, pair<bool, uint32_t>> branchTargetBuffer;

    public:
        bool hasEntry(uint32_t pc);
        bool isUnconditionalBranch(uint32_t pc);
        uint32_t getTargetAddress(uint32_t pc);
        void updateEntry(uint32_t pc, uint32_t targetAddress, bool isUnconditional);
        void printTable();
};

class HazardDetectionUnit {
    private:
        map<uint32_t, Instruction> instruction_map;
        int INSTRUCTION_SIZE;
        
    public:
        map<uint32_t, pair<bool, bool>> data_dependency_bits;
        map<uint32_t, pair<uint32_t, uint32_t>> data_dependency_map;

        void ExtractDataDependencies();
        bool HasDataDependency(Instruction premier_instruction, Instruction former_instruction);
        Instruction NextinstructionForDependency(Instruction current_instruction);

        uint8_t cycles_to_stall = 0;
        // uint8_t stall_index = 0;
        bool next_cycle_stall = false;

        PipelinedSimulator* simulator;
};

class PipelinedSimulator {
    private:
        static const int REGISTER_COUNT = 32;
        uint32_t register_file[REGISTER_COUNT];

        static const int PIPELINE_STAGES = 5;

        // map<uint32_t, uint8_t> data_map;
        // map<uint32_t, Instruction> text_map;
        
        // uint32_t MAR = 0x00000000;
        // uint32_t MDR = 0x00000000;
        uint32_t IR = 0x00000000;
        // uint32_t PC = 0x00000000;
        // uint32_t PC_temp = 0x00000000;

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
        bool printPipelineRegisters = true;
        bool printInstructions = true;
        bool printFetchedInstructionDetails = true;
        bool previouslyPrintingPipelineRegisters = true;
        int specified_instruction = 0; // 1-based indexing, i.e., 0 represents disabled / no instruction
        bool printPredictionDetails = true;

        bool CheckDataHazard();
        void ForwardData();
        bool NeedsForwardingA(uint8_t rs);
        bool NeedsForwardingB(uint8_t rs);
        uint32_t GetForwardedValue(uint8_t rs);
    
    public:
        ifstream fin;
        void Run(char** argV, bool each_stage);
        void Step(char** argV, bool each_stage);
        void RunInstruction(bool each_stage);
        
        Instruction ExtractInstruction(string machine_code);
        uint32_t GenerateMask(int length);
        string GetStageName(Stage stage);
        
        void InitializeRegisters();
        void InitialParse();
        void Reset_x0();
        
        void PrintRegisterFile();
        void PrintPipelineRegisters();
        void PrintSpecifiedPipelineRegisters();
        void PrintInstructions();

        uint32_t GetInstructionNumber(uint32_t address);
        Instruction GetSpecifiedInstruction();

        void ShiftInstructionsStage();
        void UpdateBufferRegisters();
        void Flush();
        bool recently_flushed = false;
        bool actual_outcome = true;
        bool return_jump = false;

        void SetKnob1(bool set_value);
        void SetKnob2(bool set_value);
        void SetKnob3(bool set_value);
        void SetKnob4(bool set_value);
        void SetKnob5(uint32_t instruction_index);
        void SetKnob6(bool set_value);
        void SetKnob7(bool set_value);
        void SetKnob8(bool set_value);

        void PrintInstructionInfo(Instruction instruction);

        InterStageRegisters inter_stage;
        InterStageRegisters buffer;

        Assembler assembler;
        ForwardingUnit forwarding_unit;
        ControlCircuit control;
        ProcessorMemoryInterface memory;
        IAG iag;
        HazardDetectionUnit hdu;
        PHT pht;
        BTB btb;

        Instruction instructions[PIPELINE_STAGES]; // 0 - Fetch, ..., 4 - Writeback

        PipelinedSimulator(const string assembly_file, const string machine_file) : assembler(assembly_file, machine_file), forwarding_unit(this) {
            assembler.Assemble();
            fin.open(machine_file);
            
            control.simulator = this;
            hdu.simulator = this;
            iag.simulator = this;

            InitializeRegisters();
            InitialParse();
            for (size_t i = 0; i < PIPELINE_STAGES; i++) instructions[i] = NULL_INSTRUCTION;
        };

        friend class ControlCircuit;
};

#endif