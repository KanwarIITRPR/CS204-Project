#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <emscripten.h>
#include <emscripten/bind.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <iomanip>
#include <bitset>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include "../types.hpp"

using namespace riscv;

enum class Stage { FETCH, DECODE, EXECUTE, MEMORY, WRITEBACK };
enum class InstructionType { R, I, S, SB, U, UJ };

struct Instruction {
    uint32_t PC, opcode, rs1, rs2, rd, instruction, func3, func7;
    InstructionType instructionType;
    Stage stage;
    Instruction() : opcode(0), rs1(0), rs2(0), rd(0), stage(Stage::FETCH), instruction(0), func3(0), func7(0), PC(0) {}
};

struct InstructionRegisters {
    uint32_t RA, RB, RM, RY, RZ;
    InstructionRegisters() : RA(0), RB(0), RM(0), RZ(0), RY(0) {}
};

class Simulator {
private:
    static constexpr int NUMREGISTERS = 32;
    static constexpr uint32_t MEMORY_SIZE = 0x80000000;

    uint32_t PC, registers[NUMREGISTERS];
    std::unordered_map<uint32_t, uint8_t> dataMap;
    std::map<uint32_t, std::pair<uint32_t, std::string>> textMap;
    uint64_t clockCycles;
    
    InstructionRegisters instructionRegisters;
    Instruction currentInstruction;
    bool running;

    void initialiseRegisters();
    bool isValidAddress(uint32_t, uint32_t);
    InstructionType classifyInstructions(uint32_t);
    void fetchInstruction();
    void decodeInstruction();
    void executeInstruction();
    void memoryAccess();
    void writeback();

public:
    std::map<int, std::string> logs;
    void reset();
    Simulator() { reset(); }

    bool loadProgram(const std::string &input) {
        std::stringstream ss(input);
        std::string line;
        reset();
        running = true;

        try {
            while (getline(ss, line)) {
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {return !std::isspace(ch);}));
                line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {return !std::isspace(ch);}).base(), line.end());
                if (line.empty() || line[0] == '#') continue;
                std::stringstream line_ss(line);
                std::string addrStr, instStr;
                if (!(line_ss >> addrStr >> instStr)) {
                    logs[404] = "Malformed instruction line: '" + line + "'. Expected format: <address> <instruction>";
                    return false;
                }
                uint32_t addr = std::stoul(addrStr, nullptr, 16);
                uint32_t data = std::stoul(instStr, nullptr, 16);
                if (addr >= DATA_SEGMENT_START) {
                    dataMap[addr] = data;
                } else {
                    textMap[addr] = {data, parseInstructions(data)};
                }
            }
            logs[200] = "Program loaded successfully with " + std::to_string(textMap.size()) + " instructions";
            return true;
        } catch (const std::exception& e) {
            logs[404] = "Failed to load program: " + std::string(e.what());
            emscripten_log(EM_LOG_ERROR, "%s", e.what());
            return false;
        }
    }

    bool step();
    void run();
    uint32_t getInstruction() const;
    std::string parseInstructions(uint32_t instHex);
    const uint32_t* getRegisters() const;
    uint32_t getPC() const;
    uint32_t getCycles() const;
    InstructionRegisters getPipelineRegisters() const;
    std::unordered_map<uint32_t, uint8_t> getDataMap() const;
    std::map<uint32_t, std::pair<uint32_t, std::string>> getTextMap() const;
    Stage getCurrentStage() const;
    std::map<int, std::string> getConsoleOutput() const {
        std::ostringstream oss;
        oss << "PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << currentInstruction.PC << ", Stage: ";
        std::map<int, std::string> response = logs;
        return response;
    }
    bool isRunning() const;
};

void Simulator::initialiseRegisters() {
    for (int i = 0; i < 32; i++) registers[i] = 0x00000000;
    registers[2] = 0x7FFFFFDC;
    registers[3] = 0x10000000;
    registers[10] = 0x00000001;
    registers[11] = 0x7FFFFFDC;
}

