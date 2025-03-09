#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
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

map<string, string> rOpcode,iOpcode, sOpcode, sbOpcode, uOpcode, ujOpcode;
map<string, string> rFunct3, iFunct3, sFunct3, sbFunct3;
map<string, string> rFunct7;

void DefineCodes() {
    rOpcode["add"] = "0110011";
    rOpcode["and"] = "0110011";
    rOpcode["or"] = "0110011";
    rOpcode["sll"] = "0110011";
    rOpcode["slt"] = "0110011";
    rOpcode["sra"] = "0110011";
    rOpcode["srl"] = "0110011";
    rOpcode["sub"] = "0110011";
    rOpcode["xor"] = "0110011";
    rOpcode["mul"] = "0110011";
    rOpcode["div"] = "0110011";
    rOpcode["rem"] = "0110011";

    rFunct7["add"] = "0000000";
    rFunct7["and"] = "0000000";
    rFunct7["or"] = "0000000";
    rFunct7["sll"] = "0000000";
    rFunct7["slt"] = "0000000";
    rFunct7["sra"] = "0100000";
    rFunct7["srl"] = "0000000";
    rFunct7["sub"] = "0100000";
    rFunct7["xor"] = "0000000";
    rFunct7["mul"] = "0000001";
    rFunct7["div"] = "0000001";
    rFunct7["rem"] = "0000001";

    rFunct3["add"] = "000";
    rFunct3["and"] = "111";
    rFunct3["or"] = "110";
    rFunct3["sll"] = "001";
    rFunct3["slt"] = "010";
    rFunct3["sra"] = "101";
    rFunct3["srl"] = "101";
    rFunct3["sub"] = "000";
    rFunct3["xor"] = "100";
    rFunct3["mul"] = "000";
    rFunct3["div"] = "100";
    rFunct3["rem"] = "110";

    iOpcode["addi"] = "0010011";
    iOpcode["andi"] = "0010011";
    iOpcode["ori"] = "0010011";
    iOpcode["lb"] = "0000011";
    iOpcode["ld"] = "0000011";
    iOpcode["lh"] = "0000011";
    iOpcode["lw"] = "0000011";
    iOpcode["jalr"] = "1100111";

    iFunct3["addi"] = "000";
    iFunct3["andi"] = "111";
    iFunct3["ori"] = "110";
    iFunct3["lb"] = "000";
    iFunct3["ld"] = "011";
    iFunct3["lh"] = "001";
    iFunct3["lw"] = "010";
    iFunct3["jalr"] = "000";

    sOpcode["sb"] = "0100011";
    sOpcode["sh"] = "0100011";
    sOpcode["sw"] = "0100011";
    sOpcode["sd"] = "0100011";

    sFunct3["sb"] = "000";
    sFunct3["sh"] = "001";
    sFunct3["sw"] = "010";
    sFunct3["sd"] = "011";

    sbOpcode["beq"] = "1100011";
    sbOpcode["bne"] = "1100011";
    sbOpcode["blt"] = "1100011";
    sbOpcode["bge"] = "1100011";

    sbFunct3["beq"] = "000";
    sbFunct3["bne"] = "001";
    sbFunct3["blt"] = "100";
    sbFunct3["bge"] = "101";

    uOpcode["auipc"] = "0010111";
    uOpcode["lui"] = "0110111";

    ujOpcode["jal"] = "1101111";
}

ifstream fin;
ofstream fout;

map<string, Format> instructionFormatMapping;
string instruction;
vector<string> tokens;
string machineCode;
vector<string> machineCodeDivision(7);

int current_address;
map<string, int> label_address;

