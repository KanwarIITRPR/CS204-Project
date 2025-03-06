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
    string opcode = r_opcode[tokens[0]];
    string rd = tokens[1];
    string funct3 = r_func3[tokens[0]];
    string rs1 = tokens[2];
    string rs2 = tokens[3];
    string funct7 = r_func7[tokens[0]];

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

// Convert offset(register) to valid arguments for load
void I_FormatDivision() {
    string opcode = i_opcode[tokens[0]];
    string rd = tokens[1];
    string funct3 = i_func3[tokens[0]];
    string rs1 = tokens[2];
    string immediate = tokens[3];
    
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

// Convert offset(register) to valid arguments
void S_FormatDivision() {
    string opcode = s_opcode[tokens[0]];
    string funct3 = s_func3[tokens[0]];
    string rs1 = tokens[1];
    string rs2 = tokens[2];
    string immediate = tokens[3];
    
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
    string opcode = sb_opcode[tokens[0]];
    string funct3 = sb_func3[tokens[0]];
    string rs1 = tokens[1];
    string rs2 = tokens[2];
    string immediate = tokens[3];
    
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

// Check for bounds
void U_FormatDivision() {
    string opcode = u_opcode[tokens[0]];
    string rd = tokens[1];
    string immediate = tokens[2];
    
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
    string opcode = uj_opcode[tokens[0]];
    string rd = tokens[1];
    string immediate = tokens[2];
    
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

// Check for register bounds
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
    NextInstruction();
    Tokenize();
    NextInstruction();
    Tokenize();



    return 0;
}