void Simulator::reset() {
    currentInstruction = Instruction();
    instructionRegisters = InstructionRegisters();
    initialiseRegisters();
    dataMap.clear();
    PC = TEXT_SEGMENT_START;
    clockCycles = 0;
    running = false;
    textMap.clear();
    logs.clear();
}

InstructionType Simulator::classifyInstructions(uint32_t instHex) {
    uint32_t opcode = instHex & 0x7F;
    uint32_t func3 = (instHex >> 12) & 0x7;
    uint32_t func7 = (instHex >> 25) & 0x7F;

    auto rTypeEncoding = RTypeInstructions::getEncoding();
    for (const auto &[name, op] : rTypeEncoding.opcodeMap) {
        if (op == opcode && rTypeEncoding.func3Map.at(name) == func3 && rTypeEncoding.func7Map.at(name) == func7) return InstructionType::R;
    }

    auto iTypeEncoding = ITypeInstructions::getEncoding();
    for (const auto &[name, op] : iTypeEncoding.opcodeMap) {
        if (op == opcode && iTypeEncoding.func3Map.at(name) == func3) return InstructionType::I;
    }

    auto sTypeEncoding = STypeInstructions::getEncoding();
    for (const auto &[name, op] : sTypeEncoding.opcodeMap) {
        if (op == opcode && sTypeEncoding.func3Map.at(name) == func3) return InstructionType::S;
    }

    auto sbTypeEncoding = SBTypeInstructions::getEncoding();
    for (const auto &[name, op] : sbTypeEncoding.opcodeMap) {
        if (op == opcode && sbTypeEncoding.func3Map.at(name) == func3) return InstructionType::SB;
    }

    auto uTypeEncoding = UTypeInstructions::getEncoding();
    for (const auto &[name, op] : uTypeEncoding.opcodeMap) {
        if (op == opcode) return InstructionType::U;
    }

    auto ujTypeEncoding = UJTypeInstructions::getEncoding();
    for (const auto &[name, op] : ujTypeEncoding.opcodeMap) {
        if (op == opcode) return InstructionType::UJ;
    }
    
    std::stringstream ss;
    ss << "Instruction 0x" << std::hex << instHex << " could not be classified: Invalid opcode (0x" << opcode << ")";
    logs[400] = ss.str();
    throw std::runtime_error(ss.str());
}

void Simulator::fetchInstruction() {
    if (!isValidAddress(PC, 4)) {
        std::ostringstream oss;
        oss << "Fetch error: Invalid PC address 0x" << std::hex << PC << " (out of bounds)";
        logs[400] = oss.str();
        throw std::runtime_error(oss.str());
    }
    auto it = textMap.find(PC);
    currentInstruction = Instruction();
    instructionRegisters = InstructionRegisters();
    if (it != textMap.end()) {
        uint32_t rawInstruction = it->second.first;
        currentInstruction.instruction = rawInstruction;
        currentInstruction.instructionType = classifyInstructions(currentInstruction.instruction);
        currentInstruction.PC = PC;
        if (currentInstruction.instruction == 0xDEADBEEF) {
            running = false;
            return;
        }
        PC += INSTRUCTION_SIZE;
        return;
    }
    running = false;
}

