#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "operations.hpp"
#include "lexer.hpp"

#define TEXT_ADDRESS 0x00000000
#define DATA_ADDRESS 0x10000000
#define HEAP_ADDRESS 0x10008000
#define STACK_ADDRESS 0x7FFFFFDC

#define TEXT_END "END-OF-TEXT-SEGMENT"
#define DATA_END "END-OF-DATA-SEGMENT"

const map<string, int> register_ID = {
    {"x0", 0},   {"zero", 0},
    {"x1", 1},   {"ra", 1},
    {"x2", 2},   {"sp", 2},
    {"x3", 3},   {"gp", 3},
    {"x4", 4},   {"tp", 4},
    {"x5", 5},   {"t0", 5},
    {"x6", 6},   {"t1", 6},
    {"x7", 7},   {"t2", 7},
    {"x8", 8},   {"s0", 8},   {"fp", 8},
    {"x9", 9},   {"s1", 9},
    {"x10", 10}, {"a0", 10},
    {"x11", 11}, {"a1", 11},
    {"x12", 12}, {"a2", 12},
    {"x13", 13}, {"a3", 13},
    {"x14", 14}, {"a4", 14},
    {"x15", 15}, {"a5", 15},
    {"x16", 16}, {"a6", 16},
    {"x17", 17}, {"a7", 17},
    {"x18", 18}, {"s2", 18},
    {"x19", 19}, {"s3", 19},
    {"x20", 20}, {"s4", 20},
    {"x21", 21}, {"s5", 21},
    {"x22", 22}, {"s6", 22},
    {"x23", 23}, {"s7", 23},
    {"x24", 24}, {"s8", 24},
    {"x25", 25}, {"s9", 25},
    {"x26", 26}, {"s10", 26},
    {"x27", 27}, {"s11", 27},
    {"x28", 28}, {"t3", 28},
    {"x29", 29}, {"t4", 29},
    {"x30", 30}, {"t5", 30},
    {"x31", 31}, {"t6", 31}
};

class Assembler {
    private:
        ofstream fout;
        map<string, uint32_t> label_address;
        Lexer lexer;

        const uint8_t INSTRUCTION_SIZE = 4;
        const uint8_t ADDRESS_SIZE = 8;
        map<uint32_t, pair<uint32_t, uint32_t>> data_map;
        map<uint32_t, pair<uint32_t, string>> text_map;

        // Checking for valid operation arguments
        bool IsValidRegister(string input_register, bool log_error = false);
        bool IsValidImmediate(string input_immediate, Format format, bool log_error = false);
        bool IsValidData(string input_data, string directive, bool log_error = false);
        
        // Standardize format to basic RISC-V Code
        bool HasCorrectOperandsFormat(vector<string> tokens);
        vector<string> TransformOffsetRegisterFormat(vector<string> tokens, bool log_error = true);
        vector<string> CorrectLabels(vector<string> tokens, uint32_t current_address, bool log_error = true);
        
        // Extract values for text and data segment
        uint32_t ExtractData(vector<string> tokens, uint32_t current_address);
        uint32_t ExtractText(vector<string> tokens, uint32_t current_address);

        // Extract command-specific machine code
        uint32_t Get_R_Code(vector<string> tokens);
        uint32_t Get_I_Code(vector<string> tokens);
        uint32_t Get_S_Code(vector<string> tokens);
        uint32_t Get_SB_Code(vector<string> tokens);
        uint32_t Get_U_Code(vector<string> tokens);
        uint32_t Get_UJ_Code(vector<string> tokens);

        // Compute label addresses and any possible errors in initial parse
        void InitialParse();
        
    public:
        void Assemble();
        void ShowOutput();

        Assembler(const string &assembly_file, const string &machine_file) : lexer(assembly_file) {
            fout.open(machine_file);
            if (!fout.is_open()) {
                error_stream << "Couldn't open output file: " << assembly_file << endl;
                return;
            }
        };
};

#endif