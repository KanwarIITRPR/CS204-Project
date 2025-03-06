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

vector<string> ExtractMachineCode() {
    switch (instructionFormatMapping[tokens[0]]) {
        case R:
            R_FormatDivision();
            break;
        case I:
            I_FormatDivision();
            break;
        case S:
            break;
        case SB:
            break;
        case U:
            break;
        case UJ:
            break;
        default:
            cerr << "Error: Unidentified instruction!";
            break;
    }
    return machineCodeDivision;
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