void Simulator::decodeInstruction() {
    currentInstruction.opcode = currentInstruction.instruction & 0x7F;
    currentInstruction.rd = (currentInstruction.instruction >> 7) & 0x1F;
    currentInstruction.func3 = (currentInstruction.instruction >> 12) & 0x7;
    currentInstruction.rs1 = (currentInstruction.instruction >> 15) & 0x1F;
    currentInstruction.rs2 = (currentInstruction.instruction >> 20) & 0x1F;
    currentInstruction.func7 = (currentInstruction.instruction >> 25) & 0x7F;

    instructionRegisters.RA = registers[currentInstruction.rs1];
    
    switch (currentInstruction.instructionType) {
        case InstructionType::R:
            instructionRegisters.RB = registers[currentInstruction.rs2];
            break;

        case InstructionType::I: {
            int32_t imm = (currentInstruction.instruction >> 20) & 0xFFF;
            if (imm & 0x800) imm |= 0xFFFFF000;
            instructionRegisters.RB = imm;
            break;
        }

        case InstructionType::S: {
            int32_t imm = ((currentInstruction.instruction >> 25) << 5) | ((currentInstruction.instruction >> 7) & 0x1F);
            if (imm & 0x800) imm |= 0xFFFFF000;
            instructionRegisters.RB = imm;
            instructionRegisters.RM = registers[currentInstruction.rs2];
            break;
        }

        case InstructionType::SB: {
            int32_t imm = ((currentInstruction.instruction >> 31) << 12) | (((currentInstruction.instruction >> 7) & 1) << 11) | (((currentInstruction.instruction >> 25) & 0x3F) << 5) | (((currentInstruction.instruction >> 8) & 0xF) << 1);
            if (imm & 0x1000) imm |= 0xFFFFE000;
            instructionRegisters.RB = imm;
            break;
        }

        case InstructionType::U:
            instructionRegisters.RB = currentInstruction.instruction & 0xFFFFF000;
            break;

        case InstructionType::UJ: {
            int32_t imm = ((currentInstruction.instruction >> 31) << 20) | (((currentInstruction.instruction >> 12) & 0xFF) << 12) | (((currentInstruction.instruction >> 20) & 1) << 11) | (((currentInstruction.instruction >> 21) & 0x3FF) << 1);
            if (imm & 0x100000) imm |= 0xFFE00000;
            instructionRegisters.RB = imm;
            break;
        }

        default:
            logs[400] = "The instruction is not decoded";
            throw std::runtime_error("Error: The instruction is not decoded");
    }
}

