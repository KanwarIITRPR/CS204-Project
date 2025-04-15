#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "utils.hpp"

enum class Format {
    R,
    I,
    S,
    SB,
    U,
    UJ,
    INVALID
};

// 7-bit operation code
const map<string, uint8_t> opcode = {
    // R - Format
    {"add", 0b0110011},
    {"sub", 0b0110011},
    {"mul", 0b0110011},
    {"div", 0b0110011},
    {"rem", 0b0110011},
    {"and", 0b0110011},
    {"or",  0b0110011},
    {"xor", 0b0110011},
    {"sll", 0b0110011},
    {"slt", 0b0110011},
    {"sra", 0b0110011},
    {"srl", 0b0110011},

    // I - Format
    {"addi", 0b0010011},
    {"andi", 0b0010011},
    {"ori",  0b0010011},
    {"xori", 0b0010011},
    {"lb",   0b0000011},
    {"lh",   0b0000011},
    {"lw",   0b0000011},
    {"jalr", 0b1100111},

    // S - Format
    {"sb", 0b0100011}, 
    {"sh", 0b0100011}, 
    {"sw", 0b0100011}, 

    // SB - Format
    {"beq", 0b1100011},
    {"bne", 0b1100011},
    {"blt", 0b1100011},
    {"bge", 0b1100011},

    // U - Format
    {"lui",   0b0110111},
    {"auipc", 0b0010111},

    // UJ - Format
    {"jal", 0b1101111}
};

// 3-bit function
const map<string, uint8_t> funct3 = {
    // R - Format
    {"add", 0b000},
    {"sub", 0b000},
    {"mul", 0b000},
    {"div", 0b100},
    {"rem", 0b110},
    {"and", 0b111},
    {"or",  0b110},
    {"xor", 0b100},
    {"sll", 0b001},
    {"slt", 0b010},
    {"sra", 0b101},
    {"srl", 0b101},

    // I - Format
    {"addi", 0b000},
    {"andi", 0b111},
    {"ori",  0b110},
    {"xori", 0b100},
    {"lb",   0b000},
    {"lh",   0b001},
    {"lw",   0b010},
    {"jalr", 0b000},

    // S - Format
    {"sb", 0b000}, 
    {"sh", 0b001}, 
    {"sw", 0b010}, 

    // SB - Format
    {"beq", 0b000},
    {"bne", 0b001},
    {"blt", 0b100},
    {"bge", 0b101}
};

// 7-bit function
const map<string, uint8_t> funct7 = {
    // R - Format
    {"add", 0b0000000},
    {"sub", 0b0100000},
    {"mul", 0b0000001},
    {"div", 0b0000001},
    {"rem", 0b0000001},
    {"and", 0b0000000},
    {"or",  0b0000000},
    {"xor", 0b0000000},
    {"sll", 0b0000000},
    {"slt", 0b0000000},
    {"sra", 0b0100000},
    {"srl", 0b0000000},
};

const map<string, uint8_t> directive_size = {
    {".text",   0},
    {".data",   0},
    {".byte",   1},
    {".half",   2},
    {".word",   4},
    {".dword",  8},
    {".asciiz", 1}
};

bool IsValidOperation(string operation, bool log_error = false);
bool IsValidDirective(string directive, bool log_error = false);
bool IsLoadOperation(string operation);

Format GetFormat(string operation);
string GetFormatName(Format format);

#endif