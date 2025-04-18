#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "assembler.hpp"
#include "control.hpp"
#include "memory.hpp"
#include "IAG.hpp"

enum class Stage {
    QUEUED,
    FETCH,
    DECODE,
    EXECUTE,
    MEMORY_ACCESS,
    WRITEBACK,
    COMMITTED
};

struct InterStageRegisters {
    uint32_t RA, RB;
    uint32_t RM;
    uint32_t RY, RZ;
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
        bool printBufferRegisters = true;
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

        void RegisterState();

        void SetKnob1(bool set_value);
        void SetKnob2(bool set_value);
        void SetKnob3(bool set_value);
        void SetKnob4(bool set_value);
        void SetKnob5(int instruction_index);
        void SetKnob6(bool set_value);

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

#endif