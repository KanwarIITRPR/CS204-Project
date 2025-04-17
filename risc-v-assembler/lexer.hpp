#ifndef LEXER_H
#define LEXER_H

#include "utils.hpp"

class Lexer {
    private:
        ifstream fin;
        string TrimString(string input_string);
        bool IsValidLabel(string label, bool log_error = false);
    
    public:
        vector<string> Tokenize(string current_line, bool remove_labels = true);
        bool GetNextInstruction(string &current_line);
        void ResetInputFile();

        Lexer(string assembly_file) {
            fin.open(assembly_file);
            if (!fin.is_open()) { error_stream << "Couldn't open input file: " << assembly_file << "\n"; return; }
        }
};

#endif