void Simulator::executeInstruction() {
    uint32_t result = 0;

    auto rTypeEncoding = RTypeInstructions::getEncoding();
    for (const auto &[name, op] : rTypeEncoding.opcodeMap) {
        if (op == currentInstruction.opcode && rTypeEncoding.func3Map.at(name) == currentInstruction.func3 && rTypeEncoding.func7Map.at(name) == currentInstruction.func7) {
            if (name == "add") {
                result = instructionRegisters.RA + instructionRegisters.RB;
            } else if (name == "sub") {
                result = instructionRegisters.RA - instructionRegisters.RB;
            } else if (name == "mul") {
                result = instructionRegisters.RA * instructionRegisters.RB;
            } else if (name == "div") {
                if (instructionRegisters.RB == 0) result = 0xFFFFFFFF;
                else result = static_cast<uint32_t>(static_cast<int32_t>(instructionRegisters.RA) / static_cast<int32_t>(instructionRegisters.RB));
            } else if (name == "rem") {
                if (instructionRegisters.RB == 0) result = instructionRegisters.RA;
                else result = static_cast<uint32_t>(static_cast<int32_t>(instructionRegisters.RA) % static_cast<int32_t>(instructionRegisters.RB));
            } else if (name == "and") {
                result = instructionRegisters.RA & instructionRegisters.RB;
            } else if (name == "or") {
                result = instructionRegisters.RA | instructionRegisters.RB;
            } else if (name == "xor") {
                result = instructionRegisters.RA ^ instructionRegisters.RB;
            } else if (name == "sll") {
                result = instructionRegisters.RA << (instructionRegisters.RB & 0x1F);
            } else if (name == "srl") {
                result = instructionRegisters.RA >> (instructionRegisters.RB & 0x1F);
            } else if (name == "sra") {
                result = static_cast<uint32_t>(static_cast<int32_t>(instructionRegisters.RA) >> (instructionRegisters.RB & 0x1F));
            } else if (name == "slt") {
                result = (static_cast<int32_t>(instructionRegisters.RA) < static_cast<int32_t>(instructionRegisters.RB)) ? 1 : 0;
            }
            instructionRegisters.RY = result;
            return;
        }
    }

    auto iTypeEncoding = ITypeInstructions::getEncoding();
    for (const auto &[name, op] : iTypeEncoding.opcodeMap) {
        if (op == currentInstruction.opcode && iTypeEncoding.func3Map.at(name) == currentInstruction.func3) {
            if (name == "addi") {
                result = instructionRegisters.RA + instructionRegisters.RB;
            } else if (name == "andi") {
                result = instructionRegisters.RA & instructionRegisters.RB;
            } else if (name == "ori") {
                result = instructionRegisters.RA | instructionRegisters.RB;
            } else if (name == "xori") {
                result = instructionRegisters.RA ^ instructionRegisters.RB;
            } else if (name == "slti") {
                result = (static_cast<int32_t>(instructionRegisters.RA) < static_cast<int32_t>(instructionRegisters.RB)) ? 1 : 0;
            } else if (name == "sltiu") {
                result = (instructionRegisters.RA < instructionRegisters.RB) ? 1 : 0;
            } else if (name == "slli") {
                result = instructionRegisters.RA << (instructionRegisters.RB & 0x1F);
            } else if (name == "srli") {
                result = instructionRegisters.RA >> (instructionRegisters.RB & 0x1F);
            } else if (name == "srai") {
                result = static_cast<uint32_t>(static_cast<int32_t>(instructionRegisters.RA) >> (instructionRegisters.RB & 0x1F));
            } else if (name == "lb" || name == "lh" || name == "lw") {
                result = instructionRegisters.RA + instructionRegisters.RB;
                instructionRegisters.RY = result;
                return;
            } else if (name == "ld") {
                logs[400] = "ld instruction not supported";
                throw std::runtime_error("Error: ld instruction not supported");
            } else if (name == "jalr") {
                result = PC;
                PC = (instructionRegisters.RA + instructionRegisters.RB) & ~1;
            }
            instructionRegisters.RY = result;
            return;
        }
    }

    auto sTypeEncoding = STypeInstructions::getEncoding();
    for (const auto &[name, op] : sTypeEncoding.opcodeMap) {
        if (op == currentInstruction.opcode && sTypeEncoding.func3Map.at(name) == currentInstruction.func3) {
            if (name == "sb" || name == "sh" || name == "sw" || name == "sd") {
                result = instructionRegisters.RA + instructionRegisters.RB;
                instructionRegisters.RY = result;
            }
            return;
        }
    }

    auto sbTypeEncoding = SBTypeInstructions::getEncoding();
    for (const auto &[name, op] : sbTypeEncoding.opcodeMap) {
        if (op == currentInstruction.opcode && sbTypeEncoding.func3Map.at(name) == currentInstruction.func3) {
            bool branchTaken = false;
            if (name == "beq") {
                branchTaken = (instructionRegisters.RA == instructionRegisters.RB);
            } else if (name == "bne") {
                branchTaken = (instructionRegisters.RA != instructionRegisters.RB);
            } else if (name == "blt") {
                branchTaken = (static_cast<int32_t>(instructionRegisters.RA) < static_cast<int32_t>(instructionRegisters.RB));
            } else if (name == "bge") {
                branchTaken = (static_cast<int32_t>(instructionRegisters.RA) >= static_cast<int32_t>(instructionRegisters.RB));
            } else if (name == "bltu") {
                branchTaken = (instructionRegisters.RA < instructionRegisters.RB);
            } else if (name == "bgeu") {
                branchTaken = (instructionRegisters.RA >= instructionRegisters.RB);
            }
            if (branchTaken) {
                PC = currentInstruction.PC + instructionRegisters.RB;
            }
            instructionRegisters.RY = branchTaken;
            return;
        }
    }

    auto uTypeEncoding = UTypeInstructions::getEncoding();
    for (const auto &[name, op] : uTypeEncoding.opcodeMap) {
        if (op == currentInstruction.opcode) {
            if (name == "lui") {
                result = instructionRegisters.RB;
            } else if (name == "auipc") {
                result = currentInstruction.PC + instructionRegisters.RB;
            }
            instructionRegisters.RY = result;
            return;
        }
    }

    auto ujTypeEncoding = UJTypeInstructions::getEncoding();
    for (const auto &[name, op] : ujTypeEncoding.opcodeMap) {
        if (op == currentInstruction.opcode) {
            if (name == "jal") {
                result = PC;
                PC = currentInstruction.PC + instructionRegisters.RB;
            }
            instructionRegisters.RY = result;
            return;
        }
    }
    logs[400] = "The Instruction is not executed";
    throw std::runtime_error("Error: The Instruction is not executed");
}

