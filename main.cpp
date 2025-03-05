#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include "codes.cpp"
#include "depend.cpp"

using namespace std;

#define CODE_START 0x00000000
#define DATA_START 0x10000000

map<string, int> labelAddress;
int text_address = CODE_START;
int data_address = DATA_START;
int mode = 0; // 0 -> .text, 1 -> .data

bool isNumber(const string &s) {
    return !s.empty() && (isdigit(s[0]) || s[0] == '-' || s[0] == '+');
}

vector<string> tokenize(string instruct) {
    vector<string> tokens;
    stringstream ss(instruct);
    string token;
    while (ss >> token) {
        if (token[0] == '#') break; // Ignore comments
        if (token.back() == ':') {
            labelAddress[token.substr(0, token.size() - 1)] = text_address;
            continue;
        }
        tokens.push_back(token);
    }
    return tokens;
}
string generateBinaryCode(vector<string> &tokens) {
    string binary_code;

    if (r_opcode.find(tokens[0]) != r_opcode.end()) {  // R-type
        if (tokens.size() < 4) {
            cerr << "Error: Invalid R-type instruction format -> " << tokens[0] << endl;
            return "";
        }

        tokens[1].erase(0, 1);
        tokens[2].erase(0, 1);
        tokens[3].erase(0, 1);

        if (!isNumber(tokens[1]) || !isNumber(tokens[2]) || !isNumber(tokens[3])) {
            cerr << "Error: Invalid register number in R-type instruction -> " << tokens[0] << endl;
            return "";
        }

        string rd = dec_to_bin(stoi(tokens[1]), 5);
        string rs1 = dec_to_bin(stoi(tokens[2]), 5);
        string rs2 = dec_to_bin(stoi(tokens[3]), 5);
        string func7 = r_func7[tokens[0]];
        string func3 = r_func3[tokens[0]];
        string opcode = r_opcode[tokens[0]];

        binary_code = func7 + rs2 + rs1 + func3 + rd + opcode;
    }
    // Handle Load Instructions (I-type)
else if (load_opcode.find(tokens[0]) != load_opcode.end()) {  
    if (tokens.size() < 3) {
        cerr << "Error: Invalid load instruction format -> " << tokens[0] << endl;
        return "";
    }

    tokens[1].erase(0, 1); // Remove 'x' from register

    // Extract immediate and base register from "imm(rs1)" format
    size_t openParen = tokens[2].find('(');
    size_t closeParen = tokens[2].find(')');
    if (openParen == string::npos || closeParen == string::npos) {
        cerr << "Error: Invalid format for load instruction -> " << tokens[0] << endl;
        return "";
    }

    string imm_str = tokens[2].substr(0, openParen);  // Immediate value
    string rs1_str = tokens[2].substr(openParen + 2, closeParen - openParen - 2); // Register number (skip 'x')

    if (!isNumber(tokens[1]) || !isNumber(rs1_str) || !isNumber(imm_str)) {
        cerr << "Error: Invalid register or immediate value in load instruction -> " << tokens[0] << endl;
        return "";
    }

    int imm_value = stoi(imm_str);

    // Ensure immediate value is within 12-bit signed range
    if (imm_value < -2048 || imm_value > 2047) {
        cerr << "Error: Immediate value out of range for load instruction -> " << tokens[0] << endl;
        return "";
    }

    string rd = dec_to_bin(stoi(tokens[1]), 5);
    string rs1 = dec_to_bin(stoi(rs1_str), 5);
    string imm = dec_to_bin(imm_value, 12);
    string func3 = load_func3[tokens[0]];
    string opcode = load_opcode[tokens[0]];

    binary_code = imm + rs1 + func3 + rd + opcode;
} 

// Handle Other I-type Instructions
else if (i_opcode.find(tokens[0]) != i_opcode.end()) {  
    if (tokens.size() < 4) {
        cerr << "Error: Invalid I-type instruction format -> " << tokens[0] << endl;
        return "";
    }

    tokens[1].erase(0, 1); // Remove 'x' from register
    tokens[2].erase(0, 1); 

    if (!isNumber(tokens[1]) || !isNumber(tokens[2])) {
        cerr << "Error: Invalid register number in I-type instruction -> " << tokens[0] << endl;
        return "";
    }

    int imm_value;
    try {
        imm_value = stoi(tokens[3]);
    } catch (...) {
        cerr << "Error: Invalid immediate value in I-type instruction -> " << tokens[0] << endl;
        return "";
    }

    // Ensure immediate value is within 12-bit signed range
    if (imm_value < -2048 || imm_value > 2047) {
        cerr << "Error: Immediate value out of range for I-type instruction -> " << tokens[0] << endl;
        return "";
    }

    string rd = dec_to_bin(stoi(tokens[1]), 5);
    string rs1 = dec_to_bin(stoi(tokens[2]), 5);
    string imm = dec_to_bin(imm_value, 12);
    string func3 = i_func3[tokens[0]];
    string opcode = i_opcode[tokens[0]];

    binary_code = imm + rs1 + func3 + rd + opcode;
}

    
    else if (s_opcode.find(tokens[0]) != s_opcode.end()) {  // S-type (Store Instructions)
        if (tokens.size() < 3) {
            cerr << "Error: Invalid S-type instruction format -> " << tokens[0] << endl;
            return "";
        }
    
        tokens[1].erase(0, 1); // Remove 'x' from rs2
    
        // Extract immediate and base register from "imm(rs1)" format
        size_t openParen = tokens[2].find('(');
        size_t closeParen = tokens[2].find(')');
        if (openParen == string::npos || closeParen == string::npos) {
            cerr << "Error: Invalid format for store instruction -> " << tokens[0] << endl;
            return "";
        }
    
        string imm_str = tokens[2].substr(0, openParen);  // Immediate value
        string rs1_str = tokens[2].substr(openParen + 2, closeParen - openParen - 2); // Register number (skip 'x')
    
        if (!isNumber(tokens[1]) || !isNumber(rs1_str) || !isNumber(imm_str)) {
            cerr << "Error: Invalid register or immediate value in store instruction -> " << tokens[0] << endl;
            return "";
        }
    
        int imm_value = stoi(imm_str);
    
        // Ensure immediate value is within 12-bit signed range
        if (imm_value < -2048 || imm_value > 2047) {
            cerr << "Error: Immediate value out of range for S-type instruction -> " << tokens[0] << endl;
            return "";
        }
    
        string rs2 = dec_to_bin(stoi(tokens[1]), 5);
        string rs1 = dec_to_bin(stoi(rs1_str), 5);
        string imm_bin = dec_to_bin(imm_value, 12);
        string imm_high = imm_bin.substr(0, 7);
        string imm_low = imm_bin.substr(7, 5);
        string func3 = s_func3[tokens[0]];
        string opcode = s_opcode[tokens[0]];
    
        binary_code = imm_high + rs2 + rs1 + func3 + imm_low + opcode;
    }
    
    else if (sb_func3.find(tokens[0]) != sb_func3.end()) {  // SB-type
        if (tokens.size() < 4) {
            cerr << "Error: Invalid SB-type instruction format -> " << tokens[0] << endl;
            return "";
        }
    
        tokens[1].erase(0, 1);
        tokens[2].erase(0, 1);
    
        if (!isNumber(tokens[1]) || !isNumber(tokens[2])) {
            cerr << "Error: Invalid register in SB-type instruction -> " << tokens[0] << endl;
            return "";
        }
    
        string rs1 = dec_to_bin(stoi(tokens[1]), 5);
        string rs2 = dec_to_bin(stoi(tokens[2]), 5);
        int offset = labelAddress[tokens[3]] - text_address;
        string imm = dec_to_bin(offset, 13);
        string imm_high = imm[0] + imm.substr(2, 6);
        string imm_low = imm.substr(8, 4) + imm[1];
        string func3 = sb_func3[tokens[0]];
    
        binary_code = imm_high + rs2 + rs1 + func3 + imm_low + "1100011";
    }
    else if (u_opcode.find(tokens[0]) != u_opcode.end()) {  // U-type
        if (tokens.size() < 3) {
            cerr << "Error: Invalid U-type instruction format -> " << tokens[0] << endl;
            return "";
        }
    
        tokens[1].erase(0, 1);
    
        if (!isNumber(tokens[1])) {
            cerr << "Error: Invalid register in U-type instruction -> " << tokens[0] << endl;
            return "";
        }
    
        string rd = dec_to_bin(stoi(tokens[1]), 5);
        string imm = dec_to_bin(stoi(tokens[2]), 20);
        string opcode = u_opcode[tokens[0]];
    
        binary_code = imm + rd + opcode;
    }
    else if (tokens[0] == "jal") {  // UJ-type
        if (tokens.size() < 3) {
            cerr << "Error: Invalid JAL instruction format -> " << tokens[0] << endl;
            return "";
        }
    
        tokens[1].erase(0, 1);
    
        if (!isNumber(tokens[1])) {
            cerr << "Error: Invalid register in JAL instruction -> " << tokens[0] << endl;
            return "";
        }
    
        string rd = dec_to_bin(stoi(tokens[1]), 5);
        int offset = labelAddress[tokens[2]] - text_address;
        string imm = dec_to_bin(offset, 21);
        string opcode = "1101111";
    
        binary_code = imm[0] + imm.substr(10, 10) + imm[9] + imm.substr(1, 8) + rd + opcode;
    }
    else {
        cerr << "Error: Unrecognized instruction -> " << tokens[0] << endl;
    }

    return binary_code;
}


