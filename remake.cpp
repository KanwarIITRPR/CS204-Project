#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include "codes.cpp"
using namespace std;

enum Format {
    R,
    I,
    S,
    SB,
    U,
    UJ
};

ifstream fin;
ofstream fout;

map<string, Format> instructionFormatMapping;
string instruction;
vector<string> tokens;
string machineCode;
vector<string> machineCodeDivision(7);

void NextInstruction() { getline(fin, instruction); }
bool isNumber(const string &s) {
    return !s.empty() && (isdigit(s[0]) || s[0] == '-' || s[0] == '+');
}

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

vector<string> offsetRegisterCommands = {"lb", "lh", "lw", "ld", "jalr", "sb", "sh", "sw", "sd"};
bool IsOffsetRegisterAllowed() {
    for (string command : offsetRegisterCommands) {
        if (command == tokens[0]) return true;
    }
    return false;
}

// string::npos is the value return by "string" functions when they fail
bool IsOffsetRegisterCombination() { return !(tokens[2].find('(') == string::npos || tokens[2].find(')') == string::npos); }

void StandardizeOffsetRegisterCombination() {
    int openParenthesisIndex = tokens[2].find('(');
    int closeParenthesisIndex = tokens[2].find(')');

    tokens.push_back(tokens[2].substr(0, openParenthesisIndex));
    tokens[2] = tokens[2].substr(openParenthesisIndex + 1, closeParenthesisIndex - openParenthesisIndex - 1);
}

// Identify Labels
// Call NextInstruction() before calling tokenize
vector<string> Tokenize() {
    stringstream instructionStream(instruction);
    vector<string> currTokens;
    string currToken;
    
    while (instructionStream >> currToken) {
        if (currToken[0] == '#') break;
        currTokens.push_back(currToken);
    }

    return tokens = currTokens;
}


void R_FormatDivision() {
    if (tokens.size() != 4) {
        cerr << "Invalid instruction for a R-Format operation" << endl;
        return;
    }

    string opcode = r_opcode[tokens[0]];
    string rd = tokens[1];
    string funct3 = r_func3[tokens[0]];
    string rs1 = tokens[2];
    string rs2 = tokens[3];
    string funct7 = r_func7[tokens[0]];

    if (!(IsValidRegister(rd) && IsValidRegister(rs1) && IsValidRegister(rs2))) {
        cerr << "Invalid register name!" << endl;
        return;
    }

    rd.erase(0, 1);
    rs1.erase(0, 1);
    rs2.erase(0, 1);
    
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
    string funct3 = i_func3[tokens[0]];
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
    string funct3 = s_func3[tokens[0]];
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

// Convert labels into relative addressing
void SB_FormatDivision() {
    if (tokens.size() != 4) {
        cerr << "Invalid instruction for a SB-Format operation" << endl;
        return;
    }

    string opcode = sb_opcode[tokens[0]];
    string funct3 = sb_func3[tokens[0]];
    string rs1 = tokens[1];
    string rs2 = tokens[2];
    string immediate = tokens[3];

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

    string opcode = uj_opcode[tokens[0]];
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

// Convert the registers and immediate values to hexadecimal format
vector<string> ExtractMachineCode() {
    switch (instructionFormatMapping[tokens[0]]) {
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

void InitializeInstructions() {
    instructionFormatMapping["add"] = R;
    instructionFormatMapping["sub"] = R;
    instructionFormatMapping["mul"] = R;
    instructionFormatMapping["div"] = R;
    instructionFormatMapping["xor"] = R;
    instructionFormatMapping["rem"] = R;
    instructionFormatMapping["and"] = R;
    instructionFormatMapping["or"] = R;
    instructionFormatMapping["sll"] = R;
    instructionFormatMapping["slt"] = R;
    instructionFormatMapping["sra"] = R;
    instructionFormatMapping["srl"] = R;
    
    instructionFormatMapping["addi"] = I;
    instructionFormatMapping["andi"] = I;
    instructionFormatMapping["ori"] = I;
    instructionFormatMapping["lb"] = I;
    instructionFormatMapping["lh"] = I;
    instructionFormatMapping["lw"] = I;
    instructionFormatMapping["ld"] = I;
    instructionFormatMapping["jalr"] = I;

    instructionFormatMapping["sb"] = S;
    instructionFormatMapping["sh"] = S;
    instructionFormatMapping["sw"] = S;
    instructionFormatMapping["sd"] = S;
    
    instructionFormatMapping["beq"] = SB;
    instructionFormatMapping["bne"] = SB;
    instructionFormatMapping["bge"] = SB;
    instructionFormatMapping["blt"] = SB;

    instructionFormatMapping["lui"] = U;
    instructionFormatMapping["auipc"] = U;
    
    instructionFormatMapping["jal"] = UJ;
}

int main(int argC, char* argV[]) {
    if (argC < 3) {
        cerr << "Usage: " << argV[0] << " <input.asm> <output.mc>" << endl;
        return 1;
    }

    fin.open(argV[1]);
    fout.open(argV[2]);
    if (!fin.is_open() || !fout.is_open()) {
        cerr << "Error: Unable to open files" << endl;
        return 1;
    }

    InitializeInstructions();

    // Merge codes for Load with I-type
    defineAllCodes();
    
    NextInstruction();
    Tokenize();
    NextInstruction();
    Tokenize();
    // NextInstruction();
    // Tokenize();
    // NextInstruction();
    // Tokenize();
    StandardizeOffsetRegisterCombination();
    for (string token : tokens) {
        cout << token << endl;
    }



    return 0;
}