void Simulator::memoryAccess() {
    uint32_t address = instructionRegisters.RY;
    instructionRegisters.RZ = instructionRegisters.RY;
    
    auto iTypeEncoding = ITypeInstructions::getEncoding();
    for (const auto &[name, op] : iTypeEncoding.opcodeMap) {
        if (op == currentInstruction.opcode && iTypeEncoding.func3Map.at(name) == currentInstruction.func3) {
            if (name == "lb") {
                isValidAddress(address, 1);
                instructionRegisters.RZ = dataMap.count(address) ? static_cast<int8_t>(dataMap[address]) : 0;
            } else if (name == "lh") {
                isValidAddress(address, 2);
                instructionRegisters.RZ = static_cast<int16_t>(
                    (dataMap.count(address + 1) ? dataMap[address + 1] : 0) << 8 |
                    (dataMap.count(address) ? dataMap[address] : 0)
                );
            } else if (name == "lw") {
                isValidAddress(address, 4);
                instructionRegisters.RZ = 
                    ((dataMap.count(address + 3) ? dataMap[address + 3] : 0) << 24) |
                    ((dataMap.count(address + 2) ? dataMap[address + 2] : 0) << 16) |
                    ((dataMap.count(address + 1) ? dataMap[address + 1] : 0) << 8)  |
                    (dataMap.count(address) ? dataMap[address] : 0);
            }
            return;
        }
    }

    auto sTypeEncoding = STypeInstructions::getEncoding();
    for (const auto &[name, op] : sTypeEncoding.opcodeMap) {
        if (op == currentInstruction.opcode && sTypeEncoding.func3Map.at(name) == currentInstruction.func3) {
            uint32_t valueToStore = instructionRegisters.RM;
            if (name == "sb") {
                dataMap[address] = valueToStore & 0xFF;
            } else if (name == "sh") {
                dataMap[address] = valueToStore & 0xFF;
                dataMap[address + 1] = (valueToStore >> 8) & 0xFF;
            } else if (name == "sw") {
                dataMap[address] = valueToStore & 0xFF;
                dataMap[address + 1] = (valueToStore >> 8) & 0xFF;
                dataMap[address + 2] = (valueToStore >> 16) & 0xFF;
                dataMap[address + 3] = (valueToStore >> 24) & 0xFF;
            }
            return;
        }
    }
}

void Simulator::writeback() {
    if (currentInstruction.rd != 0) {
        switch (currentInstruction.instructionType) {
            case InstructionType::R:
            case InstructionType::I:
            case InstructionType::U:
            case InstructionType::UJ:
                registers[currentInstruction.rd] = instructionRegisters.RZ;
                break;
            case InstructionType::S:
            case InstructionType::SB:
                break;
            default:
                logs[400] = "Unknown instruction type in writeback";
                throw std::runtime_error("Error: Unknown instruction type in writeback");
        }
    }
    registers[0] = 0;
}

