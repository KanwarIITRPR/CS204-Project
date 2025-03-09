#include <map>
#include <bits/stdc++.h>
#include <algorithm>
#include "utils.cpp"
using namespace std;

#define TEXT_ADDRESS 0x00000000
#define DATA_ADDRESS 0x10000000
#define STACK_ADDRESS 0x7FFFFFDC
#define HEAP_ADDRESS 0x10008000

enum Format {
    R,
    I,
    S,
    SB,
    U,
    UJ
};

map<string, Format> instruction_format_mapping;
map<string, string> r_opcode,i_opcode, s_opcode, sb_opcode, u_opcode, uj_opcode;
map<string, string> r_funct3, i_funct3, s_funct3, sb_funct3;
map<string, string> r_funct7;

extern string instruction;
extern vector<string> tokens;
extern string machineCode;
extern vector<string> machineCodeDivision;

extern int current_address;
extern map<string, int> label_address;

void InitializeInstructions() {
    instruction_format_mapping["add"] = R;
    instruction_format_mapping["sub"] = R;
    instruction_format_mapping["mul"] = R;
    instruction_format_mapping["div"] = R;
    instruction_format_mapping["xor"] = R;
    instruction_format_mapping["rem"] = R;
    instruction_format_mapping["and"] = R;
    instruction_format_mapping["or"] = R;
    instruction_format_mapping["sll"] = R;
    instruction_format_mapping["slt"] = R;
    instruction_format_mapping["sra"] = R;
    instruction_format_mapping["srl"] = R;
    
    instruction_format_mapping["addi"] = I;
    instruction_format_mapping["andi"] = I;
    instruction_format_mapping["ori"] = I;
    instruction_format_mapping["lb"] = I;
    instruction_format_mapping["lh"] = I;
    instruction_format_mapping["lw"] = I;
    instruction_format_mapping["ld"] = I;
    instruction_format_mapping["jalr"] = I;

    instruction_format_mapping["sb"] = S;
    instruction_format_mapping["sh"] = S;
    instruction_format_mapping["sw"] = S;
    instruction_format_mapping["sd"] = S;
    
    instruction_format_mapping["beq"] = SB;
    instruction_format_mapping["bne"] = SB;
    instruction_format_mapping["bge"] = SB;
    instruction_format_mapping["blt"] = SB;

    instruction_format_mapping["lui"] = U;
    instruction_format_mapping["auipc"] = U;
    
    instruction_format_mapping["jal"] = UJ;
}

void DefineCodes() {
    r_opcode["add"] = "0110011";
    r_opcode["and"] = "0110011";
    r_opcode["or"] = "0110011";
    r_opcode["sll"] = "0110011";
    r_opcode["slt"] = "0110011";
    r_opcode["sra"] = "0110011";
    r_opcode["srl"] = "0110011";
    r_opcode["sub"] = "0110011";
    r_opcode["xor"] = "0110011";
    r_opcode["mul"] = "0110011";
    r_opcode["div"] = "0110011";
    r_opcode["rem"] = "0110011";

    r_funct7["add"] = "0000000";
    r_funct7["and"] = "0000000";
    r_funct7["or"] = "0000000";
    r_funct7["sll"] = "0000000";
    r_funct7["slt"] = "0000000";
    r_funct7["sra"] = "0100000";
    r_funct7["srl"] = "0000000";
    r_funct7["sub"] = "0100000";
    r_funct7["xor"] = "0000000";
    r_funct7["mul"] = "0000001";
    r_funct7["div"] = "0000001";
    r_funct7["rem"] = "0000001";

    r_funct3["add"] = "000";
    r_funct3["and"] = "111";
    r_funct3["or"] = "110";
    r_funct3["sll"] = "001";
    r_funct3["slt"] = "010";
    r_funct3["sra"] = "101";
    r_funct3["srl"] = "101";
    r_funct3["sub"] = "000";
    r_funct3["xor"] = "100";
    r_funct3["mul"] = "000";
    r_funct3["div"] = "100";
    r_funct3["rem"] = "110";

    i_opcode["addi"] = "0010011";
    i_opcode["andi"] = "0010011";
    i_opcode["ori"] = "0010011";
    i_opcode["lb"] = "0000011";
    i_opcode["ld"] = "0000011";
    i_opcode["lh"] = "0000011";
    i_opcode["lw"] = "0000011";
    i_opcode["jalr"] = "1100111";

    i_funct3["addi"] = "000";
    i_funct3["andi"] = "111";
    i_funct3["ori"] = "110";
    i_funct3["lb"] = "000";
    i_funct3["ld"] = "011";
    i_funct3["lh"] = "001";
    i_funct3["lw"] = "010";
    i_funct3["jalr"] = "000";

    s_opcode["sb"] = "0100011";
    s_opcode["sh"] = "0100011";
    s_opcode["sw"] = "0100011";
    s_opcode["sd"] = "0100011";

    s_funct3["sb"] = "000";
    s_funct3["sh"] = "001";
    s_funct3["sw"] = "010";
    s_funct3["sd"] = "011";

    sb_opcode["beq"] = "1100011";
    sb_opcode["bne"] = "1100011";
    sb_opcode["blt"] = "1100011";
    sb_opcode["bge"] = "1100011";

    sb_funct3["beq"] = "000";
    sb_funct3["bne"] = "001";
    sb_funct3["blt"] = "100";
    sb_funct3["bge"] = "101";

    u_opcode["auipc"] = "0010111";
    u_opcode["lui"] = "0110111";

    uj_opcode["jal"] = "1101111";
}


