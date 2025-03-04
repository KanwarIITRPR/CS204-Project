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
    if (s.empty()) return false;
    if (s[0] == '-' || s[0] == '+') return all_of(s.begin() + 1, s.end(), ::isdigit);
    return all_of(s.begin(), s.end(), ::isdigit);
}

vector<string> tokenize(string instruct) {
    vector<string> tokens;
    stringstream ss(instruct);
    string token;
    while (ss >> token) {
        if (token[0] == '#') break; // Ignore comments
        if (token.back() == ':') token.pop_back(); 
        tokens.push_back(token);
    }
    return tokens;
}

string generateBinaryCode(vector<string> &tokens) {
    string binary_code;

    if (r_opcode.find(tokens[0]) != r_opcode.end()) {
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
        binary_code = r_func7[tokens[0]] + rs2 + rs1 + r_func3[tokens[0]] + rd + r_opcode[tokens[0]];
    }
    else if (i_opcode.find(tokens[0]) != i_opcode.end()) {
        if (tokens.size() < 4) {
            cerr << "Error: Invalid I-type instruction format -> " << tokens[0] << endl;
            return "";
        }

        tokens[1].erase(0, 1);
        tokens[2].erase(0, 1);

        if (!isNumber(tokens[1]) || !isNumber(tokens[2])) {
            cerr << "Error: Invalid register number in I-type instruction -> " << tokens[0] << endl;
            return "";
        }

        string rd = dec_to_bin(stoi(tokens[1]), 5);
        string rs1 = dec_to_bin(stoi(tokens[2]), 5);
        string imm = isNumber(tokens[3]) ? dec_to_bin(stoi(tokens[3]), 12) : "";

        if (imm.empty()) {
            cerr << "Error: Invalid immediate value in I-type instruction -> " << tokens[0] << endl;
            return "";
        }

        binary_code = imm + rs1 + i_func3[tokens[0]] + rd + i_opcode[tokens[0]];
    }
    else if (sb_func3.find(tokens[0]) != sb_func3.end()) {
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

        if (labelAddress.find(tokens[3]) == labelAddress.end()) {
            cerr << "Error: Undefined label in SB-type instruction -> " << tokens[3] << endl;
            return "";
        }

        int offset = labelAddress[tokens[3]] - text_address;
        string imm = dec_to_bin(offset, 13);
        binary_code = imm[0] + imm.substr(2, 6) + rs2 + rs1 + sb_func3[tokens[0]] + imm.substr(8, 4) + imm[1] + "1100011";
    }
    else if (s_opcode.find(tokens[0]) != s_opcode.end()) {
        if (tokens.size() < 4) {
            cerr << "Error: Invalid S-type instruction format -> " << tokens[0] << endl;
            return "";
        }
    
        tokens[1].erase(0, 1); // Remove 'x' from register
        tokens[2].erase(0, 1); 
    
        if (!isNumber(tokens[1]) || !isNumber(tokens[2])) {
            cerr << "Error: Invalid register in S-type instruction -> " << tokens[0] << endl;
            return "";
        }
    
        string rs2 = dec_to_bin(stoi(tokens[1]), 5);
        string rs1 = dec_to_bin(stoi(tokens[2]), 5);
    
        // Handle immediate value (e.g., sw x10, 12(x11))
        size_t parenPos = tokens[3].find('(');
        if (parenPos == string::npos) {
            cerr << "Error: Invalid memory access format in S-type -> " << tokens[0] << endl;
            return "";
        }
    
        int imm = stoi(tokens[3].substr(0, parenPos));
        string imm_bin = dec_to_bin(imm, 12);
        string imm_high = imm_bin.substr(0, 7);
        string imm_low = imm_bin.substr(7, 5);
        
        string func3 = s_func3[tokens[0]];
        string opcode = s_opcode[tokens[0]];
    
        binary_code = imm_high + rs2 + rs1 + func3 + imm_low + opcode;
    }

    else if (u_opcode.find(tokens[0]) != u_opcode.end()) {
        if (tokens.size() < 3) {
            cerr << "Error: Invalid U-type instruction format -> " << tokens[0] << endl;
            return "";
        }
    
        // Remove 'x' from rd register
        tokens[1].erase(0, 1);
    
        if (!isNumber(tokens[1])) {
            cerr << "Error: Invalid register in U-type instruction -> " << tokens[0] << endl;
            return "";
        }
    
        string rd = dec_to_bin(stoi(tokens[1]), 5);
    
        // Immediate value should be 20-bit
        if (!isNumber(tokens[2])) {
            cerr << "Error: Invalid immediate in U-type instruction -> " << tokens[0] << endl;
            return "";
        }
    
        int imm_value = stoi(tokens[2]);
    
        // Ensure the immediate value is within the 20-bit range
        if (imm_value < -524288 || imm_value > 524287) {
            cerr << "Error: Immediate value out of range for U-type instruction -> " << tokens[0] << endl;
            return "";
        }
    
        string imm = dec_to_bin(imm_value, 20);
    
        binary_code = imm + rd + u_opcode[tokens[0]];
    }
    
    else if (tokens[0] == "jal") {
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

        if (labelAddress.find(tokens[2]) == labelAddress.end()) {
            cerr << "Error: Undefined label in JAL instruction -> " << tokens[2] << endl;
            return "";
        }

        int offset = labelAddress[tokens[2]] - text_address;
        string imm = dec_to_bin(offset, 21);
        binary_code = imm[0] + imm.substr(10, 10) + imm[9] + imm.substr(1, 8) + rd + "1101111";
    }
    else {
        cerr << "Error: Unrecognized instruction -> " << tokens[0] << endl;
    }

    return binary_code;
}

void resetFile(ifstream &fin) {
    fin.clear();
    fin.seekg(0);
}

void processInstruction(ofstream &fout, vector<string> &tokens, int &text_address) {
    if (tokens.empty()) return;

    string binary_code = generateBinaryCode(tokens);
    if (binary_code.empty()) {
        cerr << "Error: Failed to generate binary code for instruction: ";
        for (const auto &token : tokens) cerr << token << " ";
        cerr << endl;
        return;
    }

    string hex_code = bin_to_hex(binary_code);
    fout << "0x" << setw(8) << setfill('0') << hex << text_address
         << " 0x" << setw(8) << setfill('0') << hex << hex_code
         << " , " << tokens[0];

    for (size_t i = 1; i < tokens.size(); i++) {
        fout << " " << tokens[i];
    }

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
        if (numValues == 0) numValues = 1; // No values assigned

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

        // Fill remaining uninitialized slots with zero
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

    string line;
    int data_address = 0x10000000;

    // First Pass
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

    resetFile(fin);
    text_address = CODE_START;
    data_address = 0x10000000;

    // Second Pass
    while (getline(fin, line)) {
        vector<string> tokens = tokenize(line);
        if (tokens.empty()) continue;

        if (tokens[0] == ".data") { mode = 1; continue; }
        if (tokens[0] == ".text") { mode = 0; continue; }

        if (mode == 0) {
            processInstruction(fout, tokens, text_address);
        } else if (mode == 1) {
            processDataSegment(fout, tokens, data_address, line);
        }
    }

    fout << "0x" << setw(8) << setfill('0') << hex << text_address << " END_OF_TEXT" << endl;
    fin.close();
    fout.close();

    return 0;
}