int GetDecimalNumber(const string &s) {
    bool is_hex = (s.substr(0, 2) == "0x");
    if (is_hex) return stoi(s, nullptr, 16);
    for (char digit : s) {
        if (!isdigit(digit)) {
            cerr << "Invalid Number";
            return (int) nan;
        }
    }
    return stoi(s);
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

string DecimalToBinary(int decimalNumber, int length = -1) {
    if (!decimalNumber) return "0";

    string binary = "";
    while (decimalNumber) {
        binary = ((decimalNumber % 2 == 0) ? "0" : "1") + binary;
        decimalNumber /= 2;
    }

    while (binary.size() < length) binary = "0" + binary;
    return binary;
}

string BinaryToHex(string binaryNumber) {
    binaryNumber = string(binaryNumber.length() % 4 ? 4 - binaryNumber.length() % 4 : 0, '0') + binaryNumber;
    map<string, char> hex_dict = {
        {"0000", '0'},
        {"0001", '1'},
        {"0010", '2'},
        {"0011", '3'}, 
        {"0100", '4'},
        {"0101", '5'},
        {"0110", '6'},
        {"0111", '7'},
        {"1000", '8'},
        {"1001", '9'},
        {"1010", 'A'},
        {"1011", 'B'},
        {"1100", 'C'},
        {"1101", 'D'},
        {"1110", 'E'},
        {"1111", 'F'}
    };
    string hexadecimal;
    for (size_t i = 0; i < binaryNumber.length(); i += 4) {
        string group = binaryNumber.substr(i, 4);
        hexadecimal += hex_dict[group];
    }
    return hexadecimal;
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

vector<string> Tokenize(bool skip_label = false) {
    stringstream instructionStream;
    vector<string> currTokens;
    string currToken;

    if (instruction.find("#") != -1) instruction = instruction.substr(0, instruction.find("#"));
    instructionStream << instruction;
    
    while (instructionStream >> currToken) {
        if (!skip_label && currToken.find(":") != -1) continue;
        currTokens.push_back(currToken);
    }

    return tokens = currTokens;
}

void ExtractLabelAddresses() {
    int current_address = TEXT_ADDRESS;
    int text_mode = 1;
    while (getline(fin, instruction)) {
        if (instruction == ".data") {
            current_address = DATA_ADDRESS;
            text_mode = 0;
            continue;
        } else if (instruction == ".text") {
            current_address = TEXT_ADDRESS;
            text_mode = 1;
            continue;
        }

        Tokenize(true);
        if (tokens.empty()) continue;

        bool has_label = false;
        if (instruction.find(":") != -1) {
            has_label = true;

            string label_name = instruction.substr(0, instruction.find(":"));
            if (label_name.find(" ") != -1) {
                cerr << "Invalid Label name!" << endl;
                continue;
            }
            label_address[label_name] = current_address;
            tokens.erase(tokens.begin());
        }

        if (text_mode == 1) {
            if (tokens.empty()) continue;
            current_address += 4;
        } else {
            string data_directive = tokens[0];
            tokens.erase(tokens.begin());
            if (data_directive == ".byte") {
                if (!IsValidDirectiveData(data_directive, 1)) return;
                current_address += 1 * (tokens.size() - 1);
            } else if (data_directive == ".half") {
                if (!IsValidDirectiveData(data_directive, 2)) return;
                current_address += 2 * (tokens.size() - 1);
            } else if (data_directive == ".word") {
                if (!IsValidDirectiveData(data_directive, 4)) return;
                current_address += 4 * (tokens.size() - 1);
            } else if (data_directive == ".double") {
                if (!IsValidDirectiveData(data_directive, 8)) return;
                current_address += 8 * (tokens.size() - 1);
            } else if (data_directive == ".asciiz") {
                if (!(tokens[0].find("\"") != -1 && tokens[0].rfind("\"") != -1)) {
                    cerr << "Invalid string of characters!" << endl;
                    return;
                }
                current_address += 1 * (tokens[0].rfind("\"") - tokens[0].find("\"") - 1) + 1;
            } else {
                cerr << "Invalid direcitve: " << data_directive << endl;
                return;
            }
        }
    }

    fin.clear();
    fin.seekg(0);
}

void R_FormatDivision() {
    if (tokens.size() != 4) {
        cerr << "Invalid instruction for a R-Format operation" << endl;
        return;
    }

    string opcode = rOpcode[tokens[0]];
    string rd = tokens[1];
    string funct3 = rFunct3[tokens[0]];
    string rs1 = tokens[2];
    string rs2 = tokens[3];
    string funct7 = rFunct7[tokens[0]];

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

    string opcode = iOpcode[tokens[0]];
    string rd = tokens[1];
    string funct3 = iFunct3[tokens[0]];
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

    string opcode = sOpcode[tokens[0]];
    string funct3 = sFunct3[tokens[0]];
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

// Convert labels into relative addressing
void SB_FormatDivision() {
    if (tokens.size() != 4) {
        cerr << "Invalid instruction for a SB-Format operation" << endl;
        return;
    }

    int offset = label_address[tokens[3]] - current_address;

    string opcode = sbOpcode[tokens[0]];
    string funct3 = sbFunct3[tokens[0]];
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

    string opcode = uOpcode[tokens[0]];
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

    string opcode = ujOpcode[tokens[0]];
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

void ProcessCode() {
    if (tokens.empty()) return;

    ExtractMachineCode();

    fout << "0x" << setw(8) << setfill('0') << hex << current_address << " ";
    fout << "0x" << setw(8) << setfill('0') << BinaryToHex(machineCode) << "  ";

    for (int i = 0; i < tokens.size(); i++) {
        if (label_address[tokens[i]]) {
            int offset = label_address[tokens[i]] - current_address;
            fout << dec << offset;
        } else fout << tokens[i];

        if (i == 0 || i == tokens.size() - 1) fout << " ";
        else fout << ", ";
    }

    fout << " # ";
    for (int i = 0; i < 7; i++) {
        fout << machineCodeDivision[i];
        if (i != 6) fout << " - ";
    }

    fout << endl;
    current_address = current_address + 4;
}

void ProcessData() {
    if (tokens.empty()) return;
    map<string, int> address_size = {
        {".byte", 1},
        {".half", 2},
        {".word", 4},
        {".double", 8}
    };
    
    if (address_size[tokens[0]]) {
        for (int i = 1; i < tokens.size(); i++) {
            stringstream token_value;
            token_value << hex << GetDecimalNumber(tokens[i]);
            fout << "0x" << setw(8) << setfill('0') << hex << current_address << " ";
            fout << "0x" << setw(2 * address_size[tokens[0]]) << setfill('0') << token_value.str() << endl;
            
            current_address += address_size[tokens[0]];
        }
    } else if (tokens[0] == ".asciiz") {
        int string_length = tokens[1].length() - 1;
        tokens[1][string_length] = '\0';
        for (int i = 1; i <= string_length; i++) {
            fout << "0x" << setw(8) << setfill('0') << hex << current_address << " ";
            fout << "0x" << setw(2) << setfill('0') << hex << (int) tokens[1][i] << endl;
            current_address += 1;
        }
        tokens[1][string_length] = '\"';
    }
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
    DefineCodes();

    ExtractLabelAddresses();

    int text_mode = 1;
    current_address = TEXT_ADDRESS;
    while (getline(fin, instruction)) {
        if (Tokenize().empty()) continue;

        if (tokens[0] == ".data") {
            text_mode = 0;
            current_address = DATA_ADDRESS;
            fout << ".data" << endl;
            continue;
        } else if (tokens[0] == ".text") {
            text_mode = 1;
            current_address = TEXT_ADDRESS;
            fout << ".text" << endl;
            continue;
        }

        if (text_mode) ProcessCode();
        else ProcessData();
    }

    return 0;
}