vector<string> offsetRegisterCommands = {"lb", "lh", "lw", "ld", "jalr", "sb", "sh", "sw", "sd"};
bool IsValidRegister(const string &reg) {
    // Registers need to start with "x<regID>" where regID is the location/ID of the register, a number
    if (reg[0] != 'x') return false;
    if (!isdigit(reg[1])) return false;
    string regID = reg.substr(1, reg.length() - 1);

    // Register values can't contain additional 0s in the start, i.e., x01 is invalid
    if (stoi(regID) != 0 && regID[0] == '0') return false;
    // Register location/ID should be within the range x0-x31
    if (!(stoi(regID) >= 0 && stoi(regID) <= 31)) return false;
    return true;
}

bool IsOffsetRegisterAllowed() {
    for (string command : offsetRegisterCommands) {
        if (command == tokens[0]) return true;
    }
    return false;
}

bool IsOffsetRegisterCombination() { return !(tokens[2].find('(') == -1 || tokens[2].find(')') == -1); }

void StandardizeOffsetRegisterCombination() {
    int openParenthesisIndex = tokens[2].find('(');
    int closeParenthesisIndex = tokens[2].find(')');

    tokens.push_back(tokens[2].substr(0, openParenthesisIndex));
    tokens[2] = tokens[2].substr(openParenthesisIndex + 1, closeParenthesisIndex - openParenthesisIndex - 1);
}

bool IsValidDirectiveData(const string &data_directive, int bytes) {
    for (string data : tokens) {
        long long data_value = (data.substr(0, 2) == "0x") ? stoll(data, nullptr, 16) : stoll(data);
        if (!(data_value <= pow(2, 8 * bytes) - 1 && data_value >= 0)) {
            cerr << "Value of " << data << " exceeds " << data_directive << " size" << endl;
            return false;
        }
    }
    return true;
}


void R_FormatDivision() {
    if (tokens.size() != 4) {
        cerr << "Invalid instruction for a R-Format operation" << endl;
        return;
    }

    string opcode = r_opcode[tokens[0]];
    string rd = tokens[1];
    string funct3 = r_funct3[tokens[0]];
    string rs1 = tokens[2];
    string rs2 = tokens[3];
    string funct7 = r_funct7[tokens[0]];

    if (!(IsValidRegister(rd) && IsValidRegister(rs1) && IsValidRegister(rs2))) {
        cerr << "Invalid register name!" << endl;
        return;
    }

    rd.erase(0, 1);
    rs1.erase(0, 1);
    rs2.erase(0, 1);

    rd = DecimalToBinary(stoi(rd), 5);
    rs1 = DecimalToBinary(stoi(rs1), 5);
    rs2 = DecimalToBinary(stoi(rs2), 5);
    
    machineCodeDivision[0] = opcode;
    machineCodeDivision[1] = funct3;
    machineCodeDivision[2] = funct7;
    machineCodeDivision[3] = rd;
    machineCodeDivision[4] = rs1;
    machineCodeDivision[5] = rs2;
    machineCodeDivision[6] = "NULL";
    
    machineCode = funct7 + rs2 + rs1 + funct3 + rd + opcode;
}