bool Simulator::isValidAddress(uint32_t addr, uint32_t size) {
    if (addr + size > MEMORY_SIZE || addr + size < 0x0) {
        std::stringstream ss;
        ss << "Memory access error: Address 0x" << std::hex << addr << " with size " << std::dec << size 
           << " is outside of valid memory range (0x0-0x" << std::hex << MEMORY_SIZE << ")";
        logs[300] = ss.str();
        throw std::runtime_error(ss.str());
    }
    return true;
}

bool Simulator::step() {
    if (!running) {
        logs[400] = "Cannot step - simulator is not running";
        return false;
    }

    try {
        std::stringstream log;
        
        switch (currentInstruction.stage) {
            case Stage::FETCH:
                fetchInstruction();
                log << "FETCH: Instruction fetched at PC: 0x" << std::hex << currentInstruction.PC 
                         << ", Next PC: 0x" << PC 
                         << ", Instruction: 0x" << std::hex << currentInstruction.instruction;
                logs[200] = log.str();
                if (!running) {
                    logs[200] = "Program execution complete - reached end of instructions";
                    return false;
                }
                currentInstruction.stage = Stage::DECODE;
                clockCycles += 1;
                break;

            case Stage::DECODE:
                decodeInstruction();
                log << "DECODE: Instruction at PC: 0x" << std::hex << currentInstruction.PC 
                          << ", Opcode: 0x" << currentInstruction.opcode;
                
                if (currentInstruction.instructionType == InstructionType::R) {
                    log << ", rs1: x" << std::dec << currentInstruction.rs1 
                        << ", rs2: x" << currentInstruction.rs2
                        << ", rd: x" << currentInstruction.rd;
                } else if (currentInstruction.instructionType == InstructionType::I) {
                    log << ", rs1: x" << std::dec << currentInstruction.rs1 
                        << ", rd: x" << currentInstruction.rd
                        << ", imm: 0x" << std::hex << instructionRegisters.RB;
                } else if (currentInstruction.instructionType == InstructionType::S) {
                    log << ", rs1: x" << std::dec << currentInstruction.rs1 
                        << ", rs2: x" << currentInstruction.rs2
                        << ", offset: 0x" << std::hex << instructionRegisters.RB;
                } else if (currentInstruction.instructionType == InstructionType::SB) {
                    log << ", rs1: x" << std::dec << currentInstruction.rs1 
                        << ", rs2: x" << currentInstruction.rs2
                        << ", branch offset: 0x" << std::hex << instructionRegisters.RB;
                } else if (currentInstruction.instructionType == InstructionType::U) {
                    log << ", rd: x" << std::dec << currentInstruction.rd
                        << ", imm: 0x" << std::hex << instructionRegisters.RB;
                } else if (currentInstruction.instructionType == InstructionType::UJ) {
                    log << ", rd: x" << std::dec << currentInstruction.rd
                        << ", jump offset: 0x" << std::hex << instructionRegisters.RB;
                }
                
                logs[200] = log.str();
                currentInstruction.stage = Stage::EXECUTE;
                clockCycles += 1;
                break;

            case Stage::EXECUTE:
                executeInstruction();
                log << "EXECUTE: Instruction at PC: 0x" << std::hex << currentInstruction.PC;
                if (currentInstruction.instructionType == InstructionType::R || currentInstruction.instructionType == InstructionType::I) {
                    log << ", RA: 0x" << instructionRegisters.RA
                        << ", RB: 0x" << instructionRegisters.RB
                        << ", Result: 0x" << instructionRegisters.RY;
                } else if (currentInstruction.instructionType == InstructionType::S) {
                    log << ", RA: 0x" << instructionRegisters.RA
                        << ", Offset: 0x" << instructionRegisters.RB
                        << ", RM: 0x" << instructionRegisters.RM
                        << ", Address: 0x" << instructionRegisters.RY;
                } else if (currentInstruction.instructionType == InstructionType::SB) {
                    log << ", RA: 0x" << instructionRegisters.RA
                        << ", RB: 0x" << instructionRegisters.RB
                        << ", Branch " << (instructionRegisters.RY ? "taken" : "not taken");
                } else if (currentInstruction.instructionType == InstructionType::U || currentInstruction.instructionType == InstructionType::UJ) {
                    log << ", Imm: 0x" << instructionRegisters.RB
                        << ", Result: 0x" << instructionRegisters.RY;
                }
                
                logs[200] = log.str();
                currentInstruction.stage = Stage::MEMORY;
                clockCycles += 1;
                break;

            case Stage::MEMORY:
                memoryAccess();
                log << "MEMORY: Instruction at PC: 0x" << std::hex << currentInstruction.PC;

                if (currentInstruction.instructionType == InstructionType::I) {
                    if (currentInstruction.opcode == 0x03) {
                        log << ", Load address: 0x" << instructionRegisters.RY
                            << ", Data loaded: 0x" << instructionRegisters.RZ;
                    } else {
                        log << ", No memory access";
                    }
                } else if (currentInstruction.instructionType == InstructionType::S) {
                    log << ", Store address: 0x" << instructionRegisters.RY
                        << ", Value stored: 0x" << instructionRegisters.RM;
                } else {
                    log << ", No memory access";
                }
                
                logs[200] = log.str();
                currentInstruction.stage = Stage::WRITEBACK;
                clockCycles += 1;
                break;

            case Stage::WRITEBACK:
                writeback();
                log << "WRITEBACK: Instruction at PC: 0x" << std::hex << currentInstruction.PC;
                
                if (currentInstruction.rd != 0) {
                    log << ", Writing 0x" << instructionRegisters.RZ << " to register x" << std::dec << currentInstruction.rd;
                } else if (currentInstruction.instructionType == InstructionType::S) {
                    log << ", Store instruction (no register write)";
                } else if (currentInstruction.instructionType == InstructionType::SB) {
                    log << ", Branch instruction (no register write)";
                } else {
                    log << ", No register write needed";
                }
                
                logs[200] = log.str();
                currentInstruction = Instruction();
                instructionRegisters = InstructionRegisters();
                clockCycles += 1;
                break;

            default:
                logs[404] = "Invalid pipeline stage detected: " + std::to_string(static_cast<int>(currentInstruction.stage));
                throw std::runtime_error("Error: Invalid pipeline stage");
        }
        return running;
    } catch (const std::runtime_error& e) {
        logs[404] = "Runtime error during step execution: " + std::string(e.what());
        emscripten_log(EM_LOG_ERROR, "%s", e.what());
        running = false;
        return false;
    }
}

