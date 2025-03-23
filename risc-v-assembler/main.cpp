#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include "commands.cpp"
using namespace std;

ifstream fin;
ofstream fout;

string instruction;
vector<string> tokens;
string machineCode;
vector<string> machineCodeDivision(7);

int current_address;
map<string, int> label_address;

vector<string> Tokenize(bool remove_comments = true, bool skip_label = false) {
    stringstream instructionStream;
    vector<string> currTokens;
    string currToken;

    if (remove_comments && instruction.find("#") != -1) instruction = instruction.substr(0, instruction.find("#"));
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

void ConvertToMachineLanguage(string input_file, string output_file) {
    fin.open(input_file);
    fout.open(output_file);
    if (!fin.is_open() || !fout.is_open()) {
        cerr << "Error: Unable to open files" << endl;
        return;
    }

    InitializeInstructions();
    DefineCodes();
    ExtractLabelAddresses();

    int text_mode = 1;
    bool actually_started = false;
    current_address = TEXT_ADDRESS;
    while (getline(fin, instruction)) {
        if (Tokenize().empty()) continue;

        if (tokens[0] == ".data") {
            if (actually_started) fout << "0x" << setw(8) << setfill('0') << hex << current_address << " END-OF-TEXT-SEGMENT" << endl;
            text_mode = 0;
            current_address = DATA_ADDRESS;
            fout << ".data" << endl;
            continue;
        } else if (tokens[0] == ".text") {
            if (actually_started) fout << "0x" << setw(8) << setfill('0') << hex << current_address << " END-OF-DATA-SEGMENT" << endl;
            text_mode = 1;
            current_address = TEXT_ADDRESS;
            fout << ".text" << endl;
            continue;
        }
        
        if (text_mode) ProcessCode();
        else ProcessData();
        actually_started = true;
    }
    if (text_mode) fout << "0x" << setw(8) << setfill('0') << hex << current_address << " END-OF-TEXT-SEGMENT" << endl;
    else fout << "0x" << setw(8) << setfill('0') << hex << current_address << " END-OF-DATA-SEGMENT" << endl;
}

// int main(int argC, char* argV[]) {
//     if (argC < 3) {
//         cerr << "Usage: " << argV[0] << " <input.asm> <output.mc>" << endl;
//         return 1;
//     }

//     ConvertToMachineLanguage(argV[1], argV[2]);
// }