void I_FormatDivision() {
    if (tokens.size() == 3) {
        if (IsOffsetRegisterAllowed() && IsOffsetRegisterCombination()) StandardizeOffsetRegisterCombination();
        else {
            cerr << "Expected offset-register value" << endl;
            return;
        }
    } else if (tokens.size() != 4) {
        cerr << "Invalid Instruction for I-Format operation" << endl;
        return;
    }

    string opcode = i_opcode[tokens[0]];
    string rd = tokens[1];
    string funct3 = i_funct3[tokens[0]];
    string rs1 = tokens[2];
    string immediate = tokens[3];

    if (!(IsValidRegister(rd) && IsValidRegister(rs1))) {
        cerr << "Invalid register name!" << endl;
        return;
    }

    if (!(stoi(immediate) >= -2048 && stoi(immediate) <= 2047)) {
        cerr << "Immediate value out of range!" << endl;
        return;
    }
    
    rd.erase(0, 1);
    rs1.erase(0, 1);

    rd = DecimalToBinary(stoi(rd), 5);
    rs1 = DecimalToBinary(stoi(rs1), 5);
    immediate = DecimalToBinary(stoi(immediate), 12);
    
    machineCodeDivision[0] = opcode;
    machineCodeDivision[1] = funct3;
    machineCodeDivision[2] = "NULL";
    machineCodeDivision[3] = rd;
    machineCodeDivision[4] = rs1;
    machineCodeDivision[5] = "NULL";
    machineCodeDivision[6] = immediate;
    
    machineCode = immediate + rs1 + funct3 + rd + opcode;
}

void S_FormatDivision() {
    if (tokens.size() != 3) {
        cerr << "Invalid Instruction for I-Format operation" << endl;
        return;
    } else if (!IsOffsetRegisterCombination()) {
        cerr << "Expected offset-register value" << endl;
        return;
    }

    StandardizeOffsetRegisterCombination();

    string opcode = s_opcode[tokens[0]];
    string funct3 = s_funct3[tokens[0]];
    string rs1 = tokens[1];
    string rs2 = tokens[2];
    string immediate = tokens[3];

    if (!(IsValidRegister(rs1) && IsValidRegister(rs2))) {
        cerr << "Invalid register name!" << endl;
        return;
    }

    if (!(stoi(immediate) >= -2048 && stoi(immediate) <= 2047)) {
        cerr << "Immediate value out of range!" << endl;
        return;
    }
    
    rs1.erase(0, 1);
    rs2.erase(0, 1);

    rs1 = DecimalToBinary(stoi(rs1), 5);
    rs2 = DecimalToBinary(stoi(rs2), 5);
    immediate = DecimalToBinary(stoi(immediate), 12);

    string lowerImmediate = immediate.substr(7, 5);
    string upperImmediate = immediate.substr(0, 7);
    
    machineCodeDivision[0] = opcode;
    machineCodeDivision[1] = funct3;
    machineCodeDivision[2] = "NULL";
    machineCodeDivision[3] = "NULL";
    machineCodeDivision[4] = rs1;
    machineCodeDivision[5] = rs2;
    machineCodeDivision[6] = immediate;
    
    machineCode = upperImmediate + rs2 + rs1 + funct3 + lowerImmediate + opcode;
}