void Simulator::run() {
    if (!running) {
        logs[400] = "Cannot run - simulator is not running";
        return;
    }

    logs[200] = "Starting full program execution";
    int stepCount = 0;
    while (running) {
        if (!step()) {
            break;
        }
        stepCount++;
        if (stepCount > 100000) {
            logs[400] = "Program execution terminated - exceeded maximum step count (100000)";
            running = false;
            break;
        }
    }
    logs[200] = "Simulation completed. Total instruction cycles: " + std::to_string(clockCycles) + 
                ", Total steps executed: " + std::to_string(stepCount);
}

const uint32_t* Simulator::getRegisters() const { return registers; }

uint32_t Simulator::getPC() const { return PC; }

std::string Simulator::parseInstructions(uint32_t instHex) {
    uint32_t opcode = instHex & 0x7F;
    uint32_t rd = (instHex >> 7) & 0x1F;
    uint32_t func3 = (instHex >> 12) & 0x7;
    uint32_t rs1 = (instHex >> 15) & 0x1F;
    uint32_t rs2 = (instHex >> 20) & 0x1F;
    uint32_t func7 = (instHex >> 25) & 0x7F;

    auto rTypeEncoding = RTypeInstructions::getEncoding();
    for (const auto &[name, op] : rTypeEncoding.opcodeMap) {
        if (op == opcode && rTypeEncoding.func3Map.at(name) == func3 && rTypeEncoding.func7Map.at(name) == func7) {
            std::stringstream ss;
            ss << name << " x" << rd << ", x" << rs1 << ", x" << rs2;
            return ss.str();
        }
    }

    auto iTypeEncoding = ITypeInstructions::getEncoding();
    for (const auto &[name, op] : iTypeEncoding.opcodeMap) {
        if (op == opcode && iTypeEncoding.func3Map.at(name) == func3) {
            int32_t imm = (instHex >> 20);
            if (imm & 0x800) imm |= 0xFFFFF000;
            std::stringstream ss;
            ss << name << " x" << rd << ", x" << rs1 << ", " << imm;
            return ss.str();
        }
    }

    auto sTypeEncoding = STypeInstructions::getEncoding();
    for (const auto &[name, op] : sTypeEncoding.opcodeMap) {
        if (op == opcode && sTypeEncoding.func3Map.at(name) == func3) {
            int32_t imm = ((instHex >> 25) << 5) | ((instHex >> 7) & 0x1F);
            if (imm & 0x800) imm |= 0xFFFFF000;
            std::stringstream ss;
            ss << name << " x" << rs2 << ", " << imm << "(x" << rs1 << ")";
            return ss.str();
        }
    }

    auto sbTypeEncoding = SBTypeInstructions::getEncoding();
    for (const auto &[name, op] : sbTypeEncoding.opcodeMap) {
        if (op == opcode && sbTypeEncoding.func3Map.at(name) == func3) {
            int32_t imm = ((instHex >> 31) << 12) | (((instHex >> 7) & 1) << 11) | (((instHex >> 25) & 0x3F) << 5) | (((instHex >> 8) & 0xF) << 1);
            if (imm & 0x1000) imm |= 0xFFFFE000;
            std::stringstream ss;
            ss << name << " x" << rs1 << ", x" << rs2 << ", " << imm;
            return ss.str();
        }
    }

    auto uTypeEncoding = UTypeInstructions::getEncoding();
    for (const auto &[name, op] : uTypeEncoding.opcodeMap) {
        if (op == opcode) {
            uint32_t imm = instHex & 0xFFFFF000;
            std::stringstream ss;
            ss << name << " x" << rd << ", " << (imm >> 12);
            return ss.str();
        }
    }

    auto ujTypeEncoding = UJTypeInstructions::getEncoding();
    for (const auto &[name, op] : ujTypeEncoding.opcodeMap) {
        if (op == opcode) {
            int32_t imm = ((instHex >> 31) << 20) | (((instHex >> 12) & 0xFF) << 12) | (((instHex >> 20) & 1) << 11) | (((instHex >> 21) & 0x3FF) << 1);
            if (imm & 0x100000) imm |= 0xFFE00000;
            std::stringstream ss;
            ss << name << " x" << rd << ", " << imm;
            return ss.str();
        }
    }
    logs[400] = "The Instruction is not valid";
    throw std::runtime_error("Error: The Instruction is not valid");
}

uint32_t Simulator::getCycles() const { return clockCycles; }

std::unordered_map<uint32_t, uint8_t> Simulator::getDataMap() const { return dataMap; }

std::map<uint32_t, std::pair<uint32_t, std::string>> Simulator::getTextMap() const { return textMap; }

Stage Simulator::getCurrentStage() const { return currentInstruction.stage; }

bool Simulator::isRunning() const { return running; }

InstructionRegisters Simulator::getPipelineRegisters() const { return instructionRegisters; }

uint32_t Simulator::getInstruction() const { return currentInstruction.instruction; };

#endif