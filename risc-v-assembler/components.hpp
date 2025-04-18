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
    uint32_t machine_code, immediate;
    uint8_t opcode, rd, rs1, rs2, funct3, funct7;
    string literal;
    Format format;
    Stage stage;
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

class IAG {
    private:
        
    public:
        const int INSTRUCTION_SIZE = 4;
        uint32_t PC, buffer_PC;
        void UpdateBuffer();

        void UpdatePC();
};

class PipelinedSimulator;
class ControlCircuit {
    public:
        void IncrementClock() { clock += 1; }
        uint32_t CyclesExecuted() { return clock; }

        void UpdateControlSignals();
        void UpdateDecodeSignals();
        void UpdateExecuteSignals();
        void UpdateMemorySignals();
        void UpdateWritebackSignals();

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
};

class PipelinedSimulator {
    private:
        static const int REGISTER_COUNT = 32;
        uint32_t register_file[REGISTER_COUNT];

        static const int PIPELINE_STAGES = 5;
        string stage_name[PIPELINE_STAGES] = {"Fetch", "Decode", "Execute", "Memory Access", "Writeback"};

        // map<uint32_t, uint8_t> data_map;
        // map<uint32_t, Instruction> text_map;

        
        InterStageRegisters inter_stage;
        InterStageRegisters buffer;
        
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
        int specified_instruction = 0; // 1-based indexing, i.e., 0 represents disabled / no instruction
        bool printPredictionDetails = true;
    
        public:
        ifstream fin;
        void Run(char** argV, bool each_stage);
        void Step(char** argV, bool each_stage);
        void RunInstruction(bool each_stage);
        
        Instruction ExtractInstruction(string machine_code);
        uint32_t GenerateMask(uint8_t length);
        
        void InitializeRegisters();
        void InitialParse();
        void Reset_x0();
        
        void PrintRegisterFile();
        void PrintPipelineRegisters();
        void PrintInstructions();

        void UpdateBufferRegisters();

        void SetKnob1(bool set_value);
        void SetKnob2(bool set_value);
        void SetKnob3(bool set_value);
        void SetKnob4(bool set_value);
        void SetKnob5(int instruction_index);
        void SetKnob6(bool set_value);
        void SetKnob7(bool set_value);

        Assembler assembler;
        ControlCircuit control;
        ProcessorMemoryInterface memory;
        IAG iag;

        Instruction instructions[PIPELINE_STAGES]; // 0 - Fetch, ..., 4 - Writeback

        PipelinedSimulator(const string assembly_file, const string machine_file) : assembler(assembly_file, machine_file) {
            assembler.Assemble();
            fin.open(machine_file);

            InitializeRegisters();
            InitialParse();
            for (size_t i = 0; i < PIPELINE_STAGES; i++) instructions[i] = NULL_INSTRUCTION;

            control.simulator = this;
        };

        friend class ControlCircuit;
};

#endif