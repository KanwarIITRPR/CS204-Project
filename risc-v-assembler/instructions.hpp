#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "operations.hpp"

#define NULL_INSTRUCTION Instruction()

// Lets the compiler know the size of the enum, so as to use it without writing the definition
enum class Stage: int;

struct MachineCodeDivision {
    uint8_t opcode, rd, rs1, rs2, funct3, funct7;
    uint32_t immediate;
};

struct Instruction {
    string machine_code, literal;
    MachineCodeDivision divison;
    Format format;
    Stage stage;
};

#endif