void SB_FormatDivision() {
    if (tokens.size() != 4) {
        cerr << "Invalid instruction for a SB-Format operation" << endl;
        return;
    }

    int offset = label_address[tokens[3]] - current_address;

    string opcode = sb_opcode[tokens[0]];
    string funct3 = sb_funct3[tokens[0]];
    string rs1 = tokens[1];
    string rs2 = tokens[2];
    stringstream immediateValue;
    immediateValue << offset;
    string immediate = immediateValue.str();

    if (!(IsValidRegister(rs1) && IsValidRegister(rs2))) {
        cerr << "Invalid register name!" << endl;
        return;
    }

    if (!(stoi(immediate) >= -4096 && stoi(immediate) <= 4095)) {
        cerr << "Immediate value out of range!" << endl;
        return;
    }
    
    rs1.erase(0, 1);
    rs2.erase(0, 1);

    rs1 = DecimalToBinary(stoi(rs1), 5);
    rs2 = DecimalToBinary(stoi(rs2), 5);
    immediate = immediate.substr(0, 12);
    immediate = DecimalToBinary(stoi(immediate), 12);
    
    string lowerImmediate = immediate.substr(8, 4) + immediate[1];
    string upperImmediate = immediate[0] + immediate.substr(2, 6);
    
    machineCodeDivision[0] = opcode;
    machineCodeDivision[1] = funct3;
    machineCodeDivision[2] = "NULL";
    machineCodeDivision[3] = "NULL";
    machineCodeDivision[4] = rs1;
    machineCodeDivision[5] = rs2;
    machineCodeDivision[6] = immediate;
    
    machineCode = upperImmediate + rs2 + rs1 + funct3 + lowerImmediate  + opcode;
}

void U_FormatDivision() {
    if (tokens.size() != 3) {
        cerr << "Invalid instruction for a U-Format operation" << endl;
        return;
    }

    string opcode = u_opcode[tokens[0]];
    string rd = tokens[1];
    string immediate = tokens[2];

    if (!IsValidRegister(rd)) {
        cerr << "Invalid register name!" << endl;
        return;
    }

    if (!(stoi(immediate) >= -pow(2, 20) && stoi(immediate) <= pow(2, 20) - 1)) {
        cerr << "Immediate value out of range!" << endl;
        return;
    }
    
    rd.erase(0, 1);

    rd = DecimalToBinary(stoi(rd), 5);
    immediate = DecimalToBinary(stoi(immediate), 20);
    
    machineCodeDivision[0] = opcode;
    machineCodeDivision[1] = "NULL";
    machineCodeDivision[2] = "NULL";
    machineCodeDivision[3] = rd;
    machineCodeDivision[4] = "NULL";
    machineCodeDivision[5] = "NULL";
    machineCodeDivision[6] = immediate;
    
    machineCode = immediate + rd + opcode;
}

void UJ_FormatDivision() {
    if (tokens.size() != 3) {
        cerr << "Invalid instruction for a UJ-Format operation" << endl;
        return;
    }

    int offset = label_address[tokens[3]] - current_address;

    string opcode = uj_opcode[tokens[0]];
    string rd = tokens[1];
    stringstream immediateValue;
    immediateValue << offset;
    string immediate = immediateValue.str();

    if (!IsValidRegister(rd)) {
        cerr << "Invalid register name!" << endl;
        return;
    }

    if (!(stoi(immediate) >= -pow(2, 21) && stoi(immediate) <= pow(2, 21) - 1)) {
        cerr << "Immediate value out of range!" << endl;
        return;
    }
    
    rd.erase(0, 1);

    rd = DecimalToBinary(stoi(rd), 5);
    immediate = immediate.substr(0, 20);
    immediate = DecimalToBinary(stoi(immediate), 20);

    string adjustedImmediate = immediate[0] + immediate.substr(10, 10) + immediate[9] + immediate.substr(1, 8);
    
    machineCodeDivision[0] = opcode;
    machineCodeDivision[1] = "NULL";
    machineCodeDivision[2] = "NULL";
    machineCodeDivision[3] = rd;
    machineCodeDivision[4] = "NULL";
    machineCodeDivision[5] = "NULL";
    machineCodeDivision[6] = immediate;
    
    machineCode = adjustedImmediate + rd + opcode;
}

vector<string> ExtractMachineCode() {
    switch (instruction_format_mapping[tokens[0]]) {
        case R:
            R_FormatDivision();
            break;
        case I:
            I_FormatDivision();
            break;
        case S:
            S_FormatDivision();
            break;
        case SB:
            SB_FormatDivision();
            break;
        case U:
            U_FormatDivision();
            break;
        case UJ:
            UJ_FormatDivision();
            break;
        default:
            cerr << "Error: Unidentified instruction!";
            break;
    }
    return machineCodeDivision;
}