void firstPass(ifstream &fin) {
    string line;
    while (getline(fin, line)) {
        vector<string> tokens = tokenize(line);
        if (tokens.empty()) continue;
        
        if (tokens[0] == ".data") { mode = 1; continue; }
        if (tokens[0] == ".text") { mode = 0; continue; }
        
        if (tokens[0].back() == ':') {
            labelAddress[tokens[0]] = (mode == 0) ? text_address : data_address;
        } else if (mode == 0) {
            text_address += 4;
        } else if (mode == 1) {
            data_address += 4;
        }
    }
    text_address = CODE_START;
    data_address = DATA_START;
    fin.clear();
    fin.seekg(0);
}

void processInstruction(ofstream &fout, vector<string> &tokens) {
    if (tokens.empty()) return;

    string binary_code = generateBinaryCode(tokens);
    if (binary_code.empty()) return;

    string cleaned_binary = binary_code;
    cleaned_binary.erase(remove(cleaned_binary.begin(), cleaned_binary.end(), '-'), cleaned_binary.end());

    string hex_code = bin_to_hex(cleaned_binary);

    fout << "0x" << setw(8) << setfill('0') << hex << text_address
         << " 0x" << setw(8) << setfill('0') << hex << hex_code
         << " , " << tokens[0];

    for (size_t i = 1; i < tokens.size(); i++) fout << " " << tokens[i];
    fout << " # " << binary_code << endl;
    text_address += 4;
}

