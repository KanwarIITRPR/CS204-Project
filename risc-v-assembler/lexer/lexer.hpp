#ifndef LEXER_H
#define LEXER_H

#include "operations.hpp"

class Lexer {
    private:
        ifstream fin;
        string TrimString(string input_string);
        bool IsValidLabel(string label, bool log_error = true);
    
    public:
        vector<string> Tokenize(bool remove_labels, bool remove_comments);
        Lexer(string assembly_file) {
            fin.open(assembly_file);
        }
};

#endif