void processDataSegment(ofstream &fout, vector<string> &tokens, int &data_address, const string &line) {
    unordered_map<string, int> sizeMap = {
        {".word", 4}, {".half", 2}, {".byte", 1}, {".dword", 8}
    };

    if (sizeMap.find(tokens[1]) != sizeMap.end()) {
        int increment = sizeMap[tokens[1]];
        size_t numValues = tokens.size() - 2;
        if (numValues == 0) numValues = 1;

        for (size_t i = 2; i < tokens.size(); i++) {
            try {
                int value = stoi(tokens[i]);
                fout << "0x" << setw(8) << setfill('0') << hex << data_address
                     << " 0x" << setw(8) << setfill('0') << dec << value << endl;
            } catch (invalid_argument &) {
                cerr << "Invalid numeric data: " << tokens[i] << endl;
                fout << "0x" << setw(8) << setfill('0') << hex << data_address << " 0x00000000" << endl;
            }
            data_address += increment;
        }

        if (tokens.size() == 2) {
            fout << "0x" << setw(8) << setfill('0') << hex << data_address << " 0x00000000" << endl;
            data_address += increment;
        }
    } 
    else if (tokens[1] == ".asciiz") {
        string str = line.substr(line.find('"') + 1, line.rfind('"') - line.find('"') - 1);
        for (size_t i = 0; i < str.length(); i++) {
            fout << "0x" << setw(8) << setfill('0') << hex << data_address 
                 << " 0x" << setw(2) << setfill('0') << hex << int(str[i]) 
                 << " " << dec << int(str[i]) << endl;
            data_address += 1;
        }
        fout << "0x" << setw(8) << setfill('0') << hex << data_address 
             << " 0x00 0" << endl;
        data_address += 1;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input.asm> <output.mc>" << endl;
        return 1;
    }

    defineAllCodes();

    ifstream fin(argv[1]);
    ofstream fout(argv[2]);
    if (!fin.is_open() || !fout.is_open()) {
        cerr << "Error: Unable to open files." << endl;
        return 1;
    }

    firstPass(fin);
    string line;
    while (getline(fin, line)) {
        vector<string> tokens = tokenize(line);
        if (tokens.empty()) continue;

        if (tokens[0] == ".data") { mode = 1; continue; }
        if (tokens[0] == ".text") { mode = 0; continue; }

        if (mode == 0) {
            processInstruction(fout, tokens);
        } else if (mode == 1) {
            processDataSegment(fout, tokens, data_address, line);
        }
    }

    fout << "0x" << setw(8) << setfill('0') << hex << text_address << " END_OF_TEXT" << endl;
    fin.close();
    fout.close();